#pragma once

#include <cstdint>
#include <span>
#include <memory>

// Forward declaration of SDL_Renderer to avoid SDL header inclusion here
struct SDL_Renderer;

namespace kvm::video {

/**
 * @brief Result codes for decoder operations.
 */
enum class DecodeStatus {
    Success,        ///< Operation completed successfully
    Error,          ///< An internal error occurred
    NeedMoreData,   ///< Packet accepted, but no frame is ready yet
    FrameReady      ///< New hardware frame is decoded and available
};

/**
 * @brief Interface for hardware-accelerated H.264 video decoding.
 * 
 * Implementations should handle platform-specific APIs (DXVA2/D3D11VA, VAAPI, VideoToolbox)
 * and provide a way to access the decoded frame directly in GPU memory.
 */
class IHardwareDecoder {
public:
    virtual ~IHardwareDecoder() = default;

    /**
     * @brief Initializes the hardware decoder and its associated GPU context.
     * @return true if the hardware context was successfully created.
     */
    virtual bool Initialize() = 0;

    /**
     * @brief Submits a raw H.264 packet (NAL units) to the decoder.
     * 
     * @param encoded_data View of the raw H.264 baseline data.
     * @param pts Presentation Timestamp of the packet.
     * @return DecodeStatus indicating if a frame was produced or if more data is required.
     */
    virtual DecodeStatus SubmitPacket(std::span<const uint8_t> encoded_data, int64_t pts) = 0;

    /**
     * @brief Retrieves the hardware texture descriptor for rendering.
     * 
     * This method provides Zero-Copy access to the decoded frame. 
     * In an SDL2 context, the return value is typically a pointer to SDL_Texture.
     * 
     * @return Opaque pointer to the hardware-backed texture resource.
     */
    virtual void* GetTexture() const noexcept = 0;

    /**
     * @brief Flushes internal buffers and resets the decoder state.
     * Useful during reconnection or stream parameter changes.
     */
    virtual void Flush() noexcept = 0;

    /**
     * @brief Gets the current video width in pixels.
     */
    virtual uint32_t GetWidth() const noexcept = 0;

    /**
     * @brief Gets the current video height in pixels.
     */
    virtual uint32_t GetHeight() const noexcept = 0;
};

/**
 * @brief Factory function to create a hardware decoder for a given SDL renderer.
 * 
 * @param renderer The SDL_Renderer instance to associate with the decoder.
 * @return Unique pointer to the created IHardwareDecoder instance.
 */
std::unique_ptr<IHardwareDecoder> CreateHardwareDecoder(SDL_Renderer* renderer);

} // namespace kvm::video
