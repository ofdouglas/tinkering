#pragma once
/*
 * @file  protocol.h
 * @brief Bootloader protocol definitions
 */

#include <cstdint>
#include <cstddef>
#include <variant>
#include <optional>

#include "util/integer.h"
#include "util/span.h"

// @todo rename to (lower-case) namespace bootloader::protocol {
namespace bootloader {

static constexpr uint8_t kProtocolVersion{1U};

/******************************************************************************
 *  Enums
 ******************************************************************************/

// Status of the image in a memory region, if there is one.
enum class ImageStatus : uint8_t {
    kUnknown = 0,
    kNotSupported = 1,  // The given region 
    kInvalid = 2,       // The image is invalid (CRC error, etc)
    kErased = 3,        // The region is erased (ready for new image)
    kBusy = 4,          // The region is not available (ex: during flash erase) // TODO: this may be unnecessary
    kValid = 5,         // The image is valid and may be booted.
    kNumImageStatus = 6
};

// Returned via a GeneralCommand kCommandFailed response
enum class ErrorCode : uint32_t {
    kUnknown = 0,
    // Data Identifier errors
    kInvalidDataIdentifier = 1,
    kWriteOnlyDataIdentifier = 2,

    // Memory command errors
    kInvalidMemAddress = 3,
    kInvalidMemSize = 4,
    kWriteFailed = 5,
    kTransferTimeout = 6,
    // Start command without Prepare command, or different size or address than Prepare command
    kNotPrepared = 7,

    // Image verification errors
    kInvalidImageFormat = 8,
    kInvalidImageHeaderVersion = 9,
    kInvalidBoardType = 10,
    kInvalidBoardVersion = 11,
    kInvalidAppVersion = 12,
    kInvalidImageCrc = 13,
    kInvalidHeaderCrc = 14,

    kNotInTransferState = 15,
    kNoActiveRegion = 16,

    // TODO: Add more error codes
    kNumErrorCodes
};

inline bool isValidErrorCode(ErrorCode error_code) noexcept {
    static_assert(static_cast<uint32_t>(ErrorCode::kUnknown) == 0U, "ErrorCode::kUnknown must be 0");
    return (error_code != ErrorCode::kUnknown) && (error_code < ErrorCode::kNumErrorCodes);
}

// There are three command formats:
// 1. General Command Format   (8 bytes)
// 2. Memory Command Format    (8 bytes)
// 3. Segment Transfer Format  (N+1 32-bit words)
enum class CommandType : uint8_t {
    kUnknown = 0,

    ////////////////////////////////////////////////////////////////////////////
    // General Commands (value16 : 16, value32 : 32)
    ////////////////////////////////////////////////////////////////////////////
    // Bidirectional information messages.
    kReadDataIdentifier,  // value16 = DataIdentifier, value32 = value
    kWriteDataIdentifier, // value16 = DataIdentifier, value32 = value

    // Client -> Bootloader Command Messages
    kReset,
    kBootApplication,

    // Memory Command Response messages (in General Command Format)
    kCommandSuccess,    // value16 = command type
    kCommandPending,    // value16 = command type
    kCommandFailed,     // value16 = command type, value32 = ErrorCode
    kSegmentAck,        // value32 = segment number (max 24 bits)
    kSegmentNak,        // value32 = segment number (max 24 bits)

    ///////////////////////////////////////////////////////////////////////////
    // Memory Commands (sizeWords : 24, startWord : 30)
    ///////////////////////////////////////////////////////////////////////////
    // Client -> Bootloader Transfer Command Messages
    // Prepare / Start sequence: both commands must have same size and address.
    // This is a simple protection against communication / command errors.
    kPrepareErase,
    kStartErase,
    kPrepareAppDownload,
    kStartAppDownload,
    kEndAppDownload,
    // TODO: handle data file transfers (upload and download)

    ///////////////////////////////////////////////////////////////////////////
    // Segment Transfer (segmentNumber : 24, memory : N*32)
    ///////////////////////////////////////////////////////////////////////////
    kSegmentTransfer,

