#include "hdlc.h"
#include <cstring>

namespace Hdlc {

    size_t byteStuff(uint8_t byte, Span<uint8_t> out) {
        if (out.size() < 1U) {
            return 0U;
        }
        if ((byte == HdlcFlag::kFlag) || (byte == HdlcFlag::kEscape)) {
            if (out.size() < 2U) {
                return 0U;
            }
            out[0] = HdlcFlag::kEscape;
            out[1] = static_cast<uint8_t>(byte ^ HdlcFlag::kXorValue);
            return 2U;
        }
        out[0] = byte;
        return 1U;
    }

    bool encodeFrame(Span<const uint8_t> payload, Span<uint8_t> frame) {
        if (frame.size() < payload.size() + 2U) {
            return false;
        }
        frame[0] = HdlcFlag::kFlag;
        size_t frame_index = 1;

        for (size_t i = 0; i < payload.size(); i++) {
            uint8_t stuff[2];
            const size_t stuff_len = byteStuff(payload[i], Span<uint8_t>(stuff, 2U));
            if (stuff_len == 0U) {
                return false;
            }
            if ((frame_index + stuff_len) > frame.size()) {
                return false;
            }
            std::memcpy(frame.data() + frame_index, stuff, stuff_len);
            frame_index += stuff_len;
        }

        frame[frame_index] = HdlcFlag::kFlag;
        return true;
    }

    bool decodePayload(Span<const uint8_t> input, Span<uint8_t> output) {
        size_t output_index = 0;
        bool stuff_flag = false;

        for (size_t i = 0; (i < input.size()) && (output_index < output.size()); i++) {
            if (input[i] == HdlcFlag::kFlag) {
                return output_index > 0U;
            }
            if (input[i] == HdlcFlag::kEscape) {
                stuff_flag = true;
                continue;
            }
            if (stuff_flag) {
                output[output_index++] = input[i] ^ HdlcFlag::kXorValue;
                stuff_flag = false;
            } else {
                output[output_index++] = input[i];
            }
        }
        return output_index > 0U;
    }

} // namespace Hdlc

constexpr uint8_t Hdlc::HdlcFlag::kFlag;
constexpr uint8_t Hdlc::HdlcFlag::kEscape;
constexpr uint8_t Hdlc::HdlcFlag::kXorValue;
