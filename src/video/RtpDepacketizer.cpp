#include "video/RtpDepacketizer.hpp"

namespace kvm::video {

namespace {
    constexpr uint8_t kAnnexBStartCode[] = {0x00, 0x00, 0x00, 0x01};
    constexpr size_t kMinRtpHeaderSize = 12;
    constexpr size_t kMaxReasonableHeaderSize = 128;
}

std::vector<uint8_t> RtpDepacketizer::ProcessPayload(const uint8_t* rtpPacket, size_t rtpSize) {
    if (rtpSize < kMinRtpHeaderSize) return {};

    const uint8_t firstByte = rtpPacket[0];
    const uint8_t version = (firstByte >> 6) & 0x03;
    const bool hasPadding = (firstByte & 0x20) != 0;
    const bool hasExtension = (firstByte & 0x10) != 0;
    const uint8_t csrcCount = firstByte & 0x0F;

    if (version != 2) return {};

    const uint8_t payloadType = rtpPacket[1] & 0x7F;
    if (payloadType < 96) return {}; // Ignore RTCP (e.g. 72) and non-dynamic payload types

    size_t headerSize = kMinRtpHeaderSize + (csrcCount * 4u);
    if (rtpSize < headerSize) return {};

    if (hasExtension) {
        const size_t extOffset = headerSize;
        if (rtpSize < extOffset + 4) return {};
        const uint16_t extLen = (static_cast<uint16_t>(rtpPacket[extOffset + 2]) << 8)
                              | rtpPacket[extOffset + 3];
        const size_t extBytes = 4u + (static_cast<size_t>(extLen) * 4u);
        if (extBytes > kMaxReasonableHeaderSize || headerSize + extBytes > rtpSize) return {};
        headerSize += extBytes;
    }

    if (rtpSize <= headerSize) return {};

    const uint8_t* payload = rtpPacket + headerSize;
    size_t size = rtpSize - headerSize;

    if (hasPadding && size > 0) {
        const uint8_t paddingLen = rtpPacket[rtpSize - 1];
        if (paddingLen == 0 || paddingLen > size) return {};
        size -= paddingLen;
    }

    if (size == 0) return {};

    const uint8_t nalHeader = payload[0];
    const uint8_t nalType = nalHeader & 0x1F;

    if (nalType >= 1 && nalType <= 23) {
        std::vector<uint8_t> result;
        result.reserve(size + 4);
        result.insert(result.end(), kAnnexBStartCode, kAnnexBStartCode + 4);
        result.insert(result.end(), payload, payload + size);
        if (m_isWaitingForStart) {
            m_fuBuffer.clear();
        }
        return result;
    }

    if (nalType == 24) {
        std::vector<uint8_t> result;
        size_t offset = 1;
        while (offset + 2 <= size) {
            const uint16_t naluSize = (static_cast<uint16_t>(payload[offset]) << 8)
                                    | payload[offset + 1];
            offset += 2;
            if (naluSize == 0 || offset + naluSize > size) break;
            result.insert(result.end(), kAnnexBStartCode, kAnnexBStartCode + 4);
            result.insert(result.end(), payload + offset, payload + offset + naluSize);
            offset += naluSize;
        }
        if (!result.empty()) {
            m_fuBuffer.clear();
            m_isWaitingForStart = true;
        }
        return result;
    }

    if (nalType == 28) {
        if (size < 2) return {};

        const uint8_t fuHeader = payload[1];
        const bool startBit = (fuHeader & 0x80) != 0;
        const bool endBit   = (fuHeader & 0x40) != 0;
        const uint8_t actualNalType = fuHeader & 0x1F;
        const uint8_t reconstructedNalHeader = (nalHeader & 0xE0) | actualNalType;

        if (startBit) {
            m_fuBuffer.clear();
            m_fuBuffer.reserve(size * 4);
            m_fuBuffer.insert(m_fuBuffer.end(), kAnnexBStartCode, kAnnexBStartCode + 4);
            m_fuBuffer.push_back(reconstructedNalHeader);
            m_fuBuffer.insert(m_fuBuffer.end(), payload + 2, payload + size);
            m_isWaitingForStart = false;
        } else if (!m_isWaitingForStart) {
            m_fuBuffer.insert(m_fuBuffer.end(), payload + 2, payload + size);
        }

        if (endBit && !m_isWaitingForStart) {
            std::vector<uint8_t> result = std::move(m_fuBuffer);
            m_fuBuffer.clear();
            m_isWaitingForStart = true;
            return result;
        }
    }

    return {};
}

} // namespace kvm::video