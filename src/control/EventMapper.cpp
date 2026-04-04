#include "control/IEventMapper.hpp"
#include <SDL3/SDL.h>
#include <memory>
#include <unordered_map>

namespace kvm::control {

class EventMapper : public IEventMapper {
public:
    uint8_t SDLToHIDKeyboard(uint32_t sdlScancode) override {
        // Simple mapping for demonstration. In a full product, 
        // this would use a complete lookup table based on USB HID Usage Tables.
        if (sdlScancode >= SDL_SCANCODE_A && sdlScancode <= SDL_SCANCODE_Z) {
            return static_cast<uint8_t>(0x04 + (sdlScancode - SDL_SCANCODE_A));
        }
        if (sdlScancode >= SDL_SCANCODE_1 && sdlScancode <= SDL_SCANCODE_9) {
            return static_cast<uint8_t>(0x1E + (sdlScancode - SDL_SCANCODE_1));
        }
        if (sdlScancode == SDL_SCANCODE_0) return 0x27;
        if (sdlScancode == SDL_SCANCODE_RETURN) return 0x28;
        if (sdlScancode == SDL_SCANCODE_ESCAPE) return 0x29;
        if (sdlScancode == SDL_SCANCODE_BACKSPACE) return 0x2A;
        if (sdlScancode == SDL_SCANCODE_TAB) return 0x2B;
        if (sdlScancode == SDL_SCANCODE_SPACE) return 0x2C;

        return 0; // Unknown or unmapped
    }

    uint8_t SDLToHIDModifiers(uint16_t sdlMod) override {
        uint8_t hidMod = 0;
        if (sdlMod & SDL_KMOD_LCTRL)  hidMod |= (1 << 0);
        if (sdlMod & SDL_KMOD_LSHIFT) hidMod |= (1 << 1);
        if (sdlMod & SDL_KMOD_LALT)   hidMod |= (1 << 2);
        if (sdlMod & SDL_KMOD_LGUI)   hidMod |= (1 << 3);
        if (sdlMod & SDL_KMOD_RCTRL)  hidMod |= (1 << 4);
        if (sdlMod & SDL_KMOD_RSHIFT) hidMod |= (1 << 5);
        if (sdlMod & SDL_KMOD_RALT)   hidMod |= (1 << 6);
        if (sdlMod & SDL_KMOD_RGUI)   hidMod |= (1 << 7);
        return hidMod;
    }
};

std::unique_ptr<IEventMapper> CreateEventMapper() {
    return std::make_unique<EventMapper>();
}

} // namespace kvm::control
