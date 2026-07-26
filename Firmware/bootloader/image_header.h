#ifndef IMAGE_HEADER_H
#define IMAGE_HEADER_H

#include <stdint.h>
#include <stddef.h>

namespace Bootloader {

enum class ImageType : uint8_t {
    kUnknown = 0,
    kBootloader = 1,
    kBootupdater = 2,
    kApplication = 3,
    kDataFile = 4,
    kNumTypes = 5,
};

enum class CpuType : uint8_t {
    kUnknown = 0,
    kCortexM0 = 1,
    kCortexM3 = 2,
    kCortexM4 = 3,
    kCortexM7 = 4,
    kRiscV32I = 5,
};

/**
 *  Image Descriptor Structure -- What the image is used for and
 *  what hardware it is compatible with.
 */
struct ImageDescriptor {
    uint8_t  image_type;
    uint8_t  cpu_type;
    uint8_t  board_revision[2];
    uint8_t  board_type[4];
};

static_assert(sizeof(ImageDescriptor) == 8U, "ImageDescriptor size is incorrect");

struct ImageVersion {
    uint8_t build_type;
    uint8_t major;
    uint8_t minor;
    uint8_t patch;
    uint8_t git_sha[4];
};

static_assert(sizeof(ImageVersion) == 8U, "ImageVersion size is incorrect");

/**
 * Image Header v1.0
 */
struct ImageHeader {
    static constexpr uint32_t kMagic = 0x424F4F54; // "BOOT"
    uint32_t magic;

    // Start of the header structure. CRC and size are measured from here.
    uint8_t  header_version[2];
    uint16_t header_size_bytes;

    ImageDescriptor image_descriptor;
    ImageVersion    image_version;

    uint32_t image_size_bytes;
    uint32_t image_lma_address;
    uint32_t image_crc;

    // CRC is calculated over all other fields starting at header_version.
    uint32_t header_crc;
};

static_assert(sizeof(ImageHeader) == 40U, "ImageHeader size is incorrect");


} // namespace Bootloader

#endif // IMAGE_HEADER_H