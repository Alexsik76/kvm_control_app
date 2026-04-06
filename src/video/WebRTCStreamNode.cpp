#include "video/WebRTCStreamNode.hpp"
#include <rtc/rtc.hpp>
#include <nlohmann/json.hpp>
#include <iostream>
#include <chrono>

#ifdef _WIN32
#include <windows.h>
#include <winhttp.h>
#pragma comment(lib, "winhttp.lib")
#endif

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
}

using json = nlohmann::json;

namespace kvm::video {

// --- Helper for WinHTTP POST requests ---
static bool SendWinHttpPost(const std::string& url, const std::string& body, std::string& outResponse) {
#ifdef _WIN32
    std::wstring wUrl(url.begin(), url.end());
    
    URL_COMPONENTS urlComp = { 0 };
    urlComp.dwStructSize = sizeof(urlComp);
    wchar_t hostName[256];
    wchar_t urlPath[1024];
    urlComp.lpszHostName = hostName;
    urlComp.dwHostNameLength = sizeof(hostName) / sizeof(hostName[0]);
    urlComp.lpszUrlPath = urlPath;
    urlComp.dwUrlPathLength = sizeof(urlPath) / sizeof(urlPath[0]);

    if (!WinHttpCrackUrl(wUrl.c_str(), 0, 0, &urlComp)) {
        std::cerr << "[WinHTTP] Failed to crack URL: " << url << std::endl;
        return false;
    }

    HINTERNET hSession = WinHttpOpen(L"Mozilla/5.0 (Windows NT 10.0; Win64; x64) KVM-Client/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) {
        std::cerr << "[WinHTTP] Failed to open session." << std::endl;
        return false;
    }

    HINTERNET hConnect = WinHttpConnect(hSession, urlComp.lpszHostName, urlComp.nPort, 0);
    if (!hConnect) { 
        std::cerr << "[WinHTTP] Failed to connect to host: " << url << std::endl;
        WinHttpCloseHandle(hSession); 
        return false; 
    }

    DWORD flags = (urlComp.nScheme == INTERNET_SCHEME_HTTPS) ? WINHTTP_FLAG_SECURE : 0;
    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"POST", urlComp.lpszUrlPath, NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
    
    if (!hRequest) {
        std::cerr << "[WinHTTP] Failed to open request." << std::endl;
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return false;
    }

    // Allow invalid certs if necessary (e.g. self-signed in local dev network)
    DWORD dwFlags = SECURITY_FLAG_IGNORE_UNKNOWN_CA | SECURITY_FLAG_IGNORE_CERT_DATE_INVALID | SECURITY_FLAG_IGNORE_CERT_CN_INVALID;
    if (flags & WINHTTP_FLAG_SECURE) {
        WinHttpSetOption(hRequest, WINHTTP_OPTION_SECURITY_FLAGS, &dwFlags, sizeof(dwFlags));
    }

    std::wstring headers = L"Content-Type: application/json\r\n";
    if (!WinHttpSendRequest(hRequest, headers.c_str(), static_cast<DWORD>(headers.length()), (LPVOID)body.c_str(), static_cast<DWORD>(body.length()), static_cast<DWORD>(body.length()), 0)) {
        std::cerr << "[WinHTTP] SendRequest failed with error: " << GetLastError() << std::endl;
        WinHttpCloseHandle(hRequest); WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession);
        return false;
    }

    if (!WinHttpReceiveResponse(hRequest, NULL)) {
        std::cerr << "[WinHTTP] ReceiveResponse failed." << std::endl;
        WinHttpCloseHandle(hRequest); WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession);
        return false;
    }

    DWORD dwStatusCode = 0;
    DWORD dwStatusSize = sizeof(dwStatusCode);
    WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &dwStatusCode, &dwStatusSize, WINHTTP_NO_HEADER_INDEX);
    std::cout << "[WinHTTP] Request to " << url << " returned status: " << dwStatusCode << std::endl;

    DWORD dwSize = 0;
    DWORD dwDownloaded = 0;
    do {
        dwSize = 0;
        if (!WinHttpQueryDataAvailable(hRequest, &dwSize)) break;
        if (dwSize == 0) break;
        
        std::vector<char> buffer(dwSize + 1, 0);
        if (WinHttpReadData(hRequest, (LPVOID)buffer.data(), dwSize, &dwDownloaded)) {
            outResponse.append(buffer.data(), dwDownloaded);
        }
    } while (dwSize > 0);

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    return true;
#else
    return false; // Not implemented for non-Windows
