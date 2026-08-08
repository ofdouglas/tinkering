#include <vector>

#include <gtest/gtest.h>

#include "crc/test/crc_test_algorithms.h"
#include "crc/test/crc_test_infra.h"

using crc::test::CrcTestCatalog;
using crc::test::CrcTestCase;
using crc::test::ExpectationSpec;
using crc::test::TestVector;
using crc::test::kAutosarCrc8Algorithm;
using crc::test::kSaeJ1850Algorithm;
using crc::test::kCrc16CcittFalseAlgorithm;
using crc::test::kCrc32Mpeg2Algorithm;

namespace {

/* Test Data Sources
    https://www.sunshine2k.de/coding/javascript/crc/crc_js.html
    https://emn178.github.io/online-tools/crc.html
 */

const CrcTestCatalog& testCatalog() {
    static const CrcTestCatalog kCatalog{std::vector<TestVector>{
        // 1 byte inputs
        {
            {0x42},
            {
                {kSaeJ1850Algorithm, 0x12},
                {kAutosarCrc8Algorithm, 0x05},
                {kCrc16CcittFalseAlgorithm, 0x8976},
                {kCrc32Mpeg2Algorithm, 0x730CF4AD},
            },
        },
        {
            {0xFE},
            {
                {kSaeJ1850Algorithm, 0xE2},
                {kAutosarCrc8Algorithm, 0xD0},
                {kCrc16CcittFalseAlgorithm, 0xEF21},
                {kCrc32Mpeg2Algorithm, 0xFB3EE2B7},
            },
        },
        // 2 byte inputs : TODO
        // 3 byte inputs : TODO
        // 4 byte inputs
        {
            {0xDE, 0xAD, 0xBE, 0xEF},
            {
                {kSaeJ1850Algorithm, 0xB3},
                {kAutosarCrc8Algorithm, 0xEB},
                {kCrc16CcittFalseAlgorithm, 0x4097},
                {kCrc32Mpeg2Algorithm, 0x81DA1A18},
            },
        },
        // 5 byte inputs : TODO
        // 6 byte inputs : TODO
        // 7 byte inputs : TODO
        // 8 byte inputs
        {   // Default test vector from https://www.sunshine2k.de/coding/javascript/crc/crc_js.html
            {0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39},
            {
                {kSaeJ1850Algorithm, 0x4B},
                {kAutosarCrc8Algorithm, 0xDF},
                {kCrc16CcittFalseAlgorithm, 0x29B1},
                {kCrc32Mpeg2Algorithm, 0x0376E6E7},
            },
        },
        // 9 byte inputs : TODO
        // 10 byte inputs : TODO
        // ASCII inputs
        {
            "DEADBEEF",
            {
                {kSaeJ1850Algorithm, 0x8C},
                {kAutosarCrc8Algorithm, 0x4B},
                {kCrc16CcittFalseAlgorithm, 0x7484},
                {kCrc32Mpeg2Algorithm, 0x5791F5B3},
            },
        },
    }};
    return kCatalog;
}

} // namespace

class CrcTest : public ::testing::TestWithParam<CrcTestCase> {};

TEST_P(CrcTest, SingleComputeTest) {
    GetParam().run();
}

INSTANTIATE_TEST_SUITE_P(
    CrcSingleComputePositiveTest,
    CrcTest,
    ::testing::ValuesIn(testCatalog().positiveCases()),
    [](const testing::TestParamInfo<CrcTestCase>& info) { return info.param.gtestName(); });

INSTANTIATE_TEST_SUITE_P(
    CrcSingleComputeNegativeTest,
    CrcTest,
    ::testing::ValuesIn(testCatalog().negativeCases()),
    [](const testing::TestParamInfo<CrcTestCase>& info) { return info.param.gtestName(); });