    kNumCommandTypes
};

static_assert(static_cast<uint8_t>(CommandType::kNumCommandTypes) < 64U, "CommandType will not fit in 6 bits");

inline bool isValidCommandType(CommandType command_type) noexcept {
    static_assert(static_cast<uint8_t>(CommandType::kUnknown) == 0U, "CommandType::kUnknown must be 0");
    return (command_type != CommandType::kUnknown) && (command_type < CommandType::kNumCommandTypes);
}

inline bool isGeneralCommand(CommandType command_type) noexcept {
    return (command_type >= CommandType::kReadDataIdentifier) && (command_type <= CommandType::kSegmentNak);
}

inline bool isMemoryCommand(CommandType command_type) noexcept {
    return (command_type >= CommandType::kPrepareErase) && (command_type <= CommandType::kEndAppDownload);
}

inline bool isSegmentTransferCommand(CommandType command_type) noexcept {
    return (command_type == CommandType::kSegmentTransfer);
}

/******************************************************************************
 *  Serialized Packet Types (for transport layer)
 *
 *  - No CRC: it is assumed that all packet types fit in 1 link-layer frame, and that the
 *    link-layer provides a CRC. The full image is verified after the transfer is complete.
 ******************************************************************************/

// GeneralCommand or MemoryCommand
struct CommandPacket {
    uint8_t bytes[8U];
};

// Image data (application or data file (not implemented yet))
template<size_t kSegmentSizeWords>
struct SegmentTransferPacket {
    // command_bytes: {command_type:6, segment_number:24}
    uint8_t  command_bytes[4U];
    uint8_t  memory_bytes[kSegmentSizeWords * 4U];
};


/******************************************************************************
 *  Deserialized Message Types in firmware (NOT sent on the wire)
 ******************************************************************************/

// General-purpose command or response. Sent by the bootloader or the client.
struct GeneralCommand {
    // Wire type: CommandPacket (8 bytes, little-endian)
    // bytes[0]   = {reserved1: 2, command_type : 6}
    // bytes[1]   = {reserved2: 8}
    // bytes[2:3] = {value16: 16}
    // bytes[4:7] = {value32: 32}

    // Command fields
    CommandType command_type;
    uint16_t    value16;
    uint32_t    value32;

    bool isValid() const noexcept;
    std::optional<CommandPacket> encode() const noexcept;
    static std::optional<CommandPacket> createEncoded(CommandType command_type, uint16_t value16, uint32_t value32) noexcept;
    static std::optional<GeneralCommand> decode(const CommandPacket& command) noexcept;
};

// Memory operation command. Sent by the client to the bootloader. The
// bootloader's responses to these commands are sent as GeneralCommands.
struct MemoryCommand {
    // Wire type: CommandPacket (8 bytes, little-endian)
    // bytes[0]   = {reserved1: 2, command_type : 6}
    // bytes[1:3] = {size_words: 24}
    // bytes[4:7] = {reserved2: 2, start_word: 30}

    // Command fields
    CommandType command_type;
    util::Uint24_s size_words; // Number of 32-bit words to transfer
    uint32_t    start_word; // Starting word address

    bool isValid() const noexcept;
    std::optional<CommandPacket> encode() const noexcept;
    static std::optional<CommandPacket> createEncoded(CommandType command_type, uint32_t size_words, uint32_t start_word) noexcept;
    static std::optional<MemoryCommand> decode(const CommandPacket& command) noexcept;
};

// The 4-byte header of a segment transfer packet.
struct SegmentTransferCommandWord {
    // command_bytes: {reserved:2, command_type:6, segment_number:24}
    uint8_t  command_type;
    util::Uint24_s segment_number;

    bool isValid() const noexcept;
};

// Arbitrary segment size so the bootloader is independent of segment size template argument
struct SegmentTransferPacketView {
    // command_bytes: {reserved:2, command_type:6, segment_number:24}
    uint32_t  command_word;
    util::Span<const uint32_t> memory_words;

    static std::optional<SegmentTransferCommandWord> decodeCommandWord(uint32_t command_word) noexcept;
};


/******************************************************************************
 *  Packet Helpers
 ******************************************************************************/

template<size_t kSegmentSizeWords>
constexpr size_t kSegmentTransferPayloadBytes = 4U + (kSegmentSizeWords * 4U);

static constexpr size_t kDefaultHdlcSegmentSizeWords = 16U;

using CommandVariant = std::variant<GeneralCommand, MemoryCommand>;

CommandType decodeCommandByte(uint8_t first_byte) noexcept;
uint8_t encodeCommandByte(CommandType command_type) noexcept;
std::optional<CommandVariant> decodeCommand(const CommandPacket& command) noexcept;

} // namespace bootloader
