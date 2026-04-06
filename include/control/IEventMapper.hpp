#pragma once

#include <cstdint>
#include <memory>

namespace kvm::control {

/**
 * @brief Interface for translating SDL events into raw HID codes.
 */
class IEventMapper {
public:
    virtual ~IEventMapper() = default;

    /**
     * @brief Translates SDL Scancode to USB HID Keyboard Scancode.
     */
    virtual uint8_t SDLToHIDKeyboard(uint32_t sdlScancode) = 0;

    /**
     * @brief Translates SDL Keymod to USB HID Modifiers bitmask.
     */
    virtual uint8_t SDLToHIDModifiers(uint16_t sdlMod) = 0;
};

/**
 * @brief Factory function for the event mapper.
 */
std::unique_ptr<IEventMapper> CreateEventMapper();

} // namespace kvm::control
