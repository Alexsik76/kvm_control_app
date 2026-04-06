#include "core/AppConfig.hpp"
#include <yaml-cpp/yaml.h>
#include <filesystem>
#include <format>
#include <iostream>

namespace kvm::core {

bool AppConfig::Load(const std::string& filepath) {
    if (!std::filesystem::exists(filepath)) return false;

    try {
        YAML::Node config = YAML::LoadFile(filepath);
        if (config["server"]) {
            auto srv = config["server"];
            if (srv["host"]) m_globalHost = srv["host"].as<std::string>();
            if (srv["node_id"]) m_nodeId = srv["node_id"].as<std::string>();

            if (srv["auth"]) {
                auto auth = srv["auth"];
                if (auth["login_url"]) m_loginUrlOverride = auth["login_url"].as<std::string>();
                if (auth["username"]) m_username = auth["username"].as<std::string>();
                if (auth["password"]) m_password = auth["password"].as<std::string>();
            }

            if (srv["video"]) {
                auto v = srv["video"];
                if (v["host"]) m_videoHost = v["host"].as<std::string>();
                if (v["path"]) m_videoPath = v["path"].as<std::string>();
                if (v["token"]) m_videoTokenOverride = v["token"].as<std::string>();
            }

            if (srv["hid"]) {
                auto h = srv["hid"];
                if (h["host"]) m_hidHost = h["host"].as<std::string>();
                if (h["path"]) m_hidPath = h["path"].as<std::string>();
                if (h["token"]) m_hidTokenOverride = h["token"].as<std::string>();
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "[Config] YAML error: " << e.what() << "\n";
        return false;
    }
    return true;
}

std::string AppConfig::GetLoginUrl() const {
    if (!m_loginUrlOverride.empty()) return m_loginUrlOverride;
    return std::format("https://{}/api/v1/auth/login", m_globalHost);
}

std::string AppConfig::GetStreamUrl(const std::string& token) const {
    std::string host = m_videoHost.empty() ? m_globalHost : m_videoHost;
    std::string t = token.empty() ? m_videoTokenOverride : token;
    return BuildUrl("https", host, m_videoPath, m_nodeId, t);
}

std::string AppConfig::GetHidUrl(const std::string& token) const {
    std::string host = m_hidHost.empty() ? m_globalHost : m_hidHost;
    std::string t = token.empty() ? m_hidTokenOverride : token;
    return BuildUrl("wss", host, m_hidPath, m_nodeId, t);
}

std::string AppConfig::ReplacePlaceholder(std::string str, const std::string& placeholder, const std::string& value) const {
    size_t pos = 0;
    while ((pos = str.find(placeholder, pos)) != std::string::npos) {
        str.replace(pos, placeholder.length(), value);
        pos += value.length();
    }
    return str;
}

std::string AppConfig::BuildUrl(const std::string& protocol, 
                                 const std::string& host, 
                                 const std::string& pathTemplate, 
                                 const std::string& nodeId, 
                                 const std::string& token) const {
    std::string path = ReplacePlaceholder(pathTemplate, "${node_id}", nodeId);
    std::string url = std::format("{}://{}{}", protocol, host, path);
    if (!token.empty()) {
        url += (url.find('?') == std::string::npos ? "?" : "&");
        url += "token=" + token;
    }
    return url;
}

} // namespace kvm::core
