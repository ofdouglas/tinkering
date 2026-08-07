#pragma once

#include <stdint.h>
#include <type_traits>
#include <limits>

#include "data_structures/span.h"

namespace crc {

/**
 * @brief  Specification for a CRC algorithm.
 *
 * @tparam T           The CRC integer type (e.g. uint16_t for CRC-16)
 * @tparam kPolynomial The polynomial of the CRC algorithm
 * @tparam kInitial    The initial value of the CRC algorithm
 * @tparam kXorOut     The XOR output value of the CRC algorithm
 * @tparam reflectIn   Whether to reflect the input data
 * @tparam reflectOut  Whether to reflect the output data
 */
template <typename T, T kPolynomial, T kInitial, T kXorOut, bool reflectIn, bool reflectOut>
struct Spec {
    static_assert(std::is_integral_v<T>, "T must be an integral type");
    static_assert(std::is_unsigned_v<T>, "T must be an unsigned type");

    using value_type = T;
    static constexpr T polynomial     = kPolynomial;
    static constexpr T initial        = kInitial;
    static constexpr T xorOut         = kXorOut;
    static constexpr bool reflect_in  = reflectIn;
    static constexpr bool reflect_out = reflectOut;
};

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
template <typename Spec>
typename Spec::value_type crcBitwise(Span<const uint8_t> input) {
    using T = typename Spec::value_type;
    static_assert(std::is_integral<T>::value, "T must be an integral type");
    static_assert(std::is_unsigned<T>::value, "T must be an unsigned type");

    constexpr T kMsbBit = static_cast<T>((std::numeric_limits<T>::max() >> 1U) + 1U);
    T result = Spec::initial;

    for (auto x : input) {
        result ^= x;
        for (int i = 0; i < 8; i++) {
            if (result & kMsbBit) {
                result = (result << 1U) ^ Spec::polynomial;
            } else {
                result <<= 1U;
            }
        }
    }

    return result ^ Spec::xorOut;
}


/*
 * @brief CRC specifications for various algorithms.
 *
 * @todo Add more specifications
 */
using SaeJ1850 = Spec<uint8_t, 0x1D, 0xFF, 0x3B, false, false>;


enum class CrcSpecTag {
    SaeJ1850,
};


} // namespace crc