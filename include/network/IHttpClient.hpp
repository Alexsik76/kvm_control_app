#pragma once

#include <string>

namespace kvm::network {

class IHttpClient {
public:
    virtual ~IHttpClient() = default;
    virtual bool Post(const std::string& url, const std::string& body, std::string& outResponse, const std::string& contentType = "application/json") = 0;
};

} // namespace kvm::network
