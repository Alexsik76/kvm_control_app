#include "video/SDLVideoDecoder.hpp"
#include "video/FFmpegStreamNode.hpp"
#include <SDL.h>
#include <iostream>

extern "C" {
#include <libavutil/frame.h>
#include <libavutil/pixfmt.h>
}

namespace kvm::video {

SDLVideoDecoder::SDLVideoDecoder(SDL_Renderer* renderer)
    : m_renderer(renderer) {
    m_stream_node = std::make_unique<FFmpegStreamNode>();
    m_render_frame = av_frame_alloc();
}

SDLVideoDecoder::~SDLVideoDecoder() {
    if (m_texture) SDL_DestroyTexture(m_texture);
    if (m_render_frame) av_frame_free(&m_render_frame);
}

bool SDLVideoDecoder::Initialize() {
    return m_stream_node->Initialize();
}

bool SDLVideoDecoder::OpenStream(const std::string& url) {
    return m_stream_node->OpenStream(url);
}

void* SDLVideoDecoder::GetTexture() const noexcept {
    const_cast<SDLVideoDecoder*>(this)->UpdateTexture();
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
    }
}

std::unique_ptr<IVideoDecoder> CreateVideoDecoder(SDL_Renderer* renderer) {
    return std::make_unique<SDLVideoDecoder>(renderer);
}

} // namespace kvm::video
