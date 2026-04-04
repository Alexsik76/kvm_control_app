#include "control/IInputCapturer.hpp"
#include <SDL3/SDL.h>
#include <memory>
#include <iostream>

namespace kvm::control {

class SDLInputCapturer : public IInputCapturer {
public:
    explicit SDLInputCapturer(SDL_Window* window) : m_window(window) {}

    void SetKeyboardCallback(KeyboardCallback cb) override { m_kbCallback = std::move(cb); }
    void SetMouseCallback(MouseCallback cb) override { m_mouseCallback = std::move(cb); }

    void ProcessEvent(const void* sdlEvent) override {
        const SDL_Event& ev = *static_cast<const SDL_Event*>(sdlEvent);
        
        switch (ev.type) {
            case SDL_EVENT_KEY_DOWN:
            case SDL_EVENT_KEY_UP:
                HandleKeyboardEvent(ev.key);
                break;

            case SDL_EVENT_MOUSE_MOTION:
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
            case SDL_EVENT_MOUSE_BUTTON_UP:
            case SDL_EVENT_MOUSE_WHEEL:
                HandleMouseEvent(ev);
                break;
        }
    }

    void SetCaptureEnabled(bool enabled) override {
        if (!m_window) return;
        SDL_SetWindowRelativeMouseMode(m_window, enabled);
    }

private:
    void HandleKeyboardEvent(const SDL_KeyboardEvent& ev) {
        if (!m_kbCallback) return;

        common::KeyboardEvent event;
        event.modifiers = static_cast<uint8_t>(ev.mod);
        event.keys.push_back(static_cast<uint8_t>(ev.scancode));
        
        m_kbCallback(event);
    }

    void HandleMouseEvent(const SDL_Event& ev) {
        if (!m_mouseCallback) return;

        common::MouseEvent mouseEv;
        if (ev.type == SDL_EVENT_MOUSE_MOTION) {
            mouseEv.deltaX = static_cast<int16_t>(ev.motion.xrel);
            mouseEv.deltaY = static_cast<int16_t>(ev.motion.yrel);
        } else if (ev.type == SDL_EVENT_MOUSE_WHEEL) {
            mouseEv.wheel = static_cast<int8_t>(ev.wheel.y);
        } else if (ev.type == SDL_EVENT_MOUSE_BUTTON_DOWN || ev.type == SDL_EVENT_MOUSE_BUTTON_UP) {
            mouseEv.buttons = static_cast<uint8_t>(SDL_GetMouseState(nullptr, nullptr));
        }
        
        m_mouseCallback(mouseEv);
    }

private:
    SDL_Window* m_window;
    KeyboardCallback m_kbCallback;
    MouseCallback m_mouseCallback;
};

/**
 * Factory function for creating the SDL-based capturer.
 */
std::unique_ptr<IInputCapturer> CreateInputCapturer(SDL_Window* window) {
    return std::make_unique<SDLInputCapturer>(window);
}

} // namespace kvm::control
