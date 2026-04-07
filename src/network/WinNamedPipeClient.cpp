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
        // For the initial handshake, we can afford a short blocking read or repeated peeks
        for (int i = 0; i < 50; ++i) { // 5 seconds timeout
            std::string message = ReadLine();
            if (!message.empty()) {
                try {
                    auto j = json::parse(message);
                    if (j.contains("Type") && j["Type"] == "Handshake") {
                        auto p = j["Payload"];
                        outData.accessToken = p.contains("AccessToken") ? p["AccessToken"] : "";
                        outData.streamUrl = p.contains("StreamUrl") ? p["StreamUrl"] : "";
                        outData.hidUrl = p.contains("HidUrl") ? p["HidUrl"] : "";
                        return true;
                    }
                } catch (...) {}
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        return false;
    }

    void StartAsync(std::function<void(const std::string& type, const std::string& payload)> onMessage) override {
        if (m_running) return;
        m_running = true;
        m_worker = std::thread([this, onMessage]() {
            while (m_running) {
                std::string line = ReadLine();
                if (line.empty()) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(50));
                    continue;
                }
                try {
                    auto j = json::parse(line);
                    std::string type = j.contains("Type") ? j["Type"] : "";
                    std::string payload = j.contains("Payload") ? 
                        (j["Payload"].is_string() ? j["Payload"].get<std::string>() : j["Payload"].dump()) : "";
                    onMessage(type, payload);
                } catch (...) {}
            }
        });
    }

    void Stop() override {
        m_running = false;
        if (m_worker.joinable()) {
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
        if (!WriteFile(m_hPipe, s.c_str(), static_cast<DWORD>(s.length()), &written, NULL)) {
            // Handle error if needed
        }
    }

    std::string ReadLine() {
        if (m_hPipe == INVALID_HANDLE_VALUE) return "";

        DWORD available = 0;
        if (!PeekNamedPipe(m_hPipe, NULL, 0, NULL, &available, NULL) || available == 0) {
            return "";
        }

        std::vector<char> buffer;
        char ch;
        DWORD bytesRead;

        // Now that we know data is available, we can read. 
        // We still read byte-by-byte to handle the newline termination properly.
        while (available > 0 && ReadFile(m_hPipe, &ch, 1, &bytesRead, NULL) && bytesRead > 0) {
            available--;
            if (ch == '\n') break;
            buffer.push_back(ch);
        }
        
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
