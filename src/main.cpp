#include "core/KVMApplication.hpp"
#include "core/AppConfig.hpp"
#include "network/WinHttpClient.hpp"
#include "video/IVideoDecoder.hpp"
#include "control/IInputCapturer.hpp"
#include "control/IHIDClient.hpp"
#include "control/IEventMapper.hpp"
#include <nlohmann/json.hpp>
#include <iostream>
#include <memory>
#include <format>

using json = nlohmann::json;

int main() {
    // 1. Configuration
    kvm::core::AppConfig config;
    if (!config.Load("config.yaml")) {
        std::cerr << "[Main] Failed to load config.yaml\n";
        return 1;
    }

    // 2. Dependencies
    auto httpClient = std::make_shared<kvm::network::WinHttpClient>();
    std::string token = "";

    // 3. Authentication (Composition Root logic)
    std::string username = config.GetUsername();
    std::string password = config.GetPassword();
    if (!username.empty() && !password.empty()) {
        std::cout << "[Auth] Attempting login for: " << username << "\n";
        std::string loginBody = std::format("username={}&password={}", username, password);
        std::string response;
        if (httpClient->Post(config.GetLoginUrl(), loginBody, response, "application/x-www-form-urlencoded")) {
            try {
                auto resJson = json::parse(response);
                if (resJson.contains("access_token")) {
                    token = resJson["access_token"].get<std::string>();
                    std::cout << "[Auth] Login successful.\n";
                }
            } catch (...) {
                std::cerr << "[Auth] Failed to parse response.\n";
            }
        } else {
            std::cerr << "[Auth] Login request failed.\n";
        }
    }

    // 4. Final URLs
    std::string streamUrl = config.GetStreamUrl(token);
    std::string hidUrl = config.GetHidUrl(token);

    std::cout << "[App] Video URL: " << streamUrl << "\n";
    std::cout << "[App] HID URL:   " << hidUrl << "\n";

    // 5. Instantiate App and Inject Dependencies
    auto hidModule = kvm::control::CreateHIDClient();
    auto eventMapper = kvm::control::CreateEventMapper();
    auto videoModule = kvm::video::CreateVideoDecoder(nullptr, httpClient);
    auto inputCapturer = kvm::control::CreateInputCapturer(nullptr);

    kvm::core::KVMApplication app;
    if (app.Initialize(streamUrl, hidUrl, std::move(videoModule), std::move(inputCapturer), std::move(hidModule), std::move(eventMapper))) {
        app.Run();
    }

    return 0;
}