#pragma once
/*
 * @file  ostream_helpers.h
 * @brief Helpers for std::ostream printing, for use in host-side unit tests.
 * @note  This file is not intended for use in production code.
*/

#include <iostream>
#include <iomanip>
#include <vector>

#include "util/integer.h"
#include "util/span.h"

// RAII guard for restoring the original stream flags
class IostreamRaiiFlagsRestorer {
    public:
        explicit IostreamRaiiFlagsRestorer(std::ostream& os) : os_(os), f_(os.flags()) {}
        ~IostreamRaiiFlagsRestorer() { os_.flags(f_); }
    
    private:
        std::ostream& os_;
        std::ios_base::fmtflags f_;
};

// Print a hex dump of a span of bytes. Format ex: 42 01 2A
inline std::ostream& operator<<(std::ostream& os, const Span<const uint8_t>& span) {
    IostreamRaiiFlagsRestorer flags_restorer{os};

    for (uint8_t byte : span) {
        os << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << static_cast<int>(byte) << " ";
    }
    return os;
}

// Print a hex dump of a vector of bytes. Format ex: 42 01 2A
inline std::ostream& operator<<(std::ostream& os, const std::vector<uint8_t>& vector) {
    return os << Span<const uint8_t>{vector.data(), vector.size()};
}

// Print a UintVariant as a zero-padded hex string of correct width for the type
// Format ex: 0xFE, 0x002A, 0xDEADBEEF
inline std::ostream& operator<<(std::ostream& os, const UintVariant& uint_variant) {
    IostreamRaiiFlagsRestorer flags_restorer{os};

    std::visit([&os](auto&& value) -> void {
        using T = std::decay_t<decltype(value)>;
        os << "0x" << std::hex << std::uppercase << std::setfill('0') << std::setw(sizeof(T) * 2);

        if constexpr (std::is_same_v<T, uint8_t>) {
            os << static_cast<unsigned int>(value);
        } else if constexpr (std::is_same_v<T, uint16_t>) {
            os << value;
        } else if constexpr (std::is_same_v<T, uint32_t>) {
            os << value;
        } else if constexpr (std::is_same_v<T, uint64_t>) {
            os << value;
        }
    }, uint_variant);

    return os;
}
