#include "video/WebRTCStreamNode.hpp"
#include "video/RtpDepacketizer.hpp"
#include <rtc/rtc.hpp>
#include <nlohmann/json.hpp>
#include <iostream>
#include <chrono>
#include <vector>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
}

using json = nlohmann::json;

namespace kvm::video {

WebRTCStreamNode::WebRTCStreamNode(std::shared_ptr<network::IHttpClient> httpClient) 
    : m_httpClient(std::move(httpClient)) {}

WebRTCStreamNode::~WebRTCStreamNode() { Cleanup(); }

bool WebRTCStreamNode::Initialize() {
    Cleanup();
    rtc::InitLogger(rtc::LogLevel::Warning);
    if (!m_frame) m_frame = av_frame_alloc();
    if (!m_packet) m_packet = av_packet_alloc();
    if (!m_sharedFrame) m_sharedFrame = av_frame_alloc();
    return m_frame && m_packet && m_sharedFrame && SetupDecoder();
}

bool WebRTCStreamNode::SetupDecoder() {
    const AVCodec* codec = avcodec_find_decoder(AV_CODEC_ID_H264);
    if (!codec) return false;
    m_codecCtx = avcodec_alloc_context3(codec);
    if (!m_codecCtx) return false;
    m_codecCtx->flags |= AV_CODEC_FLAG_LOW_DELAY;
    m_codecCtx->flags2 |= AV_CODEC_FLAG2_CHUNKS;
    m_codecCtx->thread_count = 4;
    if (avcodec_open2(m_codecCtx, codec, nullptr) < 0) {
        avcodec_free_context(&m_codecCtx);
        return false;
    }
    return true;
}

bool WebRTCStreamNode::OpenStream(const std::string& signalingUrl) {
    if (m_running) return false;
    m_running = true;
    m_decodeWorker = std::thread(&WebRTCStreamNode::DecodeLoop, this);
    m_signalingThread = std::thread(&WebRTCStreamNode::StartSignaling, this, signalingUrl);
    return true;
}

void WebRTCStreamNode::DecodeVideoData(const uint8_t* data, size_t size) {
    if (!m_codecCtx || !m_running) return;
    m_packet->data = const_cast<uint8_t*>(data);
    m_packet->size = static_cast<int>(size);
    int ret = avcodec_send_packet(m_codecCtx, m_packet);
    if (ret < 0) {
        char errbuf[256];
        av_strerror(ret, errbuf, sizeof(errbuf));
        std::cerr << "[Decoder] avcodec_send_packet error: " << errbuf << std::endl;
        return;
    }
    while (avcodec_receive_frame(m_codecCtx, m_frame) >= 0) {
        if (m_frame->width > 0 && m_frame->height > 0) {
            std::lock_guard<std::mutex> lock(m_frameMutex);
            av_frame_unref(m_sharedFrame);
            av_frame_ref(m_sharedFrame, m_frame);
            m_hasNewFrame = true;
        }
    }
    av_packet_unref(m_packet);
}

bool WebRTCStreamNode::GetLatestFrame(AVFrame* destFrame) {
    std::lock_guard<std::mutex> lock(m_frameMutex);
    if (!m_hasNewFrame || !m_sharedFrame) return false;
    av_frame_unref(destFrame);
    av_frame_ref(destFrame, m_sharedFrame);
    m_hasNewFrame = false;
    return true;
}

void WebRTCStreamNode::Flush() { if (m_codecCtx) avcodec_flush_buffers(m_codecCtx); }

void WebRTCStreamNode::Cleanup() {
    m_running = false;
    m_queueCondVar.notify_all();

    if (m_peerConnection) { m_peerConnection->close(); m_peerConnection.reset(); }
    if (m_signalingThread.joinable()) m_signalingThread.join();
    if (m_decodeWorker.joinable()) m_decodeWorker.join();

    if (m_codecCtx) avcodec_free_context(&m_codecCtx);
    if (m_sharedFrame) av_frame_free(&m_sharedFrame);
    if (m_frame) av_frame_free(&m_frame);
    if (m_packet) av_packet_free(&m_packet);
    m_hasNewFrame = false;

    std::lock_guard<std::mutex> lock(m_queueMutex);
    std::queue<std::vector<uint8_t>>().swap(m_packetQueue);
}

void WebRTCStreamNode::StartSignaling(std::string signalingUrl) {
    rtc::Configuration config;
    m_peerConnection = std::make_shared<rtc::PeerConnection>(config);
    
    m_peerConnection->onStateChange([this](rtc::PeerConnection::State state) {
        if (state == rtc::PeerConnection::State::Connected) {
            std::cout << "[WebRTC] CONNECTED!" << std::endl;
            m_connected = true;
        } else if (state == rtc::PeerConnection::State::Failed || state == rtc::PeerConnection::State::Closed) {
            m_connected = false;
        }
    });

    // CRITICAL: Setup H.264 codec parameters for MediaMTX compatibility
    auto video = rtc::Description::Video("video", rtc::Description::Direction::RecvOnly);
    video.addH264Codec(96, "packetization-mode=1;profile-level-id=42e028");
    m_videoTrack = m_peerConnection->addTrack(video);

    m_videoTrack->onMessage([this](rtc::message_variant data) {
        if (std::holds_alternative<rtc::binary>(data)) {
            const auto& bin = std::get<rtc::binary>(data);
            auto bitstream = m_depacketizer.ProcessPayload(reinterpret_cast<const uint8_t*>(bin.data()), bin.size());
            if (!bitstream.empty()) {
                std::lock_guard<std::mutex> lock(m_queueMutex);
                m_packetQueue.push(std::move(bitstream));
                m_queueCondVar.notify_one();
            }
        }
    });

    m_peerConnection->onLocalCandidate([this, signalingUrl](const rtc::Candidate& candidate) {
        std::lock_guard<std::mutex> lock(m_iceMutex);
        if (m_sessionUrl.empty()) m_pendingCandidates.push_back(candidate.candidate());
        else SendIceCandidate(signalingUrl, candidate.candidate());
    });

    m_peerConnection->onLocalDescription([signalingUrl, this](const rtc::Description& desc) {
        json offerJson = {{"sdp", desc.generateSdp()}, {"type", desc.typeString()}};
        std::string response;
        if (m_httpClient && m_httpClient->Post(signalingUrl, offerJson.dump(), response)) {
            try {
                json answerJson = json::parse(response);
                m_peerConnection->setRemoteDescription(rtc::Description(answerJson["sdp"], "answer"));
                std::lock_guard<std::mutex> lock(m_iceMutex);
                if (answerJson.contains("session_url")) m_sessionUrl = answerJson["session_url"];
                for (const auto& cand : m_pendingCandidates) SendIceCandidate(signalingUrl, cand);
                m_pendingCandidates.clear();
            } catch (const std::exception& e) {
                std::cerr << "[WebRTC] Answer parse error: " << e.what() << std::endl;
            }
        }
    });

    try {
        m_peerConnection->setLocalDescription(rtc::Description::Type::Offer);
        while (m_running) std::this_thread::sleep_for(std::chrono::milliseconds(100));
    } catch (...) {}
}

void WebRTCStreamNode::SendIceCandidate(const std::string& signalingUrl, const std::string& candidateSdp) {
    json iceJson = {
        {"candidate", candidateSdp}, 
        {"sdpMid", "0"}, 
        {"sdpMLineIndex", 0},
        {"session_url", m_sessionUrl}
    };
    std::string icePath = signalingUrl;
    size_t offerPos = icePath.rfind("/offer");
    if (offerPos != std::string::npos) {
        icePath.replace(offerPos, 6, "/ice");
        std::string unused;
        if (m_httpClient) m_httpClient->Post(icePath, iceJson.dump(), unused);
    }
}

void WebRTCStreamNode::DecodeLoop() {
    while (m_running) {
        std::vector<uint8_t> data;
        {
            std::unique_lock<std::mutex> lock(m_queueMutex);
            m_queueCondVar.wait(lock, [this] { return !m_running || !m_packetQueue.empty(); });
            if (!m_running && m_packetQueue.empty()) break;
            if (!m_packetQueue.empty()) {
                data = std::move(m_packetQueue.front());
                m_packetQueue.pop();
            }
        }
        if (!data.empty()) {
            DecodeVideoData(data.data(), data.size());
        }
    }
}

} // namespace kvm::video
