#pragma once
/*
 * @file  hdlc_transport.h
 * @brief HDLC transport layer for the bootloader. Implements the bootloader::TransportInterface.
 */

#include <cstdint>
#include <cstddef>
#include <functional>
#include <cstring>

#include "bootloader/protocol.h"
#include "bootloader/transport/transport_interface.h"

#include "hdlc/hdlc.h"
#include "hdlc/protocol.h"
#include "interfaces/stream_interface.h"
#include "logging/logging.h"

namespace bootloader {

/******************************************************************************
 *  Wire types
 ******************************************************************************/

struct __attribute__((packed)) HdlcCommandFrame {
    hdlc::ServiceType protocol_type; // ServiceType::BOOTLOADER_CMD
    uint8_t command[8];
    uint8_t crc8; // SAE-J1850 CRC-8
};

template <size_t kSegmentSizeWords>
struct __attribute__((packed)) HdlcSegmentFrame {
    hdlc::ServiceType protocol_type; // ServiceType::BOOTLOADER_SEG
    uint8_t padding[3U];              // 3 bytes of padding to align the segment to a 4-byte boundary
    uint8_t command_bytes[4U];
    uint8_t segment_bytes[kSegmentTransferPayloadBytes<kSegmentSizeWords>];
    uint32_t crc32; // HDLC-style CRC-32
};


/******************************************************************************
 *  Constants
 ******************************************************************************/

static constexpr size_t kHdlcCommandFrameSizeBytes = sizeof(HdlcCommandFrame);

template <size_t kSegmentSizeWords>
static constexpr size_t kHdlcSegmentFrameSizeBytes = sizeof(HdlcSegmentFrame<kSegmentSizeWords>);

template <size_t kSegmentSizeWords>
static constexpr size_t kHdlcMaxPayloadBytes = kHdlcSegmentFrameSizeBytes<kSegmentSizeWords>;


/******************************************************************************
 *  Transport Class
 ******************************************************************************/

template <size_t kSegmentSizeWords = kDefaultHdlcSegmentSizeWords>
class HdlcTransport : public TransportInterface {
public:
    HdlcTransport(Stream::StreamInterface& stream) noexcept : stream_(stream) {}

    bool setCommandReceivedCallback(CommandReceivedCallback callback) noexcept override {
        command_received_callback_ = callback;
        return true;
    }

    bool setSegmentReceivedCallback(SegmentReceivedCallback callback) noexcept override {
        segment_received_callback_ = callback;
        return true;
    }

    bool sendCommand(const CommandVariant& command) noexcept override {
        CommandPacket command_packet{};

        if (std::holds_alternative<GeneralCommand>(command)) {
            const GeneralCommand& general_command = std::get<GeneralCommand>(command);
            const std::optional<CommandPacket> encoded = general_command.encode();
            if (!encoded.has_value()) {
                LOG_ERROR() << "Failed to encode general command";
                return false;
            }
            command_packet = encoded.value();
        } else if (std::holds_alternative<MemoryCommand>(command)) {
            const MemoryCommand& memory_command = std::get<MemoryCommand>(command);
            const std::optional<CommandPacket> encoded = memory_command.encode();
            if (!encoded.has_value()) {
                LOG_ERROR() << "Failed to encode memory command";
                return false;
            }
            command_packet = encoded.value();
        } else {
            LOG_ERROR() << "Invalid command type";
            return false;
        }

        const util::Span<const uint8_t> bytes{command_packet.bytes, sizeof(command_packet.bytes)};
        return stream_.write(bytes) == bytes.size();
    }

    bool sendSegment(const SegmentTransferPacketView& segment) noexcept {
        if (segment.memory_words.size() != kSegmentSizeWords) {
            LOG_ERROR() << "Invalid segment size: " << segment.memory_words.size() << " != " << kSegmentSizeWords;
            return false;
        }

        HdlcSegmentFrame<kSegmentSizeWords> frame{
            .protocol_type = hdlc::ServiceType::BOOTLOADER_SEG,
        };
        memcpy(frame.command_bytes, &segment.command_word, sizeof(frame.command_bytes));
        memcpy(frame.segment_bytes, segment.memory_words.data(), sizeof(frame.segment_bytes));

        // TODO: compute CRC
        frame.crc32 = 0U;

        const util::Span<const uint8_t> frame_bytes{reinterpret_cast<const uint8_t*>(&frame), sizeof(frame)};
        return stream_.write(frame_bytes) == frame_bytes.size();
    }

