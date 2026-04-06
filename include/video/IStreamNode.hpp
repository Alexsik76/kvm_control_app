#pragma once
#include <string>
#include <cstdint>

struct AVFrame;

namespace kvm::video {

class IStreamNode {
public:
    virtual ~IStreamNode() = default;

    virtual bool Initialize() = 0;
    virtual bool OpenStream(const std::string& url) = 0;
    virtual bool IsConnected() const noexcept = 0;
    virtual bool GetLatestFrame(AVFrame* destFrame) = 0;
    virtual void Flush() = 0;
};

} // namespace kvm::video
