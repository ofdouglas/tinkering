#pragma once
/*
 * @file  crc_test_algorithms.h
 * @brief Runtime CRC algorithm handles for host-side CRC unit tests.
 * @note  Not intended for production / on-target use.
 */

#include <cstdint>

#include "crc/crc.h"
#include "util/integer.h"
#include "util/span.h"

namespace crc::test {

struct CrcAlgorithm {
    const char* name{};
    UintVariant (*compute_fn)(Span<const uint8_t> input){};
    UintVariant (*wrap_fn)(uint64_t expected){};

    UintVariant compute(Span<const uint8_t> input) const { return compute_fn(input); }

    UintVariant wrapExpected(uint64_t expected) const { return wrap_fn(expected); }
};

template <typename Derived>
constexpr CrcAlgorithm makeCrcAlgorithm() {
    return CrcAlgorithm{
        Derived::name(),
        [](Span<const uint8_t> input) -> UintVariant { return crc::details::crcBitwise<Derived>(input); },
        [](uint64_t expected) -> UintVariant {
            return static_cast<typename Derived::value_type>(expected);
        },
    };
}

constexpr CrcAlgorithm kSaeJ1850Algorithm    = makeCrcAlgorithm<crc::SaeJ1850>();
constexpr CrcAlgorithm kAutosarCrc8Algorithm = makeCrcAlgorithm<crc::AutosarCrc8>();
// constexpr CrcAlgorithm kCrc16CcittAlgorithm = makeCrcAlgorithm<crc::Crc16Ccitt>();

} // namespace crc::test
