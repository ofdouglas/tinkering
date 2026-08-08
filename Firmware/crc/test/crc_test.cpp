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
// using crc::test::kCrc32Mpeg2Algorithm;

namespace {

const CrcTestCatalog& testCatalog() {
    static const CrcTestCatalog kCatalog{std::vector<TestVector>{
        {
            {0x42},
            {
                {kSaeJ1850Algorithm, 0x12},
                {kAutosarCrc8Algorithm, 0x05},
                {kCrc16CcittFalseAlgorithm, 0x8976},
            },
        },
        {
            {0xFE},
            {
                {kSaeJ1850Algorithm, 0xE2},
                {kAutosarCrc8Algorithm, 0xD0},
                {kCrc16CcittFalseAlgorithm, 0xEF21},
            },
        },
        {
            {0xDE, 0xAD, 0xBE, 0xEF},
            {
                {kSaeJ1850Algorithm, 0xB3},
                {kAutosarCrc8Algorithm, 0xEB},
                {kCrc16CcittFalseAlgorithm, 0x4097},
            },
        },
        {
            {0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39},
            {
                {kSaeJ1850Algorithm, 0x4B},
                {kAutosarCrc8Algorithm, 0xDF},
                {kCrc16CcittFalseAlgorithm, 0x29B1},
            },
        },
        {
            "DEADBEEF",
            {
                {kSaeJ1850Algorithm, 0x8C},
                {kAutosarCrc8Algorithm, 0x4B},
                {kCrc16CcittFalseAlgorithm, 0x7484},
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
