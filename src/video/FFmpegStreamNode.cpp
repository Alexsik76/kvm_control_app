#include "video/FFmpegStreamNode.hpp"
#include <iostream>
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
}
namespace kvm::video {
FFmpegStreamNode::FFmpegStreamNode() {}
FFmpegStreamNode::~FFmpegStreamNode() { Cleanup(); }
bool FFmpegStreamNode::Initialize() {
    Cleanup();
    av_log_set_level(AV_LOG_FATAL);
    const AVCodec* codec = avcodec_find_decoder(AV_CODEC_ID_H264);
    if (!codec) return false;
    m_codec_ctx = avcodec_alloc_context3(codec);
    if (!m_codec_ctx) return false;
    m_codec_ctx->thread_count = 4;
    m_codec_ctx->thread_type = FF_THREAD_SLICE | FF_THREAD_FRAME;
    if (avcodec_open2(m_codec_ctx, codec, nullptr) < 0) return false;
    m_frame = av_frame_alloc();
    m_packet = av_packet_alloc();
    m_shared_frame = av_frame_alloc();
    return m_frame && m_packet && m_shared_frame;
}
bool FFmpegStreamNode::OpenStream(const std::string& url) {
    if (!m_codec_ctx) return false;
    AVDictionary* opts = nullptr;
    av_dict_set(&opts, "fflags", "nobuffer+discardcorrupt", 0);
    av_dict_set(&opts, "flags", "low_delay", 0);
    av_dict_set(&opts, "analyzeduration", "0", 0);
    av_dict_set(&opts, "probesize", "32", 0);
    av_dict_set(&opts, "buffer_size", "8388608", 0);
    av_dict_set(&opts, "rtsp_transport", "tcp", 0);
    int ret = avformat_open_input(&m_fmt_ctx, url.c_str(), nullptr, &opts);
    av_dict_free(&opts);
    if (ret < 0) return false;
    m_video_stream_idx = -1;
    for (unsigned int i = 0; i < m_fmt_ctx->nb_streams; i++) {
        if (m_fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            m_video_stream_idx = i;
            break;
        }
    }
    if (m_video_stream_idx == -1) m_video_stream_idx = 0;
    m_running = true;
    m_decode_thread = std::thread(&FFmpegStreamNode::DecodeLoop, this);
    return true;
}
void FFmpegStreamNode::DecodeLoop() {
    while (m_running) {
        int ret = av_read_frame(m_fmt_ctx, m_packet);
        if (ret == AVERROR(EAGAIN)) { std::this_thread::sleep_for(std::chrono::milliseconds(1)); continue; }
        if (ret == AVERROR_EOF || ret < 0) { std::this_thread::sleep_for(std::chrono::milliseconds(10)); continue; }
        if (m_packet->stream_index == m_video_stream_idx) {
            ret = avcodec_send_packet(m_codec_ctx, m_packet);
            while (ret >= 0) {
                ret = avcodec_receive_frame(m_codec_ctx, m_frame);
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF || ret < 0) break;
                if (m_frame->width > 0 && m_frame->height > 0) {
                    std::lock_guard<std::mutex> lock(m_frame_mutex);
                    av_frame_unref(m_shared_frame);
                    av_frame_ref(m_shared_frame, m_frame);
                    m_has_new_frame = true;
                }
            }
        }
        av_packet_unref(m_packet);
    }
}
bool FFmpegStreamNode::GetLatestFrame(AVFrame* dest_frame) {
    std::lock_guard<std::mutex> lock(m_frame_mutex);
    if (!m_has_new_frame || !m_shared_frame) return false;
    av_frame_unref(dest_frame);
    av_frame_ref(dest_frame, m_shared_frame);
    m_has_new_frame = false;
    return true;
}
void FFmpegStreamNode::Flush() { if (m_codec_ctx) avcodec_flush_buffers(m_codec_ctx); }
void FFmpegStreamNode::Cleanup() {
    m_running = false;
    if (m_fmt_ctx) m_fmt_ctx->interrupt_callback = { [](void*) -> int { return 1; }, nullptr };
    if (m_decode_thread.joinable()) m_decode_thread.join();
    if (m_shared_frame) av_frame_free(&m_shared_frame);
    if (m_frame) av_frame_free(&m_frame);
    if (m_packet) av_packet_free(&m_packet);
    if (m_fmt_ctx) avformat_close_input(&m_fmt_ctx);
    if (m_codec_ctx) avcodec_free_context(&m_codec_ctx);
}
} // namespace kvm::video

