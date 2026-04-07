#pragma once
#include "core/KVMApplication.hpp"
#include "network/ILauncherClient.hpp"
#include "network/WinHttpClient.hpp"
#include "video/IVideoDecoder.hpp"
#include "control/IInputCapturer.hpp"
#include "control/IHIDClient.hpp"
#include "control/IEventMapper.hpp"
#include <memory>
#include <string>

namespace kvm::core {

class AppBuilder {
public:
    static std::unique_ptr<KVMApplication> Build(const network::HandshakeData& handshake, 
                                               network::ILauncherClient* launcher) {
        auto app = std::make_unique<KVMApplication>();
        
        auto httpClient = std::make_shared<network::WinHttpClient>();
        httpClient->SetAccessToken(handshake.accessToken);
        
        auto hidModule = control::CreateHIDClient();
        
        std::string authorizedHidUrl = handshake.hidUrl;
        if (!handshake.accessToken.empty()) {
            authorizedHidUrl += (authorizedHidUrl.find('?') == std::string::npos ? "?" : "&");
            authorizedHidUrl += "token=" + handshake.accessToken;
        }

        auto eventMapper = control::CreateEventMapper();
        auto videoModule = video::CreateVideoDecoder(nullptr, httpClient);
        auto inputCapturer = control::CreateInputCapturer(nullptr);

        if (!app->Initialize(handshake.streamUrl, authorizedHidUrl, 
                           std::move(videoModule), std::move(inputCapturer), 
                           std::move(hidModule), std::move(eventMapper), 
                           launcher)) {
            return nullptr;
        }

        return app;
    }
};

} // namespace kvm::core
