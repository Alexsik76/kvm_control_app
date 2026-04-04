#pragma once

#include "common/InputEvents.hpp"
#include <string>
#include <memory>

namespace kvm::control {

/**
 * @brief Interface for an asynchronous HID client sending reports to the node.
 */
class IHIDClient {
public:
    virtual ~IHIDClient() = default;

    /**
     * @brief Establishes a WebSocket connection to the hardware node.
     */
    virtual bool Connect(const std::string& url) = 0;

    /**
     * @brief Sends a keyboard report to the node.
     */
    virtual void SendKeyboardEvent(const common::KeyboardEvent& event) = 0;

    /**
     * @brief Sends a mouse report to the node.
     */
    virtual void SendMouseEvent(const common::MouseEvent& event) = 0;

    /**
     * @brief Returns true if the WebSocket connection is active.
     */
    virtual bool IsConnected() const noexcept = 0;
};

/**
 * @brief Factory function for creating the HID client.
 */
std::unique_ptr<IHIDClient> CreateHIDClient();

} // namespace kvm::control
