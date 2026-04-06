#include "control/IInputCapturer.hpp"
#include <SDL3/SDL.h>
#include <memory>
#include <iostream>
#include <vector>
#include <algorithm>

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
                HandleKeyboardEvent(ev);
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
    void HandleKeyboardEvent(const SDL_Event& ev) {
        if (!m_kbCallback) return;

        uint32_t scancode = ev.key.scancode;

        // Track pressed keys
        if (ev.type == SDL_EVENT_KEY_DOWN) {
            if (std::find(m_pressedKeys.begin(), m_pressedKeys.end(), scancode) == m_pressedKeys.end()) {
                m_pressedKeys.push_back(scancode);
            }
        } else if (ev.type == SDL_EVENT_KEY_UP) {
            auto it = std::find(m_pressedKeys.begin(), m_pressedKeys.end(), scancode);
            if (it != m_pressedKeys.end()) {
                m_pressedKeys.erase(it);
            }
        }

        common::KeyboardEvent event;
        event.modifiers = static_cast<uint8_t>(ev.key.mod);
        
        // USB HID supports a maximum of 6 simultaneous non-modifier keys
        for (size_t i = 0; i < m_pressedKeys.size() && i < 6; ++i) {
            event.keys.push_back(static_cast<uint8_t>(m_pressedKeys[i]));
        }
        
        m_kbCallback(event);
    }

    void HandleMouseEvent(const SDL_Event& ev) {
        if (!m_mouseCallback) return;

        common::MouseEvent mouseEv = {};
        
        if (ev.type == SDL_EVENT_MOUSE_MOTION) {
            // Clamp deltas to [-127, 127] because the Go server expects int8
            mouseEv.deltaX = static_cast<int16_t>(std::clamp(static_cast<int>(ev.motion.xrel), -127, 127));
            mouseEv.deltaY = static_cast<int16_t>(std::clamp(static_cast<int>(ev.motion.yrel), -127, 127));
        } else if (ev.type == SDL_EVENT_MOUSE_WHEEL) {
            mouseEv.wheel = static_cast<int8_t>(ev.wheel.y);
        } else if (ev.type == SDL_EVENT_MOUSE_BUTTON_DOWN || ev.type == SDL_EVENT_MOUSE_BUTTON_UP) {
            // Re-evaluate mouse state on button events to ensure accuracy
            uint32_t state = SDL_GetMouseState(nullptr, nullptr);
            m_mouseButtons = 0;
            
            // Map SDL masks to USB HID standard masks (Left: Bit 0, Right: Bit 1, Middle: Bit 2)
            if (state & SDL_BUTTON_LMASK) m_mouseButtons |= 0x01; 
            if (state & SDL_BUTTON_RMASK) m_mouseButtons |= 0x02; 
            if (state & SDL_BUTTON_MMASK) m_mouseButtons |= 0x04; 
        }
        
        // Always transmit the latest button state alongside any movement
        mouseEv.buttons = m_mouseButtons;
        
        m_mouseCallback(mouseEv);
    }

private:
    SDL_Window* m_window;
    KeyboardCallback m_kbCallback;
    MouseCallback m_mouseCallback;
    
    // State trackers
    std::vector<uint32_t> m_pressedKeys;
    uint8_t m_mouseButtons = 0;
};

/**
 * Factory function for creating the SDL-based capturer.
 */
std::unique_ptr<IInputCapturer> CreateInputCapturer(SDL_Window* window) {
    return std::make_unique<SDLInputCapturer>(window);
}

} // namespace kvm::control
