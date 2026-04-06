The video packets are arriving, but `RtpDepacketizer::ProcessPayload` returns empty vectors because it treats the raw RTP packet as the H.264 payload. `libdatachannel`'s `track->onMessage` delivers the full RTP packet including the 12+ byte RTP header, so byte 0 is the RTP version (`0x80` or `0x90`), not a NAL header.

Task: 
1. Update `RtpDepacketizer.cpp` to parse and skip the RTP header.
   - Validate the RTP version (V=2).
   - Calculate the RTP header length: base 12 bytes + CSRC count (CC * 4).
   - Check for the extension bit (X). If set, read the extension header length and skip it.
   - Adjust the `payload` pointer and `size` to point to the actual H.264 payload.
   - Keep the existing H.264 NAL unit processing logic (Single NAL, STAP-A, FU-A) for the adjusted payload.
2. In `WebRTCStreamNode::DecodeVideoData`, add error logging using `std::cerr` if `avcodec_send_packet` returns a value less than 0, to ensure we catch any further decoder issues.