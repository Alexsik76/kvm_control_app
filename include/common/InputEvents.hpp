#pragma once

#include <cstdint>
#include <vector>

namespace kvm::common {

/**
 * @brief Represents a raw keyboard state to be translated into HID reports.
 */
struct KeyboardEvent {
    uint8_t modifiers{0};      // Bitmask (Shift, Ctrl, Alt, Gui)
    std::vector<uint8_t> keys; // List of active scancodes
};

/**
 * @brief Represents relative mouse movement and button states.
 */
struct MouseEvent {
    uint8_t buttons{0};  // Bitmask of pressed buttons
    int16_t deltaX{0};   // Relative horizontal movement
    int16_t deltaY{0};   // Relative vertical movement
    int8_t wheel{0};     // Vertical scroll wheel delta
};

} // namespace kvm::common
