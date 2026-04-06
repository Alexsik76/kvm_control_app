#include "video/SDLVideoDecoder.hpp"
#include "video/WebRTCStreamNode.hpp"
#include <SDL3/SDL.h>
#include <iostream>

extern "C" {
#include <libavutil/frame.h>
#include <libavutil/pixfmt.h>
#include <libavutil/log.h>
}

namespace kvm::video {

SDLVideoDecoder::SDLVideoDecoder(SDL_Renderer* renderer, std::shared_ptr<network::IHttpClient> httpClient)
    : m_renderer(renderer) {
    m_stream_node = std::make_unique<WebRTCStreamNode>(std::move(httpClient));
    m_render_frame = av_frame_alloc();
}

SDLVideoDecoder::~SDLVideoDecoder() {
    if (m_texture) SDL_DestroyTexture(m_texture);
    if (m_render_frame) av_frame_free(&m_render_frame);
}

bool SDLVideoDecoder::Initialize() {
    av_log_set_level(AV_LOG_ERROR);
    return m_stream_node->Initialize();
}

void SDLVideoDecoder::SetRenderer(SDL_Renderer* renderer) {
    m_renderer = renderer;
}

bool SDLVideoDecoder::OpenStream(const std::string& url) {
    return m_stream_node->OpenStream(url);
}

bool SDLVideoDecoder::IsConnected() const noexcept {
    return m_stream_node->IsConnected();
}

void* SDLVideoDecoder::GetTexture() noexcept {
    UpdateTexture();
    return m_texture;
}

void SDLVideoDecoder::Flush() noexcept {
    m_stream_node->Flush();
}

void SDLVideoDecoder::UpdateTexture() {
    if (!m_stream_node->GetLatestFrame(m_render_frame)) {
        return;
    }
    
    if (m_render_frame->width != static_cast<int>(m_width) || 
        m_render_frame->height != static_cast<int>(m_height) || !m_texture) {
        
        m_width = m_render_frame->width;
        m_height = m_render_frame->height;
        
        if (m_texture) SDL_DestroyTexture(m_texture);
        
        m_texture = SDL_CreateTexture(
            m_renderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, m_width, m_height
        );
    }

    if (m_render_frame->format == AV_PIX_FMT_YUV420P || m_render_frame->format == AV_PIX_FMT_YUVJ420P) {
        SDL_UpdateYUVTexture(
            m_texture, nullptr,
            m_render_frame->data[0], m_render_frame->linesize[0],
            m_render_frame->data[1], m_render_frame->linesize[1],
            m_render_frame->data[2], m_render_frame->linesize[2]
        );
    } else {
        av_log(nullptr, AV_LOG_WARNING, "[Decoder] Unsupported pixel format: %d. Use YUV420P for direct rendering.\n", m_render_frame->format);
    }
}

std::unique_ptr<IVideoDecoder> CreateVideoDecoder(SDL_Renderer* renderer, std::shared_ptr<network::IHttpClient> httpClient) {
    return std::make_unique<SDLVideoDecoder>(renderer, std::move(httpClient));
}

} // namespace kvm::video
