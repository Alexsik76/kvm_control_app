#include "network/INetworkReceiver.hpp"
#include <thread>
#include <atomic>
#include <iostream>
#include <vector>
#include <chrono>
#include <span>
#include <mutex>

#ifdef _WIN32
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #include <winsock2.h>
    #include <ws2tcpip.h>
    using SocketType = SOCKET;
    #define CLOSE_SOCKET closesocket
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    using SocketType = int;
    #ifndef INVALID_SOCKET
        #define INVALID_SOCKET -1
    #endif
    #ifndef SOCKET_ERROR
        #define SOCKET_ERROR -1
    #endif
    #define CLOSE_SOCKET close
#endif

namespace kvm::network {

class UDPReceiver : public INetworkReceiver {
public:
    UDPReceiver() : m_socket(INVALID_SOCKET), m_running(false) {}

    ~UDPReceiver() override {
        Stop();
    }

    bool Start(uint16_t port) override {
        m_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (m_socket == INVALID_SOCKET) return false;

        // Increase socket receive buffer to 8MB to prevent dropping large I-frames
        int rcvbuf = 8 * 1024 * 1024;
        setsockopt(m_socket, SOL_SOCKET, SO_RCVBUF, (const char*)&rcvbuf, sizeof(rcvbuf));

        sockaddr_in serverAddr{};
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = INADDR_ANY;
        serverAddr.sin_port = htons(port);

        if (bind(m_socket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
            CLOSE_SOCKET(m_socket);
            m_socket = INVALID_SOCKET;
            return false;
        }

        m_running = true;
        m_thread = std::thread(&UDPReceiver::ReceiveLoop, this);
        return true;
    }

    void Stop() override {
        m_running = false;
        if (m_socket != INVALID_SOCKET) {
            CLOSE_SOCKET(m_socket);
            m_socket = INVALID_SOCKET;
        }
        if (m_thread.joinable()) {
            m_thread.join();
        }
    }

    void SetCallback(DataCallback callback) override {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_callback = std::move(callback);
    }

private:
    void ReceiveLoop() {
        std::vector<uint8_t> buffer(65507); // Max UDP packet size
        while (m_running) {
            int bytesReceived = recv(m_socket, (char*)buffer.data(), (int)buffer.size(), 0);
            if (bytesReceived > 0) {
                std::lock_guard<std::mutex> lock(m_mutex);
                if (m_callback) {
                    m_callback(std::span<const uint8_t>(buffer.data(), bytesReceived));
                }
            } else if (bytesReceived == SOCKET_ERROR) {
                if (m_running) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }
            }
        }
    }

    SocketType m_socket;
    std::thread m_thread;
    std::atomic<bool> m_running;
    std::mutex m_mutex;
    DataCallback m_callback;
};

std::unique_ptr<INetworkReceiver> CreateUDPReceiver() {
    return std::make_unique<UDPReceiver>();
}

} // namespace kvm::network
