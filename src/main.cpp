#include "core/CommandLineParser.hpp"
#include "core/AppBuilder.hpp"
#include "network/ILauncherClient.hpp"
#include <iostream>
#include <memory>

int main(int argc, char* argv[]) {
    // 1. Parse Command Line
    kvm::core::CommandLineParser parser(argc, argv);
    auto pipeName = parser.GetOption("--pipe");

    if (!pipeName) {
        std::cerr << "Usage: control_app --pipe <PIPE_NAME>\n";
        return 1;
    }

    // 2. Connect to Launcher
    auto launcher = kvm::network::CreateLauncherClient();
    if (!launcher->Connect(*pipeName)) {
        std::cerr << "[Main] Failed to connect to pipe: " << *pipeName << "\n";
        return 1;
    }

    // 3. Handshake
    kvm::network::HandshakeData handshake;
    if (!launcher->WaitForHandshake(handshake)) {
        launcher->SendError("Handshake failed");
        return 1;
    }

    // 4. Start Async IPC Listener
    launcher->StartAsync([](const std::string& type, const std::string& payload) {
        std::cout << "[IPC] Received: " << type << " -> " << payload << "\n";
        // Handle incoming commands from Launcher here (e.g., Close, Settings Change)
    });

    // 5. Bootstrap and Run Application
    auto app = kvm::core::AppBuilder::Build(handshake, launcher.get());
    if (app) {
        launcher->SendStatus("Application running");
        app->Run();
    } else {
        launcher->SendError("Application failed to start");
    }

    launcher->Stop();
    return 0;
}
