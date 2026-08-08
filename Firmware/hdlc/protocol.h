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

  uint16_t crc16; // Calculated over the rest of the header and the full payload
  uint8_t  service_type;
  uint8_t  link_control;
};

static_assert(sizeof(FrameHeader) == 4U, "FrameHeader size is incorrect");

/**
 * @brief Wire `service_type` byte ranges (see hdlc/design.md).
 *
 * Ranges are inclusive [kMin, kMax]. ID 0 is forbidden on the wire.
 */
namespace service_id {

constexpr uint8_t kForbidden{0U};

constexpr uint8_t kIntrinsicMin{0x01U};
constexpr uint8_t kIntrinsicMax{0x0FU};

constexpr uint8_t kCommonMin{0x10U};
constexpr uint8_t kCommonMax{0x2FU};

constexpr uint8_t kReservedMin{0x30U};
constexpr uint8_t kReservedMax{0x9FU};

constexpr uint8_t kProjectStableMin{0xA0U};
constexpr uint8_t kProjectStableMax{0xDFU};

constexpr uint8_t kProjectEphemeralMin{0xE0U};
constexpr uint8_t kProjectEphemeralMax{0xFFU};

constexpr uint8_t kWireMin{kIntrinsicMin};
constexpr uint8_t kWireMax{kProjectEphemeralMax};

// Well-known IDs (registries in design.md)
constexpr uint8_t kNetworkManagement{kIntrinsicMin};
constexpr uint8_t kBootloaderCommand{kCommonMin};
constexpr uint8_t kBootloaderSegment{0x11U};

static_assert(kIntrinsicMax - kIntrinsicMin + 1U == 15U, "intrinsic service ID count");
static_assert(kCommonMax - kCommonMin + 1U == 32U, "common service ID count");
static_assert(kReservedMax - kReservedMin + 1U == 112U, "reserved service ID count");
static_assert(kProjectStableMax - kProjectStableMin + 1U == 64U, "project-stable service ID count");
static_assert(kProjectEphemeralMax - kProjectEphemeralMin + 1U == 32U, "project-ephemeral service ID count");
static_assert(kProjectStableMax + 1U == kProjectEphemeralMin, "stable and ephemeral bands are adjacent");

enum class Category : uint8_t {
    kForbidden,
    kIntrinsic,
    kCommon,
    kReserved,
    kProjectStable,
    kProjectEphemeral,
};

constexpr Category category(uint8_t id) noexcept {
    if (id == kForbidden) {
        return Category::kForbidden;
    }
    if (id >= kIntrinsicMin && id <= kIntrinsicMax) {
        return Category::kIntrinsic;
    }
    if (id >= kCommonMin && id <= kCommonMax) {
        return Category::kCommon;
    }
    if (id >= kReservedMin && id <= kReservedMax) {
        return Category::kReserved;
    }
    if (id >= kProjectStableMin && id <= kProjectStableMax) {
        return Category::kProjectStable;
    }
    if (id >= kProjectEphemeralMin && id <= kProjectEphemeralMax) {
        return Category::kProjectEphemeral;
    }
    return Category::kForbidden;
}

constexpr bool isForbidden(uint8_t id) noexcept { return id == kForbidden; }

constexpr bool isIntrinsic(uint8_t id) noexcept { return category(id) == Category::kIntrinsic; }

constexpr bool isCommon(uint8_t id) noexcept { return category(id) == Category::kCommon; }

constexpr bool isReserved(uint8_t id) noexcept { return category(id) == Category::kReserved; }

constexpr bool isProjectStable(uint8_t id) noexcept { return category(id) == Category::kProjectStable; }

constexpr bool isProjectEphemeral(uint8_t id) noexcept { return category(id) == Category::kProjectEphemeral; }

constexpr bool isWireRange(uint8_t id) noexcept {
    return id >= kWireMin && id <= kWireMax;
}

} // namespace service_id

// Registered services in this codebase (wire values match service_id::* constants).
enum class ServiceType : uint8_t {
    kUnknown = service_id::kForbidden,
    kNetworkManagement = service_id::kNetworkManagement,
    kBootloaderCommand = service_id::kBootloaderCommand,
    kBootloaderSegment = service_id::kBootloaderSegment,
};

constexpr bool isRegisteredServiceType(ServiceType service_type) noexcept {
    switch (service_type) {
        case ServiceType::kNetworkManagement:
        case ServiceType::kBootloaderCommand:
        case ServiceType::kBootloaderSegment:
            return true;
        default:
            return false;
    }
}

// TODO: use a generic class for type-safe bitfields.
// LinkControl bitfields:
//  - Bit {0:3} flow_control : floor(log2(receiver_rx_buffer_free_words))
//  - Bit {4:7} extensions   : Bitfield of optional header extensions; zero by default
class LinkControl { /* TODO */ };

} // namespace hdlc
