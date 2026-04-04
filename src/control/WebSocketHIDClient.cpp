#include "control/IHIDClient.hpp"
#include <ixwebsocket/IXWebSocket.h>
#include <iostream>
#include <memory>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <string>

namespace kvm::control {

class WebSocketHIDClient : public IHIDClient {
public:
    WebSocketHIDClient() : m_running(true) {
        m_webSocket.setOnMessageCallback([this](const ix::WebSocketMessagePtr& msg) {
            if (msg->type == ix::WebSocketMessageType::Open) {
                std::cout << "[HID] Connected to " << m_url << "\n";
                m_connected = true;
            } else if (msg->type == ix::WebSocketMessageType::Close) {
                std::cout << "[HID] Disconnected\n";
                m_connected = false;
            } else if (msg->type == ix::WebSocketMessageType::Error) {
                std::cerr << "[HID] Error: " << msg->errorInfo.reason << "\n";
            }
        });
        
        m_workerThread = std::thread(&WebSocketHIDClient::WorkerLoop, this);
    }

    ~WebSocketHIDClient() {
        m_running = false;
        m_cv.notify_all();
        if (m_workerThread.joinable()) {
            m_workerThread.join();
        }
        m_webSocket.stop();
    }

    bool Connect(const std::string& url) override {
        m_url = url;
        m_webSocket.setUrl(url);
        m_webSocket.start();
        return true;
    }

    void SendKeyboardEvent(const common::KeyboardEvent& event) override {
        if (!m_connected) return;

        std::string keysStr = "[";
        for (size_t i = 0; i < event.keys.size(); ++i) {
            keysStr += std::to_string(event.keys[i]);
            if (i < event.keys.size() - 1) {
                keysStr += ",";
            }
        }
        keysStr += "]";

        char buffer[256];
        int len = snprintf(buffer, sizeof(buffer),
            "{\"type\":\"keyboard\",\"modifiers\":%u,\"keys\":%s}",
            event.modifiers, keysStr.c_str());

        if (len > 0) {
            EnqueueMessage(std::string(buffer, len));
        }
    }

    void SendMouseEvent(const common::MouseEvent& event) override {
        if (!m_connected) return;

        char buffer[128];
        int len = snprintf(buffer, sizeof(buffer),
            "{\"type\":\"mouse\",\"buttons\":%u,\"x\":%d,\"y\":%d,\"wheel\":%d}",
            event.buttons, event.deltaX, event.deltaY, event.wheel);
        
        if (len > 0) {
            EnqueueMessage(std::string(buffer, len));
        }
    }

    bool IsConnected() const noexcept override { return m_connected; }

private:
    void EnqueueMessage(std::string msg) {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_queue.emplace(std::move(msg));
        }
        m_cv.notify_one();
    }

    void WorkerLoop() {
        while (m_running) {
            std::string msg;
            {
                std::unique_lock<std::mutex> lock(m_mutex);
                m_cv.wait(lock, [this] { return !m_queue.empty() || !m_running; });
                
                if (!m_running && m_queue.empty()) {
                    break;
                }
                
                msg = std::move(m_queue.front());
                m_queue.pop();
            }
            
            if (m_connected) {
                m_webSocket.send(msg);
            }
        }
    }

    ix::WebSocket m_webSocket;
    std::string m_url;
    std::atomic<bool> m_connected{false};
    
    std::atomic<bool> m_running{false};
    std::thread m_workerThread;
    std::queue<std::string> m_queue;
    std::mutex m_mutex;
    std::condition_variable m_cv;
};

/**
 * Update factory to return the real WebSocket implementation.
 */
std::unique_ptr<IHIDClient> CreateHIDClient() {
    return std::make_unique<WebSocketHIDClient>();
}

} // namespace kvm::control
