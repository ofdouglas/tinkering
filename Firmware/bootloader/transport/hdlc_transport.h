#pragma once

#include <stdint.h>
#include <stddef.h>
#include <functional>

#include "bootloader/protocol.h"
#include "bootloader/transport/transport_interface.h"

#include "hdlc/hdlc.h"
#include "hdlc/protocol.h"
#include "interfaces/stream_interface.h"

namespace Bootloader {

/******************************************************************************
 *  Wire types
 ******************************************************************************/

struct __attribute__((packed)) HdlcMessage {
    Hdlc::ProtocolType protocol_type; // ProtocolType::BOOTLOADER_MSG
    Message message;
    uint8_t crc8; // SAE-J1850 CRC-8
};

template <size_t kSegmentSizeBytes>
struct __attribute__((packed)) HdlcMemTransferSegment {
    Hdlc::ProtocolType protocol_type; // ProtocolType::BOOTLOADER_SEG
    MemTransferSegment<kSegmentSizeBytes> segment;
    uint32_t crc32; // HDLC-style CRC-32
};


/******************************************************************************
 *  Constants
 ******************************************************************************/

static constexpr size_t kHdlcMessageSizeBytes = sizeof(HdlcMessage);

template <size_t kSegmentSizeBytes>
static constexpr size_t kHdlcMemTransferSegmentSizeBytes = sizeof(HdlcMemTransferSegment<kSegmentSizeBytes>);

static constexpr size_t kDefaultHdlcSegmentSizeBytes = 64U;

static constexpr size_t kHdlcMaxPayloadBytes = kHdlcMemTransferSegmentSizeBytes<kDefaultHdlcSegmentSizeBytes>;


/******************************************************************************
 *  Transport Class
 ******************************************************************************/

template <size_t kSegmentSizeBytes = kDefaultHdlcSegmentSizeBytes>
class HdlcTransport : public TransportInterface {
public:
    HdlcTransport(Stream::StreamInterface& stream) noexcept : stream_(stream) {}

    bool sendMessage(const Message& message) noexcept override {
        HdlcMessage hdlc_message{
            .protocol_type = Hdlc::ProtocolType::BOOTLOADER_MSG,
            .message = message,
            .crc8 = 0, // TODO: calculate crc8
        };
        return stream_.write(Span<const uint8_t>(reinterpret_cast<const uint8_t*>(&hdlc_message), sizeof(hdlc_message))) != 0U;
    }

    // TODO: handle segment wrapping
    // TODO: process multiple bytes at a time
    void receive() noexcept override {
        static_assert(sizeof(buffer_) >= kReadSize, "buffer_ is too small");

        for (uint32_t i = 0U; i < kMaxReads; ++i) {
            const size_t bytes_read{stream_.read(Span<uint8_t>(buffer_, kReadSize))};
            if (bytes_read == 0U) {
                break;
            }

            for (size_t j = 0U; j < bytes_read; ++j) {
                if (receiver_.process(Span<const uint8_t>(buffer_ + j, 1U))) {
                    dispatchPayload();
                }
            }
        }
    }

    bool setMessageReceivedCallback(MessageReceivedCallback callback) noexcept override {
        message_received_callback_ = callback;
        return true;
    }

    bool setSegmentReceivedCallback(SegmentReceivedCallback callback) noexcept override {
        segment_received_callback_ = callback;
        return true;
    }

private:
    static constexpr size_t kBufferSizeBytes{kHdlcMemTransferSegmentSizeBytes<kSegmentSizeBytes>};
    static constexpr size_t kReadSize{kBufferSizeBytes};
    static constexpr uint32_t kMaxReads{3U};

    void dispatchPayload() noexcept {
        if (!receiver_.receivePayload(Span<uint8_t>(buffer_, sizeof(buffer_)))) {
            return;
        }

        const MessageType message_type = static_cast<MessageType>(buffer_[0]);
        if (message_type == MessageType::kMemTransferSegment) {
            if (segment_received_callback_) {
                MemTransferSegmentView segment_view{};
                segment_view.message_type = buffer_[0];
                segment_view.job_id = buffer_[1];
                segment_view.segment_number = static_cast<uint16_t>(buffer_[2]) |
                    (static_cast<uint16_t>(buffer_[3]) << 8U);
                segment_view.data = Span<const uint8_t>(buffer_ + 4U, kSegmentSizeBytes - 4U);
                segment_received_callback_(segment_view);
            }
        } else if (isValidMessageType(message_type)) {
            if (message_received_callback_) {
                message_received_callback_(*reinterpret_cast<const Message*>(buffer_));
            }
        }
    }

    Stream::StreamInterface& stream_;
    uint8_t buffer_[kBufferSizeBytes];
    Hdlc::Receiver<kBufferSizeBytes> receiver_{};
    MessageReceivedCallback message_received_callback_{};
    SegmentReceivedCallback segment_received_callback_{};
};

} // namespace Bootloader
