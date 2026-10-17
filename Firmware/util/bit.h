#pragma once
/*
 * @file  bit.h
 * @brief Bit-manipulation helpers for C++17 (equivalents of C++20 <bit>).
 */

#include <type_traits>

namespace util {

/**
 * @brief Number of bits needed to store value.
 *
 * Equivalent to C++20 std::bit_width: returns 0 if value is 0, otherwise
 * 1 + floor(log2(value)).
 *
 * @tparam T Unsigned integer type (not bool).
 * @param[in] value The value to measure.
 * @return Bit width of value.
 */
template <typename T>
constexpr int bitWidth(T value) noexcept {
    static_assert(std::is_integral_v<T>, "T must be an integral type");
    static_assert(std::is_unsigned_v<T>, "T must be an unsigned type");
    static_assert(!std::is_same_v<T, bool>, "T must not be bool");

    int width{0};
    while (value != T{0}) {
        value >>= 1;
        ++width;
    }
    return width;
}

}  // namespace util
