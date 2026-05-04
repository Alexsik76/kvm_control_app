#include "KVMVideoCodec.h"
#include "Version.hpp"
#include "video/WebRTCStreamNode.hpp"
#include "network/HttpClient.hpp"
#include <string>
#include <memory>
#include <mutex>

namespace kvm::bridge {

class SessionManager {
public:
    static SessionManager& Instance() {
        static SessionManager instance;
        return instance;
    }

    int Start(const char* url, const char* token, FrameCallback callback) {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        StopInternal();

        auto httpClient = std::make_shared<kvm::network::HttpClient>();
        m_streamNode = std::make_unique<kvm::video::WebRTCStreamNode>(httpClient);
        
        if (!m_streamNode->Initialize()) {
            return -1;
        }

        m_streamNode->SetFrameCallback(callback);
        
        if (!m_streamNode->OpenStream(url, token)) {
            return -2;
        }

        return 0;
    }

    void Stop() {
        std::lock_guard<std::mutex> lock(m_mutex);
        StopInternal();
    }

private:
    void StopInternal() {
        if (m_streamNode) {
            m_streamNode->Flush();
            m_streamNode.reset();
        }
    }

    std::mutex m_mutex;
    std::unique_ptr<kvm::video::WebRTCStreamNode> m_streamNode;
};

} // namespace kvm::bridge

// --- C API Implementation ---

KVM_API int KvmInitialize(const char* url, const char* token, FrameCallback callback) {
    if (!url || !callback) return -1;
    return kvm::bridge::SessionManager::Instance().Start(url, token, callback);
}

KVM_API void KvmStop() {
    kvm::bridge::SessionManager::Instance().Stop();
}

KVM_API const char* KvmGetVersion() {
    return kvm::VERSION_GIT_HASH;
}