    // TODO: process multiple bytes at a time
    void receive() noexcept override {
        static_assert(sizeof(buffer_) >= kBufferSizeBytes, "buffer_ is too small");

        for (uint32_t i = 0U; i < kMaxReads; ++i) {
            const size_t bytes_read{stream_.read(util::Span<uint8_t>(buffer_, sizeof(buffer_)))};
            if (bytes_read == 0U) {
                break;
            }

            for (size_t j = 0U; j < bytes_read; ++j) {
                if (receiver_.process(util::Span<const uint8_t>(buffer_ + j, 1U))) {
                    dispatchPayload();
                }
            }
        }
    }

private:
    static constexpr uint32_t kMaxReads{3U};
    static constexpr size_t kBufferSizeBytes{kHdlcMaxPayloadBytes<kSegmentSizeWords>};
    static constexpr size_t kSegmentDataBytes{kSegmentTransferPayloadBytes<kSegmentSizeWords>};

    // Dispatch arbitrary packet type. buffer_[0] is the protocol type (not dispatched)
    // TODO: validate CRCs
    void dispatchPayload() noexcept {
        const size_t payload_len = receiver_.receivePayload(util::Span<uint8_t>(buffer_, sizeof(buffer_)));
        if (payload_len == 0U) {
            return;
        }

       switch (static_cast<hdlc::ServiceType>(buffer_[0])) {
        case hdlc::ServiceType::BOOTLOADER_CMD:
            dispatchCommand(payload_len);
            break;
        case hdlc::ServiceType::BOOTLOADER_SEG:
            dispatchSegment(payload_len);
            break;
        default:
            LOG_ERROR() << "Unknown protocol type: " << static_cast<uint32_t>(buffer_[0]);
            break;
       }
    }

    // Dispatch buffer_ contents as a command. buffer_[0] is the protocol type (not dispatched)
    // TODO: validate CRC
    void dispatchCommand(size_t payload_len) noexcept {
        // TODO: is this long enough for the CRC?
        if (payload_len < (1U + sizeof(CommandPacket::bytes))) {
            LOG_ERROR() << "Invalid command payload length: " << payload_len;
            return;
        }
        if (!command_received_callback_) {
            LOG_ERROR() << "No command received callback set";
            return;
        }
        CommandPacket command{};
        memcpy(&command, buffer_ + 1U, sizeof(command));
        command_received_callback_(command);
    }

    // Dispatch buffer_ contents as a transfer segment. buffer_[0] is the protocol type (not dispatched)
    // 3 bytes of padding are used to align the segment to a 4-byte boundary
    // TODO: validate CRC
    void dispatchSegment(size_t payload_len) noexcept {
        // TODO: is this long enough for the CRC?
        if (payload_len < (1U + kSegmentDataBytes)) {
            LOG_ERROR() << "Invalid segment payload length: " << payload_len;
            return;
        }
        if (!segment_received_callback_) {
            LOG_ERROR() << "No segment received callback set";
            return;
        }
        uint32_t memory_words[kSegmentSizeWords]{};
        SegmentTransferPacketView packet{};
        memcpy(&packet.command_word, buffer_ + 4U, sizeof(packet.command_word));
        memcpy(memory_words, buffer_ + 8U, sizeof(memory_words));
        packet.memory_words = util::Span<const uint32_t>(memory_words, kSegmentSizeWords);
        segment_received_callback_(packet);
    }

    Stream::StreamInterface& stream_;
    uint8_t buffer_[kBufferSizeBytes];
    hdlc::Receiver<kBufferSizeBytes> receiver_{};
    CommandReceivedCallback command_received_callback_{};
    SegmentReceivedCallback segment_received_callback_{};
};

} // namespace bootloader