#endif
}


WebRTCStreamNode::WebRTCStreamNode() {}

WebRTCStreamNode::~WebRTCStreamNode() {
    Cleanup();
}

bool WebRTCStreamNode::Initialize() {
    Cleanup();
    
    rtc::InitLogger(rtc::LogLevel::Verbose);
    
    if (!m_frame) m_frame = av_frame_alloc();
    if (!m_packet) m_packet = av_packet_alloc();
    if (!m_sharedFrame) m_sharedFrame = av_frame_alloc();
    
    return m_frame && m_packet && m_sharedFrame && SetupDecoder();
}

bool WebRTCStreamNode::SetupDecoder() {
    const AVCodec* codec = avcodec_find_decoder(AV_CODEC_ID_H264);
    if (!codec) {
        m_lastError = "H.264 codec not found";
        return false;
    }

    m_codecCtx = avcodec_alloc_context3(codec);
    if (!m_codecCtx) {
        m_lastError = "Failed to allocate codec context";
        return false;
    }

    m_codecCtx->thread_count = 4;
    m_codecCtx->thread_type = FF_THREAD_SLICE | FF_THREAD_FRAME;

    if (avcodec_open2(m_codecCtx, codec, nullptr) < 0) {
        m_lastError = "Failed to open codec";
        avcodec_free_context(&m_codecCtx);
        return false;
    }
    
    return true;
}

bool WebRTCStreamNode::OpenStream(const std::string& signalingUrl) {
    if (m_running) return false;
    m_running = true;
    m_connected = false;
    m_lastError = "";
    
    m_signalingThread = std::thread(&WebRTCStreamNode::StartSignaling, this, signalingUrl);
    return true;
}

