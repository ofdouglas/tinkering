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

namespace {

const CrcTestCatalog& testCatalog() {
    static const CrcTestCatalog kCatalog{std::vector<TestVector>{
        {
            {0x42},
            {
                {kSaeJ1850Algorithm, 0x12},
                {kAutosarCrc8Algorithm, 0x05},
            },
        },
        {
            {0xFE},
            {
                {kSaeJ1850Algorithm, 0xE2},
                {kAutosarCrc8Algorithm, 0xD0},
            },
        },
        {
            {0xDE, 0xAD, 0xBE, 0xEF},
            {
                {kSaeJ1850Algorithm, 0xB3},
                {kAutosarCrc8Algorithm, 0xEB},
            },
        },
        {
            "DEADBEEF",
            {
                {kSaeJ1850Algorithm, 0x8C},
                {kAutosarCrc8Algorithm, 0x4B},
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
