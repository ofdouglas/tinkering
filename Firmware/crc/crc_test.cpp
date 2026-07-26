#include "crc/crc.h"

#include <array>
#include <gtest/gtest.h>

namespace {

struct SaeJ1850Case {
    uint8_t data;
    uint8_t expected;
};

constexpr SaeJ1850Case kFpgaVectors[] = {
    {0x42U, 0x12U},
    {0xFEU, 0xE2U},
    {0x00U, 0x3BU},
    {0x12U, 0xCCU},
};

uint8_t crcTemplated(Span<uint8_t> input) {
    return crcBitwise<uint8_t, 0x1DU, 0xFFU, 0xFFU>(input);
}

} // namespace

TEST(CrcSaeJ1850, MatchesFpgaVectors) {
    for (const SaeJ1850Case& test_case : kFpgaVectors) {
        SCOPED_TRACE(test_case.data);
        std::array<uint8_t, 1U> data{test_case.data};
        EXPECT_EQ(crcSaeJ1850(Span<uint8_t>(data)), test_case.expected);
    }
}

TEST(CrcSaeJ1850, MatchesCrcBitwiseTemplate) {
    for (const SaeJ1850Case& test_case : kFpgaVectors) {
        SCOPED_TRACE(test_case.data);
        std::array<uint8_t, 1U> data{test_case.data};
        EXPECT_EQ(crcTemplated(Span<uint8_t>(data)), test_case.expected);
    }
}
