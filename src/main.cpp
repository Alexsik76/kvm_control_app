#include "core/KVMApplication.hpp"
#include <yaml-cpp/yaml.h>
#include <iostream>
#include <string>
#include <filesystem>
#include <format>

int main() {
    std::string streamUrl = "";
    std::string hidUrl = "";
    std::string configPath = "config.yaml";

    try {
        if (std::filesystem::exists(configPath)) {
            YAML::Node config = YAML::LoadFile(configPath);
            if (config["server"]) {
                auto srv = config["server"];
                std::string globalHost = srv["host"] ? srv["host"].as<std::string>() : "kvm-api.lab.vn.ua";
                std::string node_id = srv["node_id"] ? srv["node_id"].as<std::string>() : "cab2e9b3-318e-43d4-96e9-c9511ac3b889";

                if (srv["video"]) {
                    auto v = srv["video"];
                    std::string host = v["host"] ? v["host"].as<std::string>() : globalHost;
                    std::string path = v["path"] ? v["path"].as<std::string>() : "";
                    streamUrl = std::format("https://{}{}", host, path);
                    if (v["token"]) streamUrl += std::format("?token={}", v["token"].as<std::string>());
                }

                if (srv["hid"]) {
                    auto h = srv["hid"];
                    std::string host = h["host"] ? h["host"].as<std::string>() : globalHost;
                    std::string path = h["path"] ? h["path"].as<std::string>() : std::format("/api/v1/nodes/{}/ws", node_id);
                    hidUrl = std::format("wss://{}{}", host, path);
                    if (srv["video"]["token"]) {
                         hidUrl += std::format("?token={}", srv["video"]["token"].as<std::string>());
                    }
                }
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "[Config] Error: " << e.what() << "\n";
    }

    if (streamUrl.empty()) streamUrl = std::format("https://kvm-api.lab.vn.ua/api/v1/nodes/cab2e9b3-318e-43d4-96e9-c9511ac3b889/signal/offer");
    if (hidUrl.empty()) hidUrl = "wss://kvm-api.lab.vn.ua/api/v1/nodes/cab2e9b3-318e-43d4-96e9-c9511ac3b889/ws";

    std::cout << "[App] Video URL: " << streamUrl << "\n";
    std::cout << "[App] HID URL:   " << hidUrl << "\n";

    kvm::core::KVMApplication app;
    if (app.Initialize(streamUrl, hidUrl)) {
        app.Run();
    }

    return 0;
}
