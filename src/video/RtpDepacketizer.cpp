#include "video/RtpDepacketizer.hpp"
#include <cstring>
#include <iostream>

namespace kvm::video {

std::vector<uint8_t> RtpDepacketizer::ProcessPayload(const uint8_t* payload, size_t size) {
    if (size < 1) return {};

    uint8_t nalHeader = payload[0];
    uint8_t nalType = nalHeader & 0x1F;

    // Single NAL unit packet (1-23)
    if (nalType >= 1 && nalType <= 23) {
        std::vector<uint8_t> result;
        result.reserve(size + 4);
        result.insert(result.end(), std::begin(START_CODE), std::end(START_CODE));
        result.insert(result.end(), payload, payload + size);
        m_fuBuffer.clear();
        m_isWaitingForStart = true;
        return result;
    }

    // STAP-A (24)
    if (nalType == 24) {
        std::vector<uint8_t> result;
        size_t offset = 1;
        while (offset + 2 <= size) {
            uint16_t naluSize = (payload[offset] << 8) | payload[offset + 1];
            offset += 2;
            if (offset + naluSize > size) break;

            result.insert(result.end(), std::begin(START_CODE), std::end(START_CODE));
            result.insert(result.end(), payload + offset, payload + offset + naluSize);
            offset += naluSize;
        }
        m_fuBuffer.clear();
        m_isWaitingForStart = true;
        return result;
    }

    // FU-A (28)
    if (nalType == 28) {
        if (size < 2) return {};

        uint8_t fuHeader = payload[1];
        bool startBit = (fuHeader & 0x80) != 0;
        bool endBit = (fuHeader & 0x40) != 0;
        uint8_t actualNalType = fuHeader & 0x1F;
        uint8_t reconstructedNalHeader = (nalHeader & 0xE0) | actualNalType;

        if (startBit) {
            m_fuBuffer.clear();
            m_fuBuffer.reserve(size * 4); // Heuristic
            m_fuBuffer.insert(m_fuBuffer.end(), std::begin(START_CODE), std::end(START_CODE));
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
