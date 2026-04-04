#include "control/IHIDClient.hpp"
#include <iostream>
#include <memory>

namespace kvm::control {

class StubHIDClient : public IHIDClient {
public:
    bool Connect(const std::string& url) override {
        m_url = url;
        m_connected = true;
        std::cout << "[HID] Mock connected to: " << url << "\n";
        return true;
    }

    void SendKeyboardEvent(const common::KeyboardEvent& event) override {
        // In a real implementation, this would send a WebSocket packet
        if (!event.keys.empty()) {
            // Log the first key for debug
            // std::cout << "[HID] Sending Key: " << (int)event.keys[0] << "\n";
        }
    }

    void SendMouseEvent(const common::MouseEvent& event) override {
        // std::cout << "[HID] Sending Mouse Delta: " << event.deltaX << ", " << event.deltaY << "\n";
    }

    bool IsConnected() const noexcept override { return m_connected; }

private:
    bool m_connected{false};
    std::string m_url;
};

/**
 * Factory function for the HID client.
 */
std::unique_ptr<IHIDClient> CreateHIDClient() {
    return std::make_unique<StubHIDClient>();
}

} // namespace kvm::control
