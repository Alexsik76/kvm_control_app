#include "video/IHardwareDecoder.hpp"
#include <SDL.h>
#include <windows.h>
#include <d3d11.h>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/hwcontext.h>
#include <libavutil/hwcontext_d3d11va.h>
#include <libavutil/imgutils.h>
}

#include <iostream>
#include <iomanip>
#include <memory>

namespace kvm::video {

class HardwareDecoder : public IHardwareDecoder {
public:
    explicit HardwareDecoder(SDL_Renderer* renderer) 
        : m_renderer(renderer) {}

    ~HardwareDecoder() override {
        Cleanup();
    }

    bool Initialize() override {
        Cleanup();

        const AVCodec* codec = avcodec_find_decoder(AV_CODEC_ID_H264);
        if (!codec) return false;

        m_codec_ctx = avcodec_alloc_context3(codec);
        if (!m_codec_ctx) return false;

        if (av_hwdevice_ctx_create(&m_hw_device_ctx, AV_HWDEVICE_TYPE_D3D11VA, nullptr, nullptr, 0) < 0) {
            std::cerr << "Decoder: Failed to create D3D11VA context" << std::endl;
            return false;
        }
        m_codec_ctx->hw_device_ctx = av_buffer_ref(m_hw_device_ctx);

        if (avcodec_open2(m_codec_ctx, codec, nullptr) < 0) return false;

        // Initialize Parser
        m_parser = av_parser_init(codec->id);
        if (!m_parser) return false;
        m_parser->flags |= PARSER_FLAG_COMPLETE_FRAMES; // Try to output complete frames if possible

        m_frame = av_frame_alloc();
        m_sw_frame = av_frame_alloc();
        m_packet = av_packet_alloc();

        std::cout << "[Decoder] Initialized successfully." << std::endl;
        return true;
    }

    DecodeStatus SubmitPacket(std::span<const uint8_t> encoded_data, int64_t pts) override {
        if (!m_codec_ctx || !m_parser) return DecodeStatus::Error;

        uint8_t* data = const_cast<uint8_t*>(encoded_data.data());
        int size = static_cast<int>(encoded_data.size());

        // std::cout << "[Network] Received UDP chunk of size: " << size << " bytes" << std::endl;

        DecodeStatus final_status = DecodeStatus::NeedMoreData;

        while (size > 0) {
            int ret = av_parser_parse2(m_parser, m_codec_ctx, 
                &m_packet->data, &m_packet->size,
                data, size, AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);
            
            if (ret < 0) return DecodeStatus::Error;
            if (ret == 0) break; // Prevent infinite loop

            data += ret;
            size -= ret;

            if (m_packet->size > 0) {
                // LOGGING NAL UNIT INFO
                if (m_packet->size >= 5) {
                    int nal_type = m_packet->data[4] & 0x1F;
                    std::cout << "[Parser] Assembled packet size: " << m_packet->size 
                              << " bytes | NAL Type: " << nal_type;
                    
                    if (nal_type == 7) std::cout << " (SPS)";
                    else if (nal_type == 8) std::cout << " (PPS)";
                    else if (nal_type == 5) std::cout << " (IDR Keyframe)";
                    else if (nal_type == 1) std::cout << " (P/B Frame)";
                    
                    std::cout << std::endl;
                }

                DecodeStatus status = DecodeInternal(pts);
                if (status == DecodeStatus::FrameReady) {
                    final_status = DecodeStatus::FrameReady;
                }
            }
        }

        return final_status;
    }

    void* GetTexture() const noexcept override { return m_texture; }
    void Flush() noexcept override { if (m_codec_ctx) avcodec_flush_buffers(m_codec_ctx); }
    uint32_t GetWidth() const noexcept override { return m_width; }
    uint32_t GetHeight() const noexcept override { return m_height; }

private:
    DecodeStatus DecodeInternal(int64_t pts) {
        m_packet->pts = pts;
        int ret = avcodec_send_packet(m_codec_ctx, m_packet);
        if (ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF) {
            std::cerr << "[Decoder] Error sending packet: " << ret << std::endl;
            return DecodeStatus::Error;
        }

        bool got_frame = false;
        while (true) {
            ret = avcodec_receive_frame(m_codec_ctx, m_frame);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                break;
            } else if (ret < 0) {
                std::cerr << "[Decoder] Error receiving frame: " << ret << std::endl;
                // Don't abort completely, try to continue with next packets
                break; 
            }

            // Even if the frame has corrupted macroblocks, FFmpeg's error concealment
            // tries to fix it. We should render it anyway.
            UpdateTexture();
            got_frame = true;
        }

