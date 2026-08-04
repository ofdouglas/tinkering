#ifndef HDLC_TRANSPORT_H
#define HDLC_TRANSPORT_H

#include <stdint.h>
#include <stddef.h>
#include <functional>

#include "protocol.h"
#include "hdlc/hdlc.h"
#include "hdlc/protocol.h"
#include "interfaces/stream_interface.h"

namespace Bootloader {

__attribute__((packed))
struct HdlcMessage {
    Hdlc::ProtocolType protocol_type; // ProtocolType::BootloaderMsg
    Bootloader::Message message;
    uint8_t crc8; // SAE-J1850 CRC-8
};

__attribute__((packed))
struct HdlcMemTransferSegment {
    Hdlc::ProtocolType protocol_type; // ProtocolType::BootloaderSegment
    Bootloader::MemTransferSegmentHdlc segment;
    uint32_t crc32; // HDLC-style CRC-32
};

template <size_t kSegmentSizeBytes>
class HdlcTransport : public TransportInterface {
public:
    HdlcTransport(Stream::StreamInterface& stream) noexcept :
        stream_(stream) {}

    bool sendMessage(const Bootloader::Message& message) noexcept override;
    bool sendSegment(const Bootloader::MemTransferSegment<kSegmentSizeBytes>& segment) noexcept override;

    void receive() noexcept;
    bool setMessageReceivedCallback(TransportInterface::MessageReceivedCallback callback) noexcept;
    bool setSegmentReceivedCallback(TransportInterface::SegmentReceivedCallback callback) noexcept;

private:
    static constexpr size_t kReadSize{kSegmentSizeBytes};
    static constexpr size_t kProcessSize{HdlcMessage + 1U};
    static constexpr uint32_t kMaxReads{3U};
    
    void dispatchPayload() noexcept;

    Stream::StreamInterface& stream_;
    Hdlc::Receiver<kSegmentSizeBytes> receiver_{};
    TransportInterface::MessageReceivedCallback message_received_callback_{};
    TransportInterface::SegmentReceivedCallback segment_received_callback_{};
    uint8_t buffer_[kSegmentSizeBytes];
};



} // namespace Bootloader

#endif // HDLC_TRANSPORT_H