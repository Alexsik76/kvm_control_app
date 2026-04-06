#pragma once

#include "network/IHttpClient.hpp"
#include <string>

namespace kvm::network {

class WinHttpClient : public IHttpClient {
public:
    WinHttpClient() = default;
    ~WinHttpClient() override = default;

    bool Post(const std::string& url, const std::string& body, std::string& outResponse, const std::string& contentType = "application/json") override;
};

} // namespace kvm::network
