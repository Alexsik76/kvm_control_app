#include "core/KVMApplication.hpp"
#include "network/WinHttpClient.hpp"
#include "network/ILauncherClient.hpp"
#include "video/IVideoDecoder.hpp"
#include "control/IInputCapturer.hpp"
#include "control/IHIDClient.hpp"
#include "control/IEventMapper.hpp"
#include <iostream>
#include <memory>
#include <string>
#include <vector>

int main(int argc, char* argv[]) {
    std::string pipeName;

    // 1. Parse Arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--pipe" && i + 1 < argc) {
            pipeName = argv[++i];
        }
    }

    if (pipeName.empty()) {
        std::cerr << "Usage: control_app --pipe <PIPE_NAME>\n";
        return 1;
    }

    // 2. Connect to Launcher
    auto launcher = kvm::network::CreateLauncherClient();
    std::cout << "[Main] Connecting to pipe: " << pipeName << "...\n";
    if (!launcher->Connect(pipeName)) {
        std::cerr << "[Main] Failed to connect to pipe.\n";
        return 1;
    }

    // 3. Handshake
    kvm::network::HandshakeData handshake;
    std::cout << "[Main] Waiting for handshake...\n";
    if (!launcher->WaitForHandshake(handshake)) {
        std::cerr << "[Main] Handshake failed.\n";
        launcher->SendError("Handshake failed");
        return 1;
    }

    std::cout << "[Main] Received Handshake. Stream: " << handshake.streamUrl << "\n";
    launcher->SendStatus("Handshake successful. Initializing modules...");

    // 4. Dependencies
    auto httpClient = std::make_shared<kvm::network::WinHttpClient>();
    auto hidModule = kvm::control::CreateHIDClient();
    auto eventMapper = kvm::control::CreateEventMapper();
    auto videoModule = kvm::video::CreateVideoDecoder(nullptr, httpClient);
    auto inputCapturer = kvm::control::CreateInputCapturer(nullptr);

    // 5. Run Application
    kvm::core::KVMApplication app;
    if (app.Initialize(handshake.streamUrl, handshake.hidUrl, 
                      std::move(videoModule), std::move(inputCapturer), 
                      std::move(hidModule), std::move(eventMapper),
                      launcher.get())) {
        launcher->SendStatus("Application running");
        app.Run();
    } else {
        launcher->SendError("Application initialization failed");
    }

    return 0;
}
