#pragma once

#include <string>
#include <memory>
#include <atomic>
#include <mutex>
#include <vector>
#include <functional>
#include <thread>

struct AVFrame;
struct AVCodecContext;
struct AVPacket;

namespace rtc {
    class PeerConnection;
    class Track;
}

namespace kvm::video {

class WebRTCStreamNode {
public:
    WebRTCStreamNode();
    ~WebRTCStreamNode();

    bool Initialize();
    
    // Starts async negotiation via FastAPI signaling server
    bool OpenStream(const std::string& signalingUrl);
    
    // Fetches the latest YUV frame for SDLVideoDecoder
    bool GetLatestFrame(AVFrame* destFrame);
    
    void Flush();
    
    bool IsRunning() const noexcept { return m_running.load(); }
    bool IsConnected() const noexcept { return m_connected.load(); }
    const std::string& GetLastError() const { return m_lastError; }

private:
    void Cleanup();
    void StartSignaling(std::string signalingUrl);
    void SendIceCandidate(const std::string& signalingUrl, const std::string& candidateSdp);

    // FFmpeg context for decoding H.264 NAL units from WebRTC
    bool SetupDecoder();
    void DecodeVideoData(const uint8_t* data, size_t size);

private:
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_connected{false};
    std::string m_lastError;
    std::string m_sessionUrl;
    std::vector<std::string> m_pendingCandidates;
    std::mutex m_iceMutex;

    std::shared_ptr<rtc::PeerConnection> m_peerConnection;
    std::thread m_signalingThread;
    
    // FFmpeg Decoding
    AVCodecContext* m_codecCtx = nullptr;
    AVFrame* m_frame = nullptr;
    AVPacket* m_packet = nullptr;

    std::mutex m_frameMutex;
    AVFrame* m_sharedFrame = nullptr;
    bool m_hasNewFrame = false;
};

} // namespace kvm::video
