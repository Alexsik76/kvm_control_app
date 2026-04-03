#include "network/INetworkReceiver.hpp"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <thread>
#include <atomic>
#include <iostream>
#include <vector>

namespace kvm::network {

class UDPReceiver : public INetworkReceiver {
public:
    UDPReceiver() : m_socket(INVALID_SOCKET), m_running(false) {
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);
    }

    ~UDPReceiver() override {
        Stop();
        WSACleanup();
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

        if (bind(m_socket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
            closesocket(m_socket);
            return false;
        }

        m_running = true;
        m_thread = std::thread(&UDPReceiver::ReceiveLoop, this);
        return true;
    }

    void Stop() override {
        m_running = false;
        if (m_socket != INVALID_SOCKET) {
            closesocket(m_socket);
            m_socket = INVALID_SOCKET;
        }
        if (m_thread.joinable()) {
            m_thread.join();
        }
    }

    void SetCallback(DataCallback callback) override {
        m_callback = std::move(callback);
    }

private:
    void ReceiveLoop() {
        std::vector<uint8_t> buffer(65507); // Max UDP packet size
        while (m_running) {
            int bytesReceived = recv(m_socket, (char*)buffer.data(), (int)buffer.size(), 0);
            if (bytesReceived > 0 && m_callback) {
                m_callback(std::span<const uint8_t>(buffer.data(), bytesReceived));
            } else if (bytesReceived == SOCKET_ERROR) {
                if (m_running) std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }
    }

    SOCKET m_socket;
    std::thread m_thread;
    std::atomic<bool> m_running;
    DataCallback m_callback;
};

std::unique_ptr<INetworkReceiver> CreateUDPReceiver() {
    return std::make_unique<UDPReceiver>();
}

} // namespace kvm::network
