#pragma once

#include <string>
#include <thread>
#include <atomic>
#include <mutex>
#include <memory>

struct AVFormatContext;
struct AVCodecContext;
struct AVFrame;
struct AVPacket;

namespace kvm::video {

class FFmpegStreamNode {
public:
    FFmpegStreamNode();
    ~FFmpegStreamNode();

    bool Initialize();
    bool OpenStream(const std::string& url);
    
    bool GetLatestFrame(AVFrame* dest_frame);
    void Flush();
    bool IsRunning() const { return m_running.load(); }
    bool IsConnected() const { return m_connected.load(); }
    const std::string& GetLastError() const { return m_last_error; }

private:
    void OpenLoop(std::string url);
    void DecodeLoop();
    void Cleanup();

    AVFormatContext* m_fmt_ctx = nullptr;
    AVCodecContext* m_codec_ctx = nullptr;
    AVFrame* m_frame = nullptr;
    AVPacket* m_packet = nullptr;

    std::thread m_open_thread;
    std::thread m_decode_thread;
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_connected{false};
    std::string m_last_error;
    
    std::mutex m_frame_mutex;
    AVFrame* m_shared_frame = nullptr;
    bool m_has_new_frame = false;

    int m_video_stream_idx = -1;
};

} // namespace kvm::video
