#pragma once

#include <cstddef>

class StaticString {
public:
    constexpr StaticString(const char *data, size_t length) : data_(data), length_(length) {}

    constexpr const char *data() const { return data_; }
    constexpr size_t length() const { return length_; }

private:
    const char *data_;
    size_t length_;
};

inline namespace literals {
inline constexpr StaticString operator""_lit(const char *data, size_t length) {
    return StaticString{data, length};
}
} /* literals */

#define STATIC_STRING(s) StaticString((s), sizeof(s) - 1U)
