#pragma once

#include <cstdint>
#include <span>
#include <memory>
#include <string>

// Forward declaration of SDL_Renderer to avoid SDL header inclusion here
struct SDL_Renderer;

namespace kvm::video {



/**
 * @brief Interface for hardware-accelerated H.264 video decoding.
 *
 * Implementations handle platform-specific APIs (D3D11VA, VAAPI, VideoToolbox)
 * and expose a texture handle that can be passed directly to the renderer.
 */
class IVideoDecoder {
public:
    virtual ~IVideoDecoder() = default;

    /**
     * @brief Initializes the hardware decoder and its associated GPU context.
     * @return true if the hardware context was successfully created.
     */
    virtual bool Initialize() = 0;

    /**
     * @brief Opens a network/file stream via libavformat and starts the
     *        background decoding thread.
     *
     * Supported URL schemes: udp://, rtsp://, file://, etc.
     * Must be called after Initialize() and before the render loop.
     *
     * @param url  Stream URL, e.g. "udp://127.0.0.1:5000".
     * @return true on success.
     */
    virtual bool OpenStream(const std::string& url) = 0;



    /**
     * @brief Returns an opaque pointer to the decoded SDL_Texture (may be null
     *        until the first frame arrives).
     */
    virtual void* GetTexture() noexcept = 0;

    /**
     * @brief Flushes internal decoder buffers. Call on reconnection.
     */
    virtual void Flush() noexcept = 0;

    /** @brief Current video width in pixels (0 until first frame). */
    virtual uint32_t GetWidth()  const noexcept = 0;

    /** @brief Current video height in pixels (0 until first frame). */
    virtual uint32_t GetHeight() const noexcept = 0;
};

/**
 * @brief Factory function — creates a D3D11VA-accelerated decoder for the
 *        given SDL renderer (Windows only; falls back to software on failure).
 */
std::unique_ptr<IVideoDecoder> CreateVideoDecoder(SDL_Renderer* renderer);

} // namespace kvm::video
