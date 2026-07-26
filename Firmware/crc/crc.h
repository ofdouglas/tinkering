#ifndef CRC_CRC_H
#define CRC_CRC_H

#include <stdint.h>
#include <type_traits>
#include <limits>

#include "data_structures/span.h"


template <typename T, T kPolynomial, T kInitial = 0, T kFinalXor = kInitial>
T crcBitwise(Span<T> input) {
    static_assert(std::is_integral<T>::value, "T must be an integral type");
    static_assert(std::is_unsigned<T>::value, "T must be an unsigned type");

    constexpr T kMsbBit = static_cast<T>((std::numeric_limits<T>::max() >> 1U) + 1U);
    T result = kInitial;

    for (auto x : input) {
        result ^= x;
        for (int i = 0; i < 8; i++) {
            if (result & kMsbBit) {
                result = (result << 1U) ^ kPolynomial;
            } else {
                result <<= 1U;
            }
        }
    }

    return result ^ kFinalXor;
}

uint8_t crcSaeJ1850(Span<uint8_t> input);

#endif // CRC_CRC_H
