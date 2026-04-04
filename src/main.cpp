#include "core/KVMApplication.hpp"
#include <string>

int main(int argc, char* argv[]) {
    std::string streamUrl = "udp://127.0.0.1:5000";
    std::string hidUrl = "ws://127.0.0.1:8080";

    if (argc > 1) streamUrl = argv[1];
    if (argc > 2) hidUrl = argv[2];

    kvm::core::KVMApplication app;
    
    if (app.Initialize(streamUrl, hidUrl)) {
        app.Run();
    }

    return 0;
}
