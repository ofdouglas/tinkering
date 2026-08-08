#pragma once
/*
 * @file  data_identifiers.h
 * @brief Data identifiers for the bootloader
 */

#include <cstdint>
#include <cstddef>

/*
 * @brief Data Identifier types for the bootloader, similar to UDS DID (Data Identifier).
 * 
 * @details Used to exchange data between the bootloader and the host or client. IDs have
 *          a 16-bit address and a 32-bit value. Not all addresses are supported. Some are
 *          read-only.
 *
 * @todo Design and implement a read-only attribute for the data identifiers.
 * @todo Rename to (lower-case) namespace bootloader::data_id {
 * @todo Consider organization / naming that more clearly handles value vs address (DID vs DID_VALUE)
 * @todo Document the data identifiers.
 * @todo Add more data identifiers
*/
namespace bootloader::data_id {

/******************************************************************************
 *  Data Identifier Value Types (32-bit value)
 *
 * @todo Can we use template inheritance to force sizeof() == 4 (one static_assert)?
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
 *  Data Identifier Address Types (16-bit address)
 ******************************************************************************/

// @todo consider renaming to DataIdentifierAddress
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
} // namespace bootloader::data_id