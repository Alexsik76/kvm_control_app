#pragma once
#include <string>

namespace kvm::core {

/**
 * @brief Handles application configuration parsing from YAML.
 * Follows SRP by focusing only on configuration data management.
 */
class AppConfig {
public:
    AppConfig() = default;

    /**
     * @brief Loads configuration from YAML file.
     * @param filepath Path to the YAML configuration file.
     * @return true if loading succeeded, false otherwise.
     */
    bool Load(const std::string& filepath);

    // Auth configuration getters
    std::string GetLoginUrl() const;
    std::string GetUsername() const { return m_username; }
    std::string GetPassword() const { return m_password; }

    /**
     * @brief Constructs the WebRTC signaling URL using an externally provided token.
     */
    std::string GetStreamUrl(const std::string& token) const;

    /**
     * @brief Constructs the HID (WebSocket) URL using an externally provided token.
     */
    std::string GetHidUrl(const std::string& token) const;

private:
    std::string ReplacePlaceholder(std::string str, const std::string& placeholder, const std::string& value) const;
    std::string BuildUrl(const std::string& protocol, 
                         const std::string& host, 
                         const std::string& pathTemplate, 
                         const std::string& nodeId, 
                         const std::string& token) const;

private:
    std::string m_globalHost = "kvm-api.lab.vn.ua";
    std::string m_nodeId = "cab2e9b3-318e-43d4-96e9-c9511ac3b889";
    
    // Auth data
    std::string m_loginUrlOverride;
    std::string m_username;
    std::string m_password;

    // Video data
    std::string m_videoHost;
    std::string m_videoPath = "/api/v1/nodes/${node_id}/signal/offer";
    std::string m_videoTokenOverride;

    // HID data
    std::string m_hidHost;
    std::string m_hidPath = "/api/v1/nodes/${node_id}/ws";
    std::string m_hidTokenOverride;
};

} // namespace kvm::core
