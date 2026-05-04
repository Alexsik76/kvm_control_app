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
 * @brief Returns the git short-hash of the DLL build.
 * @return Pointer to a static, null-terminated string. Never null.
 */
KVM_API const char* KvmGetVersion();
