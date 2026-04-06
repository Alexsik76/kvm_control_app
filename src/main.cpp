#include "core/KVMApplication.hpp"
#include <yaml-cpp/yaml.h>
#include <iostream>
#include <string>
#include <filesystem>
#include <format>

int main() {
    // Default values (Fallback)
    std::string streamUrl = "udp://127.0.0.1:5000";
    std::string hidUrl = "ws://127.0.0.1:8080/ws/control";
    std::string configPath = "config.yaml";

    try {
        if (std::filesystem::exists(configPath)) {
            YAML::Node config = YAML::LoadFile(configPath);
            if (config["server"]) {
                auto srv = config["server"];
                std::string globalHost = srv["host"] ? srv["host"].as<std::string>() : "127.0.0.1";

                // --- Video URL Construction ---
                if (srv["video"]) {
                    auto v = srv["video"];
                    std::string proto = v["proto"] ? v["proto"].as<std::string>() : "webrtc";
                    std::string host = v["host"] ? v["host"].as<std::string>() : globalHost;
                    int port = v["port"] ? v["port"].as<int>() : 443;
                    std::string path = v["path"] ? v["path"].as<std::string>() : "";
                    
                    if (proto == "webrtc") {
                        std::string httpProto = (port == 443) ? "https" : "http";
                        if (port == 443 || port == 80)
                            streamUrl = std::format("{}://{}{}", httpProto, host, path);
                        else
                            streamUrl = std::format("{}://{}:{}{}", httpProto, host, port, path);
                    } else if (v["user"] && v["pass"]) {
                        streamUrl = std::format("{}://{}:{}@{}:{}{}", 
                            proto, v["user"].as<std::string>(), v["pass"].as<std::string>(), host, port, path);
                    } else {
                        if ((proto == "rtsp" && port == 554) || (proto == "http" && port == 80) || (proto == "https" && port == 443))
                            streamUrl = std::format("{}://{}{}", proto, host, path);
                        else
                            streamUrl = std::format("{}://{}:{}{}", proto, host, port, path);
                    }
                }

                // --- HID URL Construction ---
                if (srv["hid"]) {
                    auto h = srv["hid"];
                    std::string proto = h["proto"] ? h["proto"].as<std::string>() : "wss";
                    std::string host = h["host"] ? h["host"].as<std::string>() : globalHost;
                    int port = h["port"] ? h["port"].as<int>() : 443;
                    std::string path = h["path"] ? h["path"].as<std::string>() : "/ws/control";
                    
                    if ((proto == "wss" && port == 443) || (proto == "ws" && port == 80))
                        hidUrl = std::format("{}://{}{}", proto, host, path);
                    else
                        hidUrl = std::format("{}://{}:{}{}", proto, host, port, path);
                }

                std::cout << "[Config] Successfully loaded and constructed URLs.\n";
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "[Config] Error: " << e.what() << ". Falling back to defaults.\n";
    }

    std::cout << "[App] Video URL: " << streamUrl << "\n";
    std::cout << "[App] HID URL:   " << hidUrl << "\n";

    kvm::core::KVMApplication app;
    
    if (app.Initialize(streamUrl, hidUrl)) {
        app.Run();
    }

    return 0;
}
