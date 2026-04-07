#pragma once
#include <string>
#include <functional>
#include <memory>

namespace kvm::network {

struct HandshakeData {
    std::string accessToken;
    std::string streamUrl;
    std::string hidUrl;
};

/**
 * @brief Interface for IPC communication with the Launcher.
 */
class ILauncherClient {
public:
    virtual ~ILauncherClient() = default;

    /**
     * @brief Connects to the named pipe.
     * @param pipeName Unique name of the pipe (e.g. kvm_pipe_...)
     * @return true if connection established.
     */
    virtual bool Connect(const std::string& pipeName) = 0;

    /**
     * @brief Waits for the Handshake message from the Launcher.
     * This is a blocking call.
     * @return HandshakeData if successful.
     */
    virtual bool WaitForHandshake(HandshakeData& outData) = 0;

    /**
     * @brief Sends a status update message to the Launcher.
     */
    virtual void SendStatus(const std::string& message) = 0;

    /**
     * @brief Sends an error message to the Launcher.
     */
    virtual void SendError(const std::string& message) = 0;

    /**
     * @brief Closes the connection.
     */
    virtual void Close() = 0;
};

/**
 * @brief Factory function to create a launcher client.
 */
std::unique_ptr<ILauncherClient> CreateLauncherClient();

} // namespace kvm::network
