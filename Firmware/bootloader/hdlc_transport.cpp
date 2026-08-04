#include "hdlc_transport.h"

namespace Bootloader {

HdlcTransport::HdlcTransport(Stream::StreamInterface& stream) noexcept :
    stream_(stream) {}

bool HdlcTransport::sendMessage(const Bootloader::Message& message) noexcept {
    HdlcMessage hdlc_message {
        .protocol_type = Hdlc::ProtocolType::BOOTLOADER_MSG,
        .message = message,
        .crc8 = 0, // TODO: calculate crc8
    };

    return stream_.write(reinterpret_cast<const uint8_t*>(&hdlc_message), sizeof(hdlc_message));
}

bool HdlcTransport::setMessageReceivedCallback(MessageReceivedCallback callback) noexcept {
    message_received_callback_ = callback;
    return true;
}

bool HdlcTransport::setSegmentReceivedCallback(SegmentReceivedCallback callback) noexcept {
    segment_received_callback_ = callback;
    return true;
}

void HdlcTransport::dispatchPayload() noexcept {
    if (!receiver_.receivePayload(Span<uint8_t>(buffer_, sizeof(buffer_)))) {
        return;
    }

    const MessageType message_type = static_cast<MessageType>(buffer_[0]);
    if (message_type == MessageType::kMemTransferSegment) {
        if (segment_received_callback_) {
            segment_received_callback_(reinterpret_cast<const MemTransferSegmentGeneric*>(buffer_));
        }
    } else if (isValidMessageType(message_type)) {
        if (message_received_callback_) {
            message_received_callback_(reinterpret_cast<const Message*>(buffer_));
        }
    }
}

void HdlcTransport::receive() noexcept {
    static_assert(sizeof(buffer_) >= kReadSize, "buffer_ is too small");

    for (uint32_t i = 0U; i < kMaxReads; i++) {
        const size_t bytes_read{stream_.read(Span<uint8_t>(buffer_, kReadSize))};
        if (bytes_read == 0) {
            break;
        }

        for (size_t j = 0U; j < bytes_read; j += kProcessSize) {
            const size_t process_size = std::min(bytes_read - j, kProcessSize);
            if (receiver_.process(Span<const uint8_t>(buffer_ + j, process_size))) {
                dispatchPayload();
            }
        }
    }
}



}; // namespace Bootloader