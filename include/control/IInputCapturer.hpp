#pragma once

#include "common/InputEvents.hpp"
#include <functional>
#include <memory>

// Forward declaration in global namespace
struct SDL_Window;

namespace kvm::control {

using KeyboardCallback = std::function<void(const common::KeyboardEvent&)>;
using MouseCallback = std::function<void(const common::MouseEvent&)>;

/**
 * @brief Interface for capturing raw OS-level input via SDL3.
 */
class IInputCapturer {
public:
    virtual ~IInputCapturer() = default;

    /**
     * @brief Set callbacks to receive events from the capturer.
     */
    virtual void SetKeyboardCallback(KeyboardCallback cb) = 0;
    virtual void SetMouseCallback(MouseCallback cb) = 0;

    /**
     * @brief Processes a single SDL event.
     */
    virtual void ProcessEvent(const void* sdlEvent) = 0;

    /**
     * @brief Toggles "Relative Mouse Mode" for high-performance input tracking.
     */
    virtual void SetCaptureEnabled(bool enabled) = 0;
};

/**
 * @brief Factory function to create an SDL-based input capturer.
 */
std::unique_ptr<IInputCapturer> CreateInputCapturer(SDL_Window* window);

} // namespace kvm::control
