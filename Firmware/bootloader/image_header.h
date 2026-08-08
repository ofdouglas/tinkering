#pragma once

#include <stdint.h>
#include <stddef.h>

/**
 *  Notes:
 *   - All multi-byte fields are little-endian.
 *   - All CRCs are CRC-32/ISO-HDLC unless otherwise specified.
 */
namespace bootloader {

/******************************************************************************
 *  Common Types
 ******************************************************************************/

enum class ImageType : uint8_t {
    kUnknown = 0,
    kBootloader = 1,
    kBootupdater = 2,
    kApplication = 3,
    kDataFile = 4,
    kNumTypes = 5,
};

enum class BoardType : uint16_t {
    kUnknown = 0,
    kStm32F7Discovery = 1,
    kDigilentNexysVideo = 2,
    kNumBoards = 3,
};

//  All ImageHeader versions start with this structure.
struct HeaderStart {
    static constexpr uint32_t kMagic = 0x424F4F54; // "BOOT"
    uint32_t magic; // Must be kMagic

    // Together these define what header structure follows.
    uint8_t  image_type;     // ImageType enum
    uint8_t  header_version; // Valid versions are in the range [1, 255].

    // Size of the remaining header; does not count HeaderStart itself.
    uint16_t header_size_bytes;
};

constexpr size_t kImageTypeOffset = offsetof(HeaderStart, image_type);
constexpr size_t kHeaderVersionOffset = offsetof(HeaderStart, header_version);
constexpr size_t kHeaderSizeBytesOffset = offsetof(HeaderStart, header_size_bytes);

static_assert(sizeof(HeaderStart) == 8U, "HeaderStart size is incorrect");


/******************************************************************************
 *  Application Image Header Types
 ******************************************************************************/

// image_type == ImageType::kApplication, header_version == 1
struct ImageHeader_Application_v1 {
    HeaderStart header_start;

    // BoardType enum and major.minor version
    uint16_t board_type;          // Valid range is [1, 65535]
    uint8_t  board_version_major; // Valid range is [1, 255]
    uint8_t  board_version_minor; // Valid range is [0, 255]

    // Application version information
    uint8_t  app_version_major; // Valid range is [1, 255]
    uint8_t  app_version_minor; // Valid range is [0, 255]

    // Padding for explicit alignment
    uint8_t  reserved[2];

    // Image information
    uint32_t image_size_bytes;
    uint32_t image_lma_address;
    uint32_t image_entry_address;
    uint32_t image_crc;

    // CRC is calculated over all other fields starting at header_start.
    uint32_t header_crc;
};

static_assert(sizeof(ImageHeader_Application_v1) == 36U, "ImageHeader_Application_v1 size is incorrect");


/******************************************************************************
 *  Data File Image Header Types
 ******************************************************************************/

 // image_type == ImageType::kDataFile, header_version == 1
struct ImageHeader_DataFile_v1 {
    HeaderStart header_start;

    // Image information
    uint32_t image_size_bytes;
    uint32_t image_start_address;
    uint32_t image_crc;

    // CRC is calculated over all other fields starting at header_start.
    uint32_t header_crc;
};

static_assert(sizeof(ImageHeader_DataFile_v1) == 24U, "ImageHeader_DataFile_v1 size is incorrect");


} // namespace bootloader