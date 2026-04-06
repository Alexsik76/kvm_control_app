#include "core/KVMApplication.hpp"
#include "network/WinHttpClient.hpp"
#include <yaml-cpp/yaml.h>
#include <nlohmann/json.hpp>
#include <iostream>
#include <string>
#include <filesystem>
#include <format>

using json = nlohmann::json;

static std::string ReplacePlaceholder(std::string str, const std::string& placeholder, const std::string& value) {
    size_t pos = 0;
    while ((pos = str.find(placeholder, pos)) != std::string::npos) {
        str.replace(pos, placeholder.length(), value);
        pos += value.length();
    }
    return str;
}

int main() {
    std::string streamUrl = "";
    std::string hidUrl = "";
    std::string configPath = "config.yaml";
    std::string token = "";

    try {
        if (std::filesystem::exists(configPath)) {
            YAML::Node config = YAML::LoadFile(configPath);
            if (config["server"]) {
                auto srv = config["server"];
                std::string globalHost = srv["host"] ? srv["host"].as<std::string>() : "kvm-api.lab.vn.ua";
                std::string node_id = srv["node_id"] ? srv["node_id"].as<std::string>() : "cab2e9b3-318e-43d4-96e9-c9511ac3b889";

                // --- Automatic Authentication ---
                if (srv["auth"]) {
                    auto auth = srv["auth"];
                    std::string loginUrl = auth["login_url"] ? auth["login_url"].as<std::string>() : std::format("https://{}/api/v1/auth/login", globalHost);
                    std::string username = auth["username"] ? auth["username"].as<std::string>() : "";
                    std::string password = auth["password"] ? auth["password"].as<std::string>() : "";

                    if (!username.empty() && !password.empty()) {
                        std::cout << "[Auth] Attempting login for user: " << username << "\n";
                        kvm::network::WinHttpClient httpClient;
                        std::string loginBody = std::format("username={}&password={}", username, password);
                        std::string response;
                        if (httpClient.Post(loginUrl, loginBody, response, "application/x-www-form-urlencoded")) {
                            try {
                                auto resJson = json::parse(response);
                                if (resJson.contains("access_token")) {
                                    token = resJson["access_token"].get<std::string>();
                                    std::cout << "[Auth] Login successful, token acquired.\n";
                                } else {
                                    std::cerr << "[Auth] Login response missing access_token.\n";
                                    return 1;
                                }
                            } catch (const std::exception& e) {
                                std::cerr << "[Auth] Failed to parse login response: " << e.what() << "\n";
                                std::cerr << "[Auth] Raw response: " << response << "\n";
                                return 1;
                            }
                        } else {
                            std::cerr << "[Auth] Login request failed.\n";
                            return 1;
                        }
                    }
                }

                if (srv["video"]) {
                    auto v = srv["video"];
                    std::string host = v["host"] ? v["host"].as<std::string>() : globalHost;
                    std::string path = v["path"] ? v["path"].as<std::string>() : "/api/v1/nodes/${node_id}/signal/offer";
                    path = ReplacePlaceholder(path, "${node_id}", node_id);
                    streamUrl = std::format("https://{}{}", host, path);
                    
                    std::string videoToken = token;
                    if (videoToken.empty() && v["token"]) videoToken = v["token"].as<std::string>();
                    if (!videoToken.empty()) streamUrl += std::format("?token={}", videoToken);
                }

                if (srv["hid"]) {
                    auto h = srv["hid"];
                    std::string host = h["host"] ? h["host"].as<std::string>() : globalHost;
                    std::string path = h["path"] ? h["path"].as<std::string>() : "/api/v1/nodes/${node_id}/ws";
                    path = ReplacePlaceholder(path, "${node_id}", node_id);
                    hidUrl = std::format("wss://{}{}", host, path);
                    
                    std::string hidToken = token;
                    if (hidToken.empty() && srv["video"]["token"]) hidToken = srv["video"]["token"].as<std::string>();
                    if (!hidToken.empty()) hidUrl += std::format("?token={}", hidToken);
                }
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "[Config] Error: " << e.what() << "\n";
    }

    if (streamUrl.empty()) streamUrl = "https://kvm-api.lab.vn.ua/api/v1/nodes/cab2e9b3-318e-43d4-96e9-c9511ac3b889/signal/offer";
    if (hidUrl.empty()) hidUrl = "wss://kvm-api.lab.vn.ua/api/v1/nodes/cab2e9b3-318e-43d4-96e9-c9511ac3b889/ws";

    std::cout << "[App] Video URL: " << streamUrl << "\n";
    std::cout << "[App] HID URL:   " << hidUrl << "\n";

    kvm::core::KVMApplication app;
    if (app.Initialize(streamUrl, hidUrl)) {
        app.Run();
    }

    return 0;
}
