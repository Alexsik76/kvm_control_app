#pragma once

#include <cstdint>

#ifdef _WIN32
    #ifdef KVM_EXPORT
        #define KVM_API extern "C" __declspec(dllexport)
    #else
        #define KVM_API extern "C" __declspec(dllimport)
    #endif
#else
    #define KVM_API extern "C" __attribute__((visibility("default")))
#endif

/**
 * @brief Callback function type for receiving decoded frames.
 * @param data Pointer to the BGRA8888 frame data.
 * @param width Frame width.
 * @param height Frame height.
 * @param stride Number of bytes per row (usually width * 4).
 */
typedef void (*FrameCallback)(uint8_t* data, int width, int height, int stride);

/**
 * @brief Initializes the KVM video stream.
 * @param url The stream URL (e.g., https://.../signal/offer).
 * @param token Authentication token.
 * @param callback Function to call when a new frame is decoded.
 * @return 0 on success, non-zero on error.
 */
KVM_API int KvmInitialize(const char* url, const char* token, FrameCallback callback);

/**
 * @brief Stops the current video stream and cleans up resources.
 */
KVM_API void KvmStop();

/**
 * @brief Manually retrieves the latest frame (optional alternative to callback).
 * @param data Pointer to pointer that will receive the data address.
 * @param width Pointer to receive the width.
 * @param height Pointer to receive the height.
 * @return 0 on success, -1 if no frame is available.
 */
KVM_API int KvmGetFrame(uint8_t** data, int* width, int* height);
