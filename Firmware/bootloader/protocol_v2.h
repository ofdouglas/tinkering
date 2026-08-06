#pragma once

#include <stdint.h>
#include <climits>

#include "data_structures/span.h"

/* Transport-agnostic Bootloader for 32-bit MCUs
    - Supports image transfers of [1, 2^24) 32-bit words (64 MB max)
    - Transfer to / from arbitrary addresses
    - All messages fit in 8 bytes except transfer segments, which are (N) 32-bit data words + 1 32-bit command word
    - Data identifiers with 16-bit address and 32-bit value
    - All packets start with {reserved:2, command_type:6}
    - Little endian encoding
*/
namespace Bootloader {

/* Example Sequences:

   Data Identifier Sequence:
   Client -> Bootloader:                Bootloader -> Client:
    - kReadDataIdentifier (id)          - kReadDataIdentifier (id, value)
    - kWriteDataIdentifier (id, value)  - kWriteDataIdentifier (id, value)
    ------------------------------------------------------------
    - kReadDataIdentifier (id)          - kCommandFailed (cmdType=kReadDataIdentifier, errorCode=kInvalidDataIdentifier)
    - kWriteDataIdentifier (id, value)  - kCommandFailed (cmdType=kWriteDataIdentifier, errorCode=kWriteOnlyDataIdentifier)

   App Erase and Download Sequence:
   Client -> Bootloader:            Bootloader -> Client:
    - kPrepareErase                     - kCommandSuccess
    - kStartErase                       - kCommandPending ...
    - ...                               - kCommandSuccess
    ------------------------------------------------------------
    - kPrepareAppDownload               - kCommandSuccess
    - kStartAppDownload                 - kCommandSuccess
    - kSegmentTransfer                  - kSegmentAck ...
    - ...                               - kSegmentNak ...  // TODO: handle retries
    - kEndAppDownload                   - kCommandPending ... (Bootloader verifies image)
    - ...                               - kCommandSuccess  or kCommandFailed (errorCode=kInvalidImageCrc or kInvalidHeaderCrc)
    ------------------------------------------------------------
    - kBootApplication                  - kCommandSuccess

   App Erase and Download Error Sequences:
   Client -> Bootloader:            Bootloader -> Client:
    - kPrepareErase                     - kCommandFailed (cmdType=kPrepareErase, errorCode=kInvalidMemAddress)
    - kStartAppDownload                 - kCommandFailed (cmdType=kStartAppDownload, errorCode=kNotPrepared)
    - kEndAppDownload                   - kCommandFailed (cmdType=kEndAppDownload, errorCode=kInvalidImageCrc)
    - kSegmentTransfer                  - kCommandFailed (cmdType=kSegmentTransfer, errorCode=kWriteFailed)
*/


/******************************************************************************
 *  Information Message Types (Data Identifiers: 16-bit address, 32-bit value)
 ******************************************************************************/

 struct BootloaderInfo1 {
    static constexpr uint8_t kMagic = 0xB1;
    uint8_t magic; // Must match kMagic
    uint8_t protocol_version; // Valid range = [1, 255]
    uint8_t bootloader_major; // Valid range = [1, 255]
    uint8_t bootloader_minor; // Valid range = [0, 255]
};

static_assert(sizeof(BootloaderInfo1) == 4U, "BootloaderInfo1 size is incorrect");

// Status of application A and B images. B is not guaranteed to be supported.
struct ImageStatus1 {
    uint8_t status_a;  // ImageStatus enum
    uint8_t status_b;  // ImageStatus enum
    uint8_t reserved[2];
};

static_assert(sizeof(ImageStatus1) == 4U, "ImageStatus1 size is incorrect");

struct BoardVersion1 {
    uint16_t board_type;          // Valid range = [1, 65535]
    uint8_t  board_version_major; // Valid range = [1, 255]
    uint8_t  board_version_minor; // Valid range = [0, 255]
};

static_assert(sizeof(BoardVersion1) == 4U, "BoardVersion1 size is incorrect");

struct AppVersion1 {
    uint8_t  app_version_major; // Valid range = [1, 255]
    uint8_t  app_version_minor; // Valid range = [0, 255]
    uint8_t  reserved[2];
};

static_assert(sizeof(AppVersion1) == 4U, "AppVersion1 size is incorrect");


/******************************************************************************
 *  Enums
 ******************************************************************************/

 enum class ImageStatus : uint8_t {
    kUnknown = 0,
    kNotSupported = 1,
    kInvalid = 2,
    kErased = 3,
    kBusy = 4,
    kValid = 5,
    kNumImageStatus = 6
 };

// Similar to UDS DID (Data Identifier) or CAN Open Data Dictionary ID
enum class DataIdentifier : uint16_t {
    kUnknown = 0,
    kBootloaderInfo1 = 1,
    kImageStatus1 = 2,
    kBoardVersion1 = 3,
    kAppVersion1 = 4,
    // TODO: add more data identifiers
    kNumDataIdentifiers = 5
};

inline bool isValidDataIdentifier(DataIdentifier data_identifier) noexcept {
    static_assert(static_cast<uint16_t>(DataIdentifier::kUnknown) == 0U, "DataIdentifier::kUnknown must be 0");
    return (data_identifier != DataIdentifier::kUnknown) && (data_identifier < DataIdentifier::kNumDataIdentifiers);
}

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

    // TODO: Add more error codes
    kNumErrorCodes = 15
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


/******************************************************************************
 *  Packet Types
 ******************************************************************************/

 struct GeneralCommandWireFormat {
    uint8_t  reserved1    : 2;
    uint8_t  command_type : 6;
    uint8_t  reserved2    : 8;
    uint16_t value16      : 16;
    uint32_t value32      : 32;
};

struct GeneralCommandDecoded {
    CommandType command_type;
    uint16_t    value_16;
    uint32_t    value32;
};

struct GeneralCommandEncoded {
    uint8_t command[8];
};

static_assert(sizeof(GeneralCommandEncoded) == 8U, "GeneralCommand size is incorrect");

struct MemoryCommandWireFormat {
    uint8_t  reserved1    : 2;
    uint8_t  command_type : 6;
    uint32_t size_words   : 24; // Number of 32-bit words to transfer
    uint32_t start_word   : 30; // Starting word address
    uint8_t  reserved2    : 2;
};

struct MemoryCommandDecoded {
    CommandType command_type;
    uint32_t    size_words; // Number of 32-bit words to transfer
    uint32_t    start_word; // Starting word address
};

struct MemoryCommandEncoded {
    uint8_t command[8];
};

static_assert(sizeof(MemoryCommandEncoded) == 8U, "MemoryCommand size is incorrect");

// No CRC: it is assumed this fits in 1 packet and the link layer provides a CRC.
// The full image is verified after transfer is complete.
template<size_t kSegmentSizeWords>
struct SegmentTransferDecoded {
    CommandType command_type;
    uint32_t    segment_number;
    uint32_t    memory[kSegmentSizeWords];
};

template<size_t kSegmentSizeWords>
struct SegmentTransferEncoded {
    uint8_t  reserved1      : 2;
    uint8_t  command_type   : 6;
    uint32_t segment_number : 24; 
    uint32_t memory[kSegmentSizeWords];
};

/******************************************************************************
 *  Packet Helpers
 ******************************************************************************/

// Arbitrary segment size so the bootloader is independent of segment size template argument
// This is for bootloader internal use only, NOT a packet sent on the wire.
struct SegmentTransferView {
    CommandType command_type;
    uint32_t    segment_number;
    Span<const uint32_t> memory;
};

} // namespace Bootloader
