#pragma once
/*
 * @file  protocol.h
 * @brief HDLC framing protocol definitions.
 */

#include <cstdint>
#include <cstddef>
#include <type_traits>

#include "util/span.h"

namespace hdlc {

// All HDLC frames (all service types) start with this 4-byte header:
struct HdlcFrameHeader {
  uint8_t  service_type;
  uint8_t  link_control;
  uint16_t crc16;
};

static_assert(sizeof(HdlcFrameHeader) == 4U, "HdlcFrameHeader size is incorrect");


// Defines the type of the payload / application protocol
enum class ServiceType : uint8_t {
    ASCII_TEXT      = 0xF0, // Raw text stream
    BOOTLOADER_CMD  = 0xE1, // 8 bytes (GeneralCommand or MemoryCommand encoded) + CRC-8
    BOOTLOADER_SEG  = 0xD2, // MemTransferSegment<64U> + CRC-32
};


} // namespace hdlc