void WebRTCStreamNode::DecodeVideoData(const uint8_t* data, size_t size) {
    if (!m_codecCtx || !m_running) return;

    m_packet->data = const_cast<uint8_t*>(data);
    m_packet->size = static_cast<int>(size);

    int ret = avcodec_send_packet(m_codecCtx, m_packet);
    if (ret < 0) return;

    while (ret >= 0) {
        ret = avcodec_receive_frame(m_codecCtx, m_frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF || ret < 0) break;

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

void WebRTCStreamNode::Flush() {
    if (m_codecCtx) avcodec_flush_buffers(m_codecCtx);
}

void WebRTCStreamNode::Cleanup() {
    m_running = false;
    
    if (m_peerConnection) {
        m_peerConnection->close();
        m_peerConnection.reset();
    }
    
    if (m_signalingThread.joinable()) {
        m_signalingThread.join();
    }
    
    if (m_codecCtx) avcodec_free_context(&m_codecCtx);
    if (m_sharedFrame) av_frame_free(&m_sharedFrame);
    if (m_frame) av_frame_free(&m_frame);
    if (m_packet) av_packet_free(&m_packet);
    
    m_hasNewFrame = false;
    m_connected = false;
}

void WebRTCStreamNode::StartSignaling(std::string signalingUrl) {
    std::cout << "[WebRTC] Starting signaling to: " << signalingUrl << std::endl << std::flush;
    rtc::Configuration config;
    
    std::cout << "[WebRTC] Creating PeerConnection..." << std::endl << std::flush;
    m_peerConnection = std::make_shared<rtc::PeerConnection>(config);
    
    m_peerConnection->onStateChange([this](rtc::PeerConnection::State state) {
        std::cout << "[WebRTC] State change: " << (int)state << std::endl << std::flush;
        if (state == rtc::PeerConnection::State::Connected) {
            std::cout << "[WebRTC] Connected!" << std::endl << std::flush;
            m_connected = true;
        } else if (state == rtc::PeerConnection::State::Failed || state == rtc::PeerConnection::State::Closed) {
            std::cout << "[WebRTC] Disconnected / Failed." << std::endl << std::flush;
            m_connected = false;
        }
    });

    m_peerConnection->onTrack([this](std::shared_ptr<rtc::Track> track) {
        std::cout << "[WebRTC] Track received: " << track->mid() << std::endl << std::flush;
        track->onMessage([this](rtc::message_variant data) {
            if (std::holds_alternative<rtc::binary>(data)) {
                const auto& bin = std::get<rtc::binary>(data);
                DecodeVideoData(reinterpret_cast<const uint8_t*>(bin.data()), bin.size());
            }
        });
    });

    // Request Video Track (WHEP/WebRTC RecvOnly)
    std::cout << "[WebRTC] Adding video track..." << std::endl << std::flush;
    (void)m_peerConnection->addTrack(rtc::Description::Video{"video", rtc::Description::Direction::RecvOnly});

    // Register local candidate handler BEFORE setLocalDescription
    m_peerConnection->onLocalCandidate([this, signalingUrl](const rtc::Candidate& candidate) {
        std::string candStr = candidate.candidate();
        std::cout << "[WebRTC] Local ICE Candidate: " << candStr.substr(0, 30) << "..." << std::endl << std::flush;
        std::lock_guard<std::mutex> lock(m_iceMutex);
        if (m_sessionUrl.empty()) {
            std::cout << "[WebRTC] Queueing ICE Candidate (waiting for SDP Answer)..." << std::endl << std::flush;
            m_pendingCandidates.push_back(candStr);
        } else {
            SendIceCandidate(signalingUrl, candStr);
        }
    });

    // Wait for the SDP Offer generation
    m_peerConnection->onLocalDescription([signalingUrl, this](const rtc::Description& desc) {
        std::cout << "[WebRTC] Local Description (Offer) generated!" << std::endl << std::flush;
        json offerJson = {
            {"sdp", desc.generateSdp()},
            {"type", desc.typeString()}
        };
        
        std::cout << "[WebRTC] Sending SDP Offer via WinHTTP..." << std::endl << std::flush;
        std::string response;
        if (SendWinHttpPost(signalingUrl, offerJson.dump(), response)) {
            std::cout << "[WebRTC] Received SDP Answer!" << std::endl << std::flush;
            try {
                json answerJson = json::parse(response);
                std::string answerSdp = answerJson["sdp"];
                rtc::Description answer(answerSdp, "answer");
                m_peerConnection->setRemoteDescription(answer);
                
                // Extract session_url for Trickle ICE
                {
                    std::lock_guard<std::mutex> lock(m_iceMutex);
                    if (answerJson.contains("session_url") && !answerJson["session_url"].is_null()) {
                        m_sessionUrl = answerJson["session_url"];
                    }
                    
                    // Flush pending candidates
                    std::cout << "[WebRTC] Flushing " << m_pendingCandidates.size() << " queued ICE candidates..." << std::endl << std::flush;
                    for (const auto& cand : m_pendingCandidates) {
                        SendIceCandidate(signalingUrl, cand);
                    }
                    m_pendingCandidates.clear();
                }
            } catch (const std::exception& e) {
                m_lastError = std::string("Signaling failed to parse answer: ") + e.what();
                std::cerr << "[WebRTC] " << m_lastError << "\n" << std::flush;
                m_running = false;
            }
        } else {
            m_lastError = "Signaling failed: WinHTTP Error.";
            std::cerr << "[WebRTC] " << m_lastError << "\n" << std::flush;
            m_running = false;
        }
    });

    // Start SDP generation
    try {
        std::cout << "[WebRTC] Initiating setLocalDescription..." << std::endl << std::flush;
        m_peerConnection->setLocalDescription(rtc::Description::Type::Offer);
        
        // Keep thread alive while running
        while (m_running) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        std::cout << "[WebRTC] Signaling thread exiting." << std::endl << std::flush;
    } catch (const std::exception& e) {
        std::cerr << "[WebRTC] CRITICAL EXCEPTION: " << e.what() << std::endl << std::flush;
    } catch (...) {
        std::cerr << "[WebRTC] UNKNOWN CRITICAL EXCEPTION" << std::endl << std::flush;
    }
}

void WebRTCStreamNode::SendIceCandidate(const std::string& signalingUrl, const std::string& candidateSdp) {
    json iceJson = {
        {"candidate", candidateSdp},
        {"session_url", m_sessionUrl}
    };
    
    std::string icePath = signalingUrl;
    size_t offerPos = icePath.rfind("/offer");
    if (offerPos != std::string::npos) {
        icePath.replace(offerPos, 6, "/ice");
        std::string unusedResponse;
        SendWinHttpPost(icePath, iceJson.dump(), unusedResponse);
    }
}

} // namespace kvm::video
