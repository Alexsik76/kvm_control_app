#pragma once
#include <cstdint>
#include <span>
#include <functional>
#include <memory>

namespace kvm::network {

/**
 * @brief Interface for receiving raw data from the network.
 */
class INetworkReceiver {
public:
    virtual ~INetworkReceiver() = default;

    /**
     * @brief Starts listening for incoming packets.
     * @param port The port to bind to.
     * @return true if successfully bound to the port.
     */
    virtual bool Start(uint16_t port) = 0;

    /**
     * @brief Stops the receiver and closes sockets.
     */
    virtual void Stop() = 0;

    /**
     * @brief Callback type for received data.
     */
    using DataCallback = std::function<void(std::span<const uint8_t> data)>;

    /**
     * @brief Sets the function to be called when a packet arrives.
     */
    virtual void SetCallback(DataCallback callback) = 0;
};

/**
 * @brief Factory function to create a UDP network receiver.
 */
std::unique_ptr<INetworkReceiver> CreateUDPReceiver();

} // namespace kvm::network
