#pragma once

#include <stdint.h>
#include <type_traits>
#include <limits>

#include "util/span.h"
#include "util/static_string.h"

namespace crc::details {

// CRC algorithm name string
// TODO: detect truncation / name overflow
static constexpr size_t kMaxNameLength = 16U;
using NameString = StaticString<kMaxNameLength>;

/**
 * @brief Calculate the CRC of a given input data using the bitwise algorithm.
 *
 * @tparam Spec  The CRC specification.
 * @param  input The input data to calculate the CRC of.
 * @return The   CRC of the input data.
 *
 * @todo Handle reflect_in and reflect_out
 * @todo Support incremental processing (update, ... finalize)
 */
 template <typename Derived>
 typename Derived::value_type crcBitwise(Span<const uint8_t> input) {
    using T = typename Derived::value_type;
    static_assert(std::is_integral<T>::value, "T must be an integral type");
    static_assert(std::is_unsigned<T>::value, "T must be an unsigned type");

    constexpr T kMsbBit = static_cast<T>((std::numeric_limits<T>::max() >> 1U) + 1U);
    T result = Derived::initial;

    for (auto x : input) {
        result ^= x;
        for (int i = 0; i < 8; i++) {
            if (result & kMsbBit) {
                result = (result << 1U) ^ Derived::polynomial;
            } else {
                result <<= 1U;
            }
        }
    }

    return result ^ Derived::xorOut;
}

/**
 * @brief  Specification for a CRC algorithm.
 *
 * @tparam Derived The derived class type (a specific CRC algorithm)
 * @tparam T       The CRC integer type (e.g. uint16_t for CRC-16)
 * @tparam kPoly   The polynomial of the CRC algorithm
 * @tparam kInit   The initial value of the CRC algorithm
 * @tparam kXorOut The XOR output value of the CRC algorithm
 * @tparam refIn   Whether to reflect the input data
 * @tparam refOut  Whether to reflect the output data
 */
template <typename Derived, typename T, T kPoly, T kInit, T kXorOut, bool refIn, bool refOut>
struct SpecImpl {
    static_assert(std::is_integral_v<T>, "T must be an integral type");
    static_assert(std::is_unsigned_v<T>, "T must be an unsigned type");

    using value_type = T;
    static constexpr T polynomial     = kPoly;
    static constexpr T initial        = kInit;
    static constexpr T xorOut         = kXorOut;
    static constexpr bool reflect_in  = refIn;
    static constexpr bool reflect_out = refOut;

    // TODO: return a StringView instead?
    static constexpr const char* name() { return Derived::name(); }

    // Implementation defined via link-seam injection?
    static constexpr value_type compute(Span<const uint8_t> input) {
        return details::crcBitwise<Derived>(input);
    }

    // TODO: support incremental processing (update, ... finalize)
};

} // namespace crc::details

namespace crc {

struct SaeJ1850 : details::SpecImpl<SaeJ1850, uint8_t, 0x1D, 0xFF, 0xFF, false, false> {
    static constexpr const char* name() { return "SaeJ1850"; }
};

struct AutosarCrc8 : details::SpecImpl<AutosarCrc8, uint8_t, 0x2F, 0xFF, 0xFF, false, false> {
    static constexpr const char* name() { return "AutosarCrc8"; }
};

// struct Crc16Ccitt : details::SpecImpl<Crc16Ccitt, uint16_t, 0x1021, 0xFFFF, 0x0000, true, true> {
//     static constexpr const char* name() { return "Crc16Ccitt"; }
// };

} // namespace crc
