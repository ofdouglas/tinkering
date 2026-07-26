#include "crc/crc.h"

uint8_t crcSaeJ1850(Span<uint8_t> input) {
    const uint8_t kPolynomial = 0x1D;
    const uint8_t kTestBit    = 0x80;
    uint8_t result = 0xFF;

    for (auto x : input) {
        result ^= x;
        for (int i = 0; i < 8; i++) {
            if (result & kTestBit) {
                result = (result << 1U) ^ kPolynomial;
            } else {
                result <<= 1U;
            }
        }
    }

    return static_cast<uint8_t>(~result);
}
