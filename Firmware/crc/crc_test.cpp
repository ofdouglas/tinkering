#include "crc.h"
#include "crc/crc.h"

#include <array>
#include <string>
#include <variant>
#include <vector>
#include <gtest/gtest.h>

using namespace crc;

/******************************************************************************
 *  Test Helpers
 ******************************************************************************/

// Intended for {8,16,32} bit CRC use. u64 is used for future-proofing.
// only the lowest crc::Spec::value_type bits of expected_crc are checked.
struct Expectation {
    std::string name{};
    CrcSpecTag  spec_tag{};
    uint64_t    expected_crc{};
};

// Each test suite tests a single input against multiple expectations.
struct TestSuite {
    std::vector<uint8_t>     input{};
    std::vector<Expectation> expectations{};
};

// A test case is a single input against a single expectation.
struct TestCase {
    const std::vector<uint8_t>* input{};
    const Expectation*          expectation{};
};

std::vector<TestCase> make_crc_test_cases(const std::vector<TestSuite>& test_suites) {
    std::vector<TestCase> test_cases{};

    for (const TestSuite& suite : test_suites) {
        for (const Expectation& expectation : suite.expectations) {
            test_cases.push_back(TestCase{&suite.input, &expectation});
        }
    }
    return test_cases;
}

// This is used to dispatch to EXPECT_EQ() with correct uint type so failures print cleanly.
using UintVariant = std::variant<uint8_t, uint16_t, uint32_t, uint64_t>;

UintVariant dispatch_crc(CrcSpecTag spec, Span<const uint8_t> input) {
    switch (spec) {
        case CrcSpecTag::SaeJ1850:
            return crc::crcBitwise<crc::SaeJ1850>(input);
            break;
        default:
            // TODO: throw exception
            break;
    }

    // TODO: throw exception
    return 0U;
}


/******************************************************************************
 *  Test Inputs and Expectations
 ******************************************************************************/

std::vector<TestSuite> kTestSuites = {
    {
        {0x42U},
        {
            {"SaeJ1850 0x42", CrcSpecTag::SaeJ1850, 0x12U},
        }
    },
    {
        {0xFEU},
        {
            {"SaeJ1850 0xFE", CrcSpecTag::SaeJ1850, 0xE2U},
        }
    }
};

class CrcTest : public ::testing::TestWithParam<TestCase> {
protected:
    void expect(uint64_t expected_crc, UintVariant actual_crc) {
        std::visit([expected_crc](UintVariant actual_crc) {
            using T = std::decay_t<decltype(actual_crc)>;
    
            if constexpr (std::is_same_v<T, uint8_t>) {
                EXPECT_EQ(static_cast<uint8_t>(expected_crc), actual_crc);
            } else if constexpr (std::is_same_v<T, uint16_t>) {
                EXPECT_EQ(static_cast<uint16_t>(expected_crc), actual_crc);
            } else if constexpr (std::is_same_v<T, uint32_t>) {
                EXPECT_EQ(static_cast<uint32_t>(expected_crc), actual_crc);
            } else if constexpr (std::is_same_v<T, uint64_t>) {
                EXPECT_EQ(static_cast<uint64_t>(expected_crc), actual_crc);
            }
        }, actual_crc);
    }
};

TEST_P(CrcTest, CrcMatchesExpectation) {
    SCOPED_TRACE(GetParam().expectation->name);

    const TestCase& test_case = GetParam();
    const Span<const uint8_t> input_span{test_case.input->data(), test_case.input->size()};
    const UintVariant result = dispatch_crc(test_case.expectation->spec_tag, input_span);
    
    expect(test_case.expectation->expected_crc, result);
}

INSTANTIATE_TEST_SUITE_P(
    CrcTestSuite,
    CrcTest,
    ::testing::ValuesIn(make_crc_test_cases(kTestSuites))
);

