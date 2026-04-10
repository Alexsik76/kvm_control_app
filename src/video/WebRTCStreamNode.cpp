#include "video/WebRTCStreamNode.hpp"
#include "video/RtpDepacketizer.hpp"
#include <rtc/rtc.hpp>
#include <nlohmann/json.hpp>
#include <iostream>
#include <sstream>
#include <chrono>
#include <vector>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}

using json = nlohmann::json;

namespace kvm::video {

namespace {

std::vector<uint8_t> Base64Decode(const std::string& input) {
    static constexpr std::string_view kChars =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::vector<uint8_t> result;
    int val = 0;
    int bits = -8;
    for (unsigned char c : input) {
        if (c == '=' || c == '\r' || c == '\n') break;
        const size_t pos = kChars.find(c);
        if (pos == std::string_view::npos) continue;
        val = (val << 6) + static_cast<int>(pos);
        bits += 6;
        if (bits >= 0) {
            result.push_back(static_cast<uint8_t>((val >> bits) & 0xFF));
            bits -= 8;
        }
    }
    return result;
}

} // anonymous namespace

WebRTCStreamNode::WebRTCStreamNode(std::shared_ptr<network::IHttpClient> httpClient) 
    : m_httpClient(std::move(httpClient)) {}

WebRTCStreamNode::~WebRTCStreamNode() { Cleanup(); }

bool WebRTCStreamNode::Initialize() {
    Cleanup();
    rtc::InitLogger(rtc::LogLevel::Warning);
    av_log_set_level(AV_LOG_QUIET);
    if (!m_frame) m_frame = av_frame_alloc();
    if (!m_packet) m_packet = av_packet_alloc();
    if (!m_sharedFrame) m_sharedFrame = av_frame_alloc();
    if (!m_bgraFrame) m_bgraFrame = av_frame_alloc();
    return m_frame && m_packet && m_sharedFrame && m_bgraFrame && SetupDecoder();
}

bool WebRTCStreamNode::SetupDecoder() {
    const AVCodec* codec = avcodec_find_decoder(AV_CODEC_ID_H264);
    if (!codec) return false;
    m_codecCtx = avcodec_alloc_context3(codec);
    if (!m_codecCtx) return false;

    m_codecCtx->flags |= AV_CODEC_FLAG_LOW_DELAY;
    m_codecCtx->flags2 |= AV_CODEC_FLAG2_FAST;
    m_codecCtx->thread_count = 1;
    m_codecCtx->thread_type = FF_THREAD_SLICE;

    if (avcodec_open2(m_codecCtx, codec, nullptr) < 0) {
        avcodec_free_context(&m_codecCtx);
        return false;
    }
    return true;
}

bool WebRTCStreamNode::OpenStream(const std::string& signalingUrl, const std::string& token) {
    if (m_running) return false;
    if (m_httpClient) {
        m_httpClient->SetAccessToken(token);
    }
    m_running = true;
    m_decodeWorker = std::thread(&WebRTCStreamNode::DecodeLoop, this);
    m_signalingThread = std::thread(&WebRTCStreamNode::StartSignaling, this, signalingUrl, token);
    return true;
}

void WebRTCStreamNode::DecodeVideoData(const uint8_t* data, size_t size) {
    if (!m_codecCtx || !m_running) return;

    if (size > 4) {
        const uint8_t nalType = data[4] & 0x1F;
        if (nalType == 7) {
            avcodec_flush_buffers(m_codecCtx);
        }
    }

    av_packet_unref(m_packet);
    m_packet->data = const_cast<uint8_t*>(data);
    m_packet->size = static_cast<int>(size);
    
    int ret = avcodec_send_packet(m_codecCtx, m_packet);
    if (ret < 0) return;

    while (avcodec_receive_frame(m_codecCtx, m_frame) >= 0) {
        if (m_frame->width > 0 && m_frame->height > 0) {
            ProcessFrame(m_frame);

            std::lock_guard<std::mutex> lock(m_frameMutex);
            av_frame_unref(m_sharedFrame);
            av_frame_ref(m_sharedFrame, m_frame);
            m_hasNewFrame = true;
        }
    }
}

void WebRTCStreamNode::ProcessFrame(AVFrame* frame) {
    if (!m_frameCallback) return;

    m_swsContext = sws_getCachedContext(m_swsContext,
        frame->width, frame->height, (AVPixelFormat)frame->format,
        frame->width, frame->height, AV_PIX_FMT_BGRA,
        SWS_FAST_BILINEAR, nullptr, nullptr, nullptr);

    if (!m_swsContext) return;

    if (m_bgraFrame->width != frame->width || m_bgraFrame->height != frame->height) {
        av_frame_unref(m_bgraFrame);
        m_bgraFrame->format = AV_PIX_FMT_BGRA;
        m_bgraFrame->width = frame->width;
        m_bgraFrame->height = frame->height;
        av_frame_get_buffer(m_bgraFrame, 0);
    }

    sws_scale(m_swsContext, frame->data, frame->linesize, 0, frame->height,
              m_bgraFrame->data, m_bgraFrame->linesize);

    m_frameCallback(m_bgraFrame->data[0], m_bgraFrame->width, m_bgraFrame->height, m_bgraFrame->linesize[0]);
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

    if (m_swsContext) { sws_freeContext(m_swsContext); m_swsContext = nullptr; }
    if (m_bgraFrame) { av_frame_free(&m_bgraFrame); m_bgraFrame = nullptr; }
    if (m_codecCtx) { avcodec_free_context(&m_codecCtx); m_codecCtx = nullptr; }
    if (m_sharedFrame) { av_frame_free(&m_sharedFrame); m_sharedFrame = nullptr; }
    if (m_frame) { av_frame_free(&m_frame); m_frame = nullptr; }
    if (m_packet) { av_packet_free(&m_packet); m_packet = nullptr; }
    m_hasNewFrame = false;

    std::lock_guard<std::mutex> lock(m_queueMutex);
    std::queue<std::vector<uint8_t>>().swap(m_packetQueue);
}

void WebRTCStreamNode::StartSignaling(std::string signalingUrl, std::string token) {
    rtc::Configuration config;
    m_peerConnection = std::make_shared<rtc::PeerConnection>(config);
    
    m_peerConnection->onStateChange([this](rtc::PeerConnection::State state) {
        if (state == rtc::PeerConnection::State::Connected) {
            std::cout << "[WebRTC] CONNECTED!" << std::endl;
            m_connected = true;
            if (m_videoTrack) {
                m_videoTrack->requestKeyframe();
            }
        } else if (state == rtc::PeerConnection::State::Failed || state == rtc::PeerConnection::State::Closed) {
            m_connected = false;
        }
    });

    auto video = rtc::Description::Video("video", rtc::Description::Direction::RecvOnly);
    video.addH264Codec(96, "packetization-mode=1;profile-level-id=42001f");
    video.addH264Codec(97, "packetization-mode=1;profile-level-id=4d001f");
    video.addH264Codec(98, "packetization-mode=1;profile-level-id=64001f");
    video.addH264Codec(99, "packetization-mode=0;profile-level-id=42001f");
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

    m_peerConnection->onLocalDescription([signalingUrl, token, this](const rtc::Description& desc) {
        json offerJson = {{"sdp", desc.generateSdp()}, {"type", desc.typeString()}};
        std::string response;
        
        if (m_httpClient && m_httpClient->Post(signalingUrl, offerJson.dump(), response)) {
            try {
                json answerJson = json::parse(response);
                const std::string answerSdp = answerJson["sdp"].get<std::string>();
                m_peerConnection->setRemoteDescription(rtc::Description(answerSdp, "answer"));
                ParseAndInjectSpropParameterSets(answerSdp);
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

void WebRTCStreamNode::ParseAndInjectSpropParameterSets(const std::string& sdp) {
    static constexpr std::string_view kMarker = "sprop-parameter-sets=";

    size_t pos = sdp.find(kMarker);
    if (pos == std::string::npos) return;

    pos += kMarker.size();
    const size_t end = sdp.find_first_of(" \r\n", pos);
    const std::string paramSetsField = sdp.substr(pos, end - pos);

    std::istringstream stream(paramSetsField);
    std::string encoded;
    int injected = 0;
    while (std::getline(stream, encoded, ',')) {
        if (const size_t sc = encoded.find(';'); sc != std::string::npos) {
            encoded = encoded.substr(0, sc);
        }
        if (encoded.empty()) continue;

        const auto decoded = Base64Decode(encoded);
        if (decoded.empty()) continue;

        std::vector<uint8_t> annexbNal;
        annexbNal.reserve(decoded.size() + 4);
        annexbNal.insert(annexbNal.end(), {0x00, 0x00, 0x00, 0x01});
        annexbNal.insert(annexbNal.end(), decoded.begin(), decoded.end());

        {
            std::lock_guard<std::mutex> lock(m_queueMutex);
            m_packetQueue.push(std::move(annexbNal));
        }
        ++injected;
    }

    if (injected > 0) {
        m_queueCondVar.notify_all();
        std::cout << "[WebRTC] Queued " << injected << " sprop-parameter-sets for decoder." << std::endl;
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
        if (data.empty()) continue;

        const uint8_t* buf = data.data();
        const size_t bufSize = data.size();

        size_t offset = 0;
        while (offset < bufSize) {
            if (offset + 4 > bufSize) break;
            if (buf[offset]   != 0x00 || buf[offset+1] != 0x00 ||
                buf[offset+2] != 0x00 || buf[offset+3] != 0x01) {
                ++offset;
                continue;
            }
            const size_t nalStart = offset;
            offset += 4;

            size_t nalEnd = bufSize;
            for (size_t i = offset; i + 3 < bufSize; ++i) {
                if (buf[i] == 0x00 && buf[i+1] == 0x00 &&
                    buf[i+2] == 0x00 && buf[i+3] == 0x01) {
                    nalEnd = i;
                    break;
                }
            }

            const size_t nalSize = nalEnd - nalStart;
            DecodeVideoData(buf + nalStart, nalSize);

            offset = nalEnd;
        }
    }
}

} // namespace kvm::video