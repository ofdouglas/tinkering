#pragma once
/*
 * @file  protocol.h
 * @brief Custom link-layer protocol using HDLC framing.
 */

#include <cstdint>
#include <cstddef>
#include <type_traits>

#include "crc/crc_algorithm.h"
#include "util/span.h"

namespace hdlc {

// All HDLC frames (all service types) start with this 4-byte header:
struct FrameHeader {
  using CrcAlgorithm = crc::algorithm::Crc16CcittFalse;

  uint8_t  service_type;
  uint8_t  link_control;
  uint16_t crc16; // Calculated over {service_type | link_control | payload}
};

static_assert(sizeof(FrameHeader) == 4U, "FrameHeader size is incorrect");

// Defines the type of the payload / application protocol
// TODO: inverted nibble encoding may be unnecessary. Some bits could be used for other purposes.
enum class ServiceType : uint8_t {
    ASCII_TEXT      = 0xF0, // Raw text stream
    BOOTLOADER_CMD  = 0xE1, // 8 bytes (GeneralCommand or MemoryCommand encoded) + CRC-8
    BOOTLOADER_SEG  = 0xD2, // MemTransferSegment<64U> + CRC-32
    // TODO: add more service types once they are defined.
};

// TODO: use a generic class for type-safe bitfields.
// LinkControl bitfields:
//  - Bit {0:3} flow_control : floor(log2(receiver_rx_buffer_free_words))
//  - Bit {4:7} reserved     : reserved / To Be Determined
class LinkControl { /* TODO */ };

} // namespace hdlc