        return got_frame ? DecodeStatus::FrameReady : DecodeStatus::NeedMoreData;
    }

    void UpdateTexture() {
        if (m_frame->width == 0 || m_frame->height == 0) return;

        m_width = m_frame->width;
        m_height = m_frame->height;

        AVFrame* render_frame = m_frame;

        // Check if the frame is a hardware format (like D3D11)
        if (m_frame->format == AV_PIX_FMT_D3D11 || m_frame->hw_frames_ctx != nullptr) {
            av_frame_unref(m_sw_frame);
            int ret = av_hwframe_transfer_data(m_sw_frame, m_frame, 0);
            if (ret < 0) {
                std::cerr << "[Decoder] HW Transfer failed: " << ret << std::endl;
                return;
            }
            render_frame = m_sw_frame;
        }

        // Detect if we need NV12 or IYUV based on software frame format
        uint32_t sdl_format = (render_frame->format == AV_PIX_FMT_NV12) ? SDL_PIXELFORMAT_NV12 : SDL_PIXELFORMAT_IYUV;

        if (!m_texture || m_width != static_cast<uint32_t>(m_tex_w) || m_height != static_cast<uint32_t>(m_tex_h)) {
            std::cout << "[Decoder] Creating SDL Texture: " << m_width << "x" << m_height 
                      << " (Format: " << render_frame->format << ")" << std::endl;
            if (m_texture) SDL_DestroyTexture(m_texture);
            m_texture = SDL_CreateTexture(m_renderer, sdl_format, SDL_TEXTUREACCESS_STREAMING, m_width, m_height);
            m_tex_w = m_width;
            m_tex_h = m_height;
        }

        if (m_texture && render_frame->data[0]) {
            if (sdl_format == SDL_PIXELFORMAT_NV12) {
                // SDL2 2.0.16+ has SDL_UpdateNVTexture
                SDL_UpdateNVTexture(m_texture, nullptr,
                    render_frame->data[0], render_frame->linesize[0],
                    render_frame->data[1], render_frame->linesize[1]);
            } else {
                SDL_UpdateYUVTexture(m_texture, nullptr,
                    render_frame->data[0], render_frame->linesize[0],
                    render_frame->data[1], render_frame->linesize[1],
                    render_frame->data[2], render_frame->linesize[2]);
            }
        }
    }

    void Cleanup() {
        if (m_texture) { SDL_DestroyTexture(m_texture); m_texture = nullptr; }
        if (m_codec_ctx) { avcodec_free_context(&m_codec_ctx); m_codec_ctx = nullptr; }
        if (m_hw_device_ctx) { av_buffer_unref(&m_hw_device_ctx); m_hw_device_ctx = nullptr; }
        if (m_parser) { av_parser_close(m_parser); m_parser = nullptr; }
        if (m_frame) { av_frame_free(&m_frame); m_frame = nullptr; }
        if (m_sw_frame) { av_frame_free(&m_sw_frame); m_sw_frame = nullptr; }
        if (m_packet) { av_packet_free(&m_packet); m_packet = nullptr; }
    }

    SDL_Renderer* m_renderer = nullptr;
    SDL_Texture* m_texture = nullptr;
    AVCodecContext* m_codec_ctx = nullptr;
    AVCodecParserContext* m_parser = nullptr;
    AVBufferRef* m_hw_device_ctx = nullptr;
    AVFrame* m_frame = nullptr;
    AVFrame* m_sw_frame = nullptr;
    AVPacket* m_packet = nullptr;
    uint32_t m_width = 0, m_height = 0;
    int m_tex_w = 0, m_tex_h = 0;
};

std::unique_ptr<IHardwareDecoder> CreateHardwareDecoder(SDL_Renderer* renderer) {
    return std::make_unique<HardwareDecoder>(renderer);
}

} // namespace kvm::video
