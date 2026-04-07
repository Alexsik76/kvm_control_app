#include "network/ILauncherClient.hpp"
#include <windows.h>
#include <nlohmann/json.hpp>
#include <iostream>
#include <format>
#include <vector>
#include <thread>
#include <atomic>

using json = nlohmann::json;

namespace kvm::network {

class WinNamedPipeClient : public ILauncherClient {
public:
    WinNamedPipeClient() : m_hPipe(INVALID_HANDLE_VALUE), m_running(false) {}
    ~WinNamedPipeClient() override { 
        Stop();
        Close(); 
    }

    bool Connect(const std::string& pipeName) override {
        // ... (previous implementation remains same)
        std::string fullPath = std::format("\\\\.\\pipe\\{}", pipeName);
        
        while (true) {
            m_hPipe = CreateFileA(
                fullPath.c_str(),
                GENERIC_READ | GENERIC_WRITE,
                0, NULL, OPEN_EXISTING, 0, NULL);

            if (m_hPipe != INVALID_HANDLE_VALUE) break;

            if (GetLastError() != ERROR_PIPE_BUSY) {
                return false;
            }

            if (!WaitNamedPipeA(fullPath.c_str(), 2000)) {
                return false;
            }
        }

        DWORD mode = PIPE_READMODE_MESSAGE;
        SetNamedPipeHandleState(m_hPipe, &mode, NULL, NULL);
        return true;
    }

    bool WaitForHandshake(HandshakeData& outData) override {
        std::string message = ReadLine();
        if (message.empty()) return false;

        try {
            auto j = json::parse(message);
            if (j["Type"] == "Handshake") {
                auto p = j["Payload"];
                outData.accessToken = p["AccessToken"];
                outData.streamUrl = p["StreamUrl"];
                outData.hidUrl = p["HidUrl"];
                return true;
            }
        } catch (...) {}
        return false;
    }

    void StartAsync(std::function<void(const std::string& type, const std::string& payload)> onMessage) override {
        if (m_running) return;
        m_running = true;
        m_worker = std::thread([this, onMessage]() {
            while (m_running) {
                std::string line = ReadLine();
                if (line.empty()) {
                    if (m_running) std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    continue;
                }
                try {
                    auto j = json::parse(line);
                    onMessage(j["Type"], j["Payload"].is_string() ? j["Payload"].get<std::string>() : j["Payload"].dump());
                } catch (...) {}
            }
        });
    }

    void Stop() override {
        m_running = false;
        if (m_worker.joinable()) {
            // Cancel pending I/O if necessary, or just wait for timeout/disconnect
            m_worker.join();
        }
    }

    void SendStatus(const std::string& message) override {
        SendMessage("StatusUpdate", message);
    }

    void SendError(const std::string& message) override {
        SendMessage("Error", message);
    }

    void Close() override {
        if (m_hPipe != INVALID_HANDLE_VALUE) {
            CloseHandle(m_hPipe);
            m_hPipe = INVALID_HANDLE_VALUE;
        }
    }

private:
    void SendMessage(const std::string& type, const std::string& payload) {
        if (m_hPipe == INVALID_HANDLE_VALUE) return;

        json j = {{"Type", type}, {"Payload", payload}};
        std::string s = j.dump() + "\n";
        
        DWORD written;
        WriteFile(m_hPipe, s.c_str(), static_cast<DWORD>(s.length()), &written, NULL);
    }

    std::string ReadLine() {
        if (m_hPipe == INVALID_HANDLE_VALUE) return "";
        std::vector<char> buffer;
        char ch;
        DWORD bytesRead;

        // Use PeekNamedPipe to avoid blocking indefinitely if possible, 
        // but for simplicity, we use blocking ReadFile in the background thread.
        while (ReadFile(m_hPipe, &ch, 1, &bytesRead, NULL) && bytesRead > 0) {
            if (ch == '\n') break;
            buffer.push_back(ch);
        }
        
        if (buffer.empty() && bytesRead == 0) return "";
        return std::string(buffer.begin(), buffer.end());
    }

private:
    HANDLE m_hPipe;
    std::thread m_worker;
    std::atomic<bool> m_running;
};

std::unique_ptr<ILauncherClient> CreateLauncherClient() {
    return std::make_unique<WinNamedPipeClient>();
}

} // namespace kvm::network
