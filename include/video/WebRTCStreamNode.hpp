#pragma once

#include "network/IHttpClient.hpp"
#include "video/RtpDepacketizer.hpp"
#include <string>
#include <memory>
#include <atomic>
#include <mutex>
#include <vector>
#include <functional>
#include <thread>
#include <queue>
#include <condition_variable>

struct AVFrame;
struct AVCodecContext;
struct AVPacket;
struct SwsContext;

namespace rtc {
    class PeerConnection;
    class Track;
}

namespace kvm::video {

typedef void (*FrameCallback)(uint8_t* data, int width, int height, int stride);

class WebRTCStreamNode {
public:
    explicit WebRTCStreamNode(std::shared_ptr<network::IHttpClient> httpClient);
    ~WebRTCStreamNode();

    bool Initialize();
    
    // Starts async negotiation via FastAPI signaling server
    bool OpenStream(const std::string& signalingUrl, const std::string& token = "");
    
    // Set callback for receiving decoded frames
    void SetFrameCallback(FrameCallback callback) { m_frameCallback = callback; }
    
    // Fetches the latest YUV frame (legacy, kept for internal use if needed)
    bool GetLatestFrame(AVFrame* destFrame);
    
    void Flush();
    
    bool IsRunning() const noexcept { return m_running.load(); }
    bool IsConnected() const noexcept { return m_connected.load(); }
    const std::string& GetLastError() const { return m_lastError; }

private:
    void Cleanup();
    void StartSignaling(std::string signalingUrl, std::string token);
    void SendIceCandidate(const std::string& signalingUrl, const std::string& candidateSdp);
    void DecodeLoop();

    // FFmpeg context for decoding H.264 NAL units from WebRTC
    bool SetupDecoder();
    void DecodeVideoData(const uint8_t* data, size_t size);
    void ProcessFrame(AVFrame* frame);

private:
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_connected{false};
    std::string m_lastError;
    std::string m_sessionUrl;
    std::vector<std::string> m_pendingCandidates;
    std::mutex m_iceMutex;

    std::shared_ptr<rtc::PeerConnection> m_peerConnection;
    std::shared_ptr<rtc::Track> m_videoTrack;
    std::thread m_signalingThread;
    
    // FFmpeg Decoding
    AVCodecContext* m_codecCtx = nullptr;
    AVFrame* m_frame = nullptr;
    AVPacket* m_packet = nullptr;

    std::mutex m_frameMutex;
    AVFrame* m_sharedFrame = nullptr;
    bool m_hasNewFrame = false;

    // Conversion to BGRA
    SwsContext* m_swsContext = nullptr;
    AVFrame* m_bgraFrame = nullptr;
    FrameCallback m_frameCallback = nullptr;

    // Producer-Consumer for decoding
    std::queue<std::vector<uint8_t>> m_packetQueue;
    std::mutex m_queueMutex;
    std::condition_variable m_queueCondVar;
    std::thread m_decodeWorker;

    std::shared_ptr<network::IHttpClient> m_httpClient;
    RtpDepacketizer m_depacketizer;
};

} // namespace kvm::video
