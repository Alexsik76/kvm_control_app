#pragma once

#include "video/IVideoDecoder.hpp"
#include <memory>

struct SDL_Renderer;
struct SDL_Texture;
struct AVFrame;

namespace kvm::video {

class FFmpegStreamNode;

class SDLVideoDecoder : public IVideoDecoder {
public:
    explicit SDLVideoDecoder(SDL_Renderer* renderer);
    ~SDLVideoDecoder() override;

    bool Initialize() override;
    bool OpenStream(const std::string& url) override;
    
    void* GetTexture() const noexcept override;
    void Flush() noexcept override;

    uint32_t GetWidth() const noexcept override { return m_width; }
    uint32_t GetHeight() const noexcept override { return m_height; }

private:
    void UpdateTexture();

    SDL_Renderer* m_renderer = nullptr;
    SDL_Texture* m_texture = nullptr;

    std::unique_ptr<FFmpegStreamNode> m_stream_node;
    AVFrame* m_render_frame = nullptr;

    uint32_t m_width = 0;
    uint32_t m_height = 0;
};

} // namespace kvm::video
