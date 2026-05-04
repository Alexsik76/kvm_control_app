#pragma once

#include "network/IHttpClient.hpp"
#include <string>

namespace kvm::network {

class HttpClient : public IHttpClient {
public:
    HttpClient() = default;
    ~HttpClient() override = default;

    bool Post(const std::string& url, const std::string& body, std::string& outResponse, const std::string& contentType = "application/json") override;
    void SetAccessToken(const std::string& token) override { m_accessToken = token; }

private:
    std::string m_accessToken;
};

} // namespace kvm::network
