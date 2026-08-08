#pragma once
/*
 * @file  crc_test_algorithms.h
 * @brief Runtime CRC algorithm handles for host-side CRC unit tests.
 * @note  Not intended for production / on-target use.
 */

#include <cstdint>

#include "crc/crc_algorithm.h"
#include "util/integer.h"
#include "util/span.h"

namespace crc::test {

struct CrcAlgorithm {
    const char* name{};
    util::UintVariant (*compute_fn)(util::Span<const uint8_t> input){};
    util::UintVariant (*wrap_fn)(uint64_t expected){};

    util::UintVariant compute(util::Span<const uint8_t> input) const { return compute_fn(input); }

    util::UintVariant wrapExpected(uint64_t expected) const { return wrap_fn(expected); }
};

template <typename Derived>
constexpr CrcAlgorithm makeCrcAlgorithm() {
    return CrcAlgorithm{
        Derived::name(),
        [](util::Span<const uint8_t> input) -> util::UintVariant { return crc::details::crcBitwise<Derived>(input); },
        [](uint64_t expected) -> util::UintVariant {
            return static_cast<typename Derived::value_type>(expected);
        },
    };
}

constexpr CrcAlgorithm kSaeJ1850Algorithm = makeCrcAlgorithm<crc::algorithm::SaeJ1850>();
constexpr CrcAlgorithm kAutosarCrc8Algorithm = makeCrcAlgorithm<crc::algorithm::AutosarCrc8>();
constexpr CrcAlgorithm kCrc16CcittFalseAlgorithm = makeCrcAlgorithm<crc::algorithm::Crc16CcittFalse>();
constexpr CrcAlgorithm kCrc32Mpeg2Algorithm = makeCrcAlgorithm<crc::algorithm::Crc32Mpeg2>();

} // namespace crc::test
