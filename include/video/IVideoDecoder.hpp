#pragma once

#include <cstdint>
#include <memory>
#include <string>

// Forward declarations
struct SDL_Renderer;
namespace kvm::network { class IHttpClient; }

namespace kvm::video {

/**
 * @brief Interface for hardware-accelerated H.264 video decoding.
 */
class IVideoDecoder {
public:
    virtual ~IVideoDecoder() = default;

    virtual bool Initialize() = 0;
    virtual bool OpenStream(const std::string& url) = 0;
    virtual bool IsConnected() const noexcept = 0;
    virtual void* GetTexture() noexcept = 0;
    virtual void Flush() noexcept = 0;
    virtual uint32_t GetWidth() const noexcept = 0;
    virtual uint32_t GetHeight() const noexcept = 0;
};

/**
 * @brief Factory function to create a video decoder.
 */
std::unique_ptr<IVideoDecoder> CreateVideoDecoder(SDL_Renderer* renderer, std::shared_ptr<network::IHttpClient> httpClient);

} // namespace kvm::video
