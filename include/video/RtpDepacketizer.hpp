#pragma once

#include <vector>
#include <cstdint>
#include <optional>

namespace kvm::video {

/**
 * @brief Handles RFC 6184 RTP depacketization for H.264.
 * Transforms RTP payloads into Annex B bitstream (with 00 00 00 01 start codes).
 */
class RtpDepacketizer {
public:
    RtpDepacketizer() = default;
    ~RtpDepacketizer() = default;

    /**
     * @brief Processes an incoming RTP payload.
     * @param payload Pointer to the RTP payload (after RTP header).
     * @param size Size of the RTP payload.
     * @return A vector of bytes containing one or more Annex B NAL units, or empty if more fragments are needed.
     */
    std::vector<uint8_t> ProcessPayload(const uint8_t* payload, size_t size);

private:
    std::vector<uint8_t> m_fuBuffer;
    bool m_isWaitingForStart = true;

    static constexpr uint8_t START_CODE[] = {0x00, 0x00, 0x00, 0x01};
};

} // namespace kvm::video
