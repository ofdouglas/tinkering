#pragma once
/*
 * @file  integer.h
 * @brief Integer utility types with range checks and serialization/deserialization.
 */

#include <cstdint>
#include <cstddef>
#include <optional>
#include <variant>

namespace util {

// Variant of all native uintN_t types. This is useful for specifying a specific integer width
// to use for some operation like comparison or a string hex dump, when the two operands may
// have different widths.
using UintVariant = std::variant<uint8_t, uint16_t, uint32_t, uint64_t>;

// Type-safe 24-bit unsigned integer in 32-bit container. This is intended for use in classes
// which represent a deserialized message type, to ensure that the value is always within the
// range of 24 bits.
struct Uint24_s {
    static constexpr uint32_t kMask = 0x00FF'FFFFU;
    static constexpr size_t kSizeBytes = 3U;

    uint32_t value{0U}; // 24 bits max

    static bool isValid(uint32_t value) noexcept {
        return (value & ~kMask) == 0U;
    }

    std::optional<uint32_t> serialize() const noexcept {
        return isValid(value) ? std::optional<uint32_t>(value) : std::nullopt;
    }

    bool deserialize(uint32_t wire) noexcept {
        if (!isValid(wire)) {
            return false;
        }
        value = wire & kMask;
        return true;
    }
};

// TODO: add generic bounded integer types (eg. templated way to make UintN inside of UintM container,
// for any N and M where N < M, and M = [8, 16, 32, 64]). It must have range checks and serialization/deserialization.
 
} // namespace util
 