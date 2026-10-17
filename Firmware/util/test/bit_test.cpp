#include <cstdint>
#include <limits>

#include <gtest/gtest.h>

#include "util/bit.h"

namespace {

/*
 * Test plan (util::bitWidth)
 *
 * Happy path:
 *   - Powers of two: bitWidth(2^n) == n + 1
 *   - Values just below a power of two: bitWidth(2^n - 1) == n
 *
 * Expected errors / misuse:
 *   - Signed types and bool are rejected at compile time (static_assert); not runtime-tested
 *
 * Edge cases / boundary values:
 *   - 0 -> 0
 *   - 1 -> 1
 *   - Maximum value of uint8/16/32/64_t -> digit count of that type
 *
 * Adversarial / stress-style:
 *   - Representative mid-range values that are not powers of two
 */

struct BitWidthCase {
    uint64_t value;
    int expected;
};

class BitWidthTest : public ::testing::TestWithParam<BitWidthCase> {};

}  // namespace

// Verifies bitWidth matches 1 + floor(log2(value)), including 0 -> 0.
TEST_P(BitWidthTest, MatchesExpectedWidth) {
    EXPECT_EQ(GetParam().expected, util::bitWidth(GetParam().value));
}

INSTANTIATE_TEST_SUITE_P(KnownValues, BitWidthTest,
                         ::testing::Values(BitWidthCase{0U, 0}, BitWidthCase{1U, 1},
                                           BitWidthCase{2U, 2}, BitWidthCase{3U, 2},
                                           BitWidthCase{4U, 3}, BitWidthCase{7U, 3},
                                           BitWidthCase{8U, 4}, BitWidthCase{255U, 8},
                                           BitWidthCase{256U, 9}, BitWidthCase{65535U, 16},
                                           BitWidthCase{65536U, 17},
                                           BitWidthCase{0x8000'0000ULL, 32},
                                           BitWidthCase{0xFFFF'FFFFULL, 32},
                                           BitWidthCase{1ULL << 63, 64}));

// Verifies bitWidth is a constant expression for the types used by HardwareTimer.
TEST(BitWidthConstexprTest, EvaluatesAtCompileTime) {
    static_assert(util::bitWidth(0U) == 0);
    static_assert(util::bitWidth(1U) == 1);
    static_assert(util::bitWidth(size_t{65536U}) == 17);
    SUCCEED();
}

// Verifies the full width of each unsigned integer type's maximum value.
TEST(BitWidthBoundaryTest, MaxValueUsesAllDigits) {
    EXPECT_EQ(8, util::bitWidth(std::numeric_limits<uint8_t>::max()));
    EXPECT_EQ(16, util::bitWidth(std::numeric_limits<uint16_t>::max()));
    EXPECT_EQ(32, util::bitWidth(std::numeric_limits<uint32_t>::max()));
    EXPECT_EQ(64, util::bitWidth(std::numeric_limits<uint64_t>::max()));
}
