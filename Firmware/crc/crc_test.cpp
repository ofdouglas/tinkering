#include "crc.h"
#include "crc/crc.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <iostream>
#include <iomanip>
#include <string>
#include <variant>
#include <vector>
#include <gtest/gtest.h>

using namespace crc;

/* TODOs:
    - Test correct output formatting for negative tests / errors
    - Build kNegativeTestSuites from kTestSuites by corrupting the expected CRC
    - Factory class and pos/neg Enum?
*/


/******************************************************************************
 *  Runtime Dispatch for CRC Algorithms
 ******************************************************************************/

struct CrcAlgorithm {
    const char* name;
    UintVariant (*compute)(Span<const uint8_t> input);
    UintVariant (*wrapExpected)(uint64_t expected);
};

template <typename Derived>
constexpr CrcAlgorithm makeCrcAlgorithm() {
    return CrcAlgorithm{
        Derived::name(),
        [](Span<const uint8_t> input) -> UintVariant {
            return details::crcBitwise<Derived>(input);
        },
        [](uint64_t expected) -> UintVariant {
            return static_cast<typename Derived::value_type>(expected);
        }
    };
}

constexpr CrcAlgorithm SaeJ1850Algorithm  = makeCrcAlgorithm<crc::SaeJ1850>();
constexpr CrcAlgorithm AutosarCrc8Algorithm = makeCrcAlgorithm<crc::AutosarCrc8>();
// constexpr CrcAlgorithm Crc16CcittAlgorithm = makeCrcAlgorithm<crc::Crc16Ccitt>();


/******************************************************************************
 *  Ostream Helpers
 *  TODO: move to a separate file
 ******************************************************************************/

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
std::ostream& operator<<(std::ostream& os, const Span<const uint8_t>& span) {
    IostreamRaiiFlagsRestorer flags_restorer{os};

    for (uint8_t byte : span) {
        os << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << static_cast<int>(byte) << " ";
    }
    return os;
}

// Print a hex dump of a vector of bytes. Format ex: 42 01 2A
std::ostream& operator<<(std::ostream& os, const std::vector<uint8_t>& vector) {
    return os << Span<const uint8_t>{vector.data(), vector.size()};
}

// Print a UintVariant as a zero-padded hex string of correct width for the type
// Format ex: 0xFE, 0x002A, 0xDEADBEEF
std::ostream& operator<<(std::ostream& os, const UintVariant& uint_variant) {
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


/******************************************************************************
 *  CRC Test Helpers
 ******************************************************************************/

// Intended for {8,16,32} bit CRC use. u64 is used for future-proofing.
// only the lowest crc::Spec::value_type bits of expected_crc are checked.
struct Expectation {
    std::string  name{};
    CrcAlgorithm algorithm{};
    uint64_t     expected_crc{};
};

struct ExpectationSpec {
    CrcAlgorithm algorithm{};
    uint64_t     expected_crc{};
    const char*  name_override{};
};

namespace {

constexpr size_t kMaxInputLabelBytes = 8U;

std::string inputPrefixHex(const std::vector<uint8_t>& input) {
    std::string s = "0x";
    const size_t n = std::min(input.size(), kMaxInputLabelBytes);
    s.reserve(2U + (n * 2U));
    static constexpr char kHex[] = "0123456789ABCDEF";
    for (size_t i = 0U; i < n; ++i) {
        const unsigned b = static_cast<unsigned>(input[i]);
        s += kHex[b >> 4U];
        s += kHex[b & 0xFU];
    }
    return s;
}

std::string inputLabel(const std::vector<uint8_t>& input, bool is_ascii) {
    if (is_ascii) {
        std::string s = "ASCII_";
        const size_t n = std::min(input.size(), kMaxInputLabelBytes);
        s.append(reinterpret_cast<const char*>(input.data()), n);
        return s;
    }
    return inputPrefixHex(input);
}

std::string defaultExpectationName(const std::vector<uint8_t>& input, bool is_ascii, const CrcAlgorithm& algorithm) {
    return inputLabel(input, is_ascii) + "_" + algorithm.name;
}

std::vector<Expectation> makeExpectations(const std::vector<uint8_t>& input,
                                          bool is_ascii,
                                          std::initializer_list<ExpectationSpec> specs) {
    std::vector<Expectation> expectations{};
    expectations.reserve(specs.size());
    for (const ExpectationSpec& spec : specs) {
        expectations.push_back(Expectation{
            spec.name_override != nullptr ? spec.name_override
                                          : defaultExpectationName(input, is_ascii, spec.algorithm),
            spec.algorithm,
            spec.expected_crc,
        });
    }
    return expectations;
}

} // namespace

// Each test suite tests a single input against multiple expectations.
struct TestSuite {
    bool                     is_ascii{false};
    std::vector<uint8_t>     input{};
    std::vector<Expectation> expectations{};

    TestSuite(const char* ascii_input, std::initializer_list<ExpectationSpec> specs)
        : is_ascii(true)
        , input(reinterpret_cast<const uint8_t*>(ascii_input),
                reinterpret_cast<const uint8_t*>(ascii_input) + std::strlen(ascii_input))
        , expectations(makeExpectations(input, is_ascii, specs)) {}

    TestSuite(std::initializer_list<uint8_t> bytes, std::initializer_list<ExpectationSpec> specs)
        : is_ascii(false)
        , input(bytes)
        , expectations(makeExpectations(input, is_ascii, specs)) {}
};

// A test case is a single input against a single expectation.
struct TestCase {
    const std::vector<uint8_t>* input{};
    bool                        is_ascii{};
    const Expectation*          expectation{};
    bool                        is_negative_test{};
};

// TODO: factory class and pos/neg Enum?
std::vector<TestCase> make_crc_test_cases(const std::vector<TestSuite>& test_suites, bool negative_test = false) {
    std::vector<TestCase> test_cases{};

    for (const TestSuite& suite : test_suites) {
        for (const Expectation& expectation : suite.expectations) {
            test_cases.push_back(TestCase{&suite.input, suite.is_ascii, &expectation, negative_test});
        }
    }
    return test_cases;
}


/******************************************************************************
 *  Test Inputs and Expectations
 ******************************************************************************/

const std::vector<TestSuite> kTestSuites = {
    {
        {0x42},
        {
            {SaeJ1850Algorithm, 0x12},
            {AutosarCrc8Algorithm, 0x05},
        },
    },
    {
        {0xFE},
        {
            {SaeJ1850Algorithm, 0xE2},
            {AutosarCrc8Algorithm, 0xD0},
        },
    },
    {
        {0xDE, 0xAD, 0xBE, 0xEF},
        {
            {SaeJ1850Algorithm, 0xB3},
            {AutosarCrc8Algorithm, 0xEB},
        },
    },
    {
        "DEADBEEF",
        {
            {SaeJ1850Algorithm, 0x8C},
            {AutosarCrc8Algorithm, 0x4B},
        },
    },
};

const std::vector<TestSuite> kNegativeTestSuites = {
    {
        {0x42},
        {
            {SaeJ1850Algorithm, 0x11, "SaeJ1850_0x42_negative"},
            {AutosarCrc8Algorithm, 0x15, "AutosarCrc8_0x42_negative"},
        },
    },
    {
        {0xDE, 0xAD, 0xBE, 0xEF},
        {
            {SaeJ1850Algorithm, 0xB4, "SaeJ1850_0xDEADBEEF_negative"},
            {AutosarCrc8Algorithm, 0xEA, "AutosarCrc8_0xDEADBEEF_negative"},
        },
    },
    {
        "DEADBEEF",
        {
            {SaeJ1850Algorithm, 0x8D, "SaeJ1850_ASCII_DEADBEEF_negative"},
            {AutosarCrc8Algorithm, 0x44, "AutosarCrc8_ASCII_DEADBEEF_negative"},
        },
    },
};


/******************************************************************************
 *  CrcTest Class
 ******************************************************************************/

class CrcTest : public ::testing::TestWithParam<TestCase> {
protected:
    // TODO: move to TestCase class?
    void logTestCase(const TestCase& test_case, UintVariant expected_crc, UintVariant actual_crc) {
        const Span<const uint8_t> input_span{test_case.input->data(), test_case.input->size()};
        
        if (test_case.is_ascii) {
            std::cout << "Input (ASCII): " << std::string(reinterpret_cast<const char*>(input_span.data()), input_span.size()) << std::endl;
        } else {
            std::cout << "Input (HEX):   " << input_span << std::endl;
        }
        std::cout << "Expected CRC:  " << expected_crc << std::endl;
        std::cout << "Actual CRC:    " << actual_crc << std::endl;
        if (test_case.is_negative_test) {
            std::cout << "Negative Test: TRUE" << std::endl;
        }
    }
};

TEST_P(CrcTest, SingleComputeTest) {
    const TestCase& test_case = GetParam();
    const CrcAlgorithm& algorithm = test_case.expectation->algorithm;
    SCOPED_TRACE(test_case.expectation->name);

    const Span<const uint8_t> input_span{test_case.input->data(), test_case.input->size()};
    const UintVariant actual_crc = algorithm.compute(input_span);
    const UintVariant expected_crc = algorithm.wrapExpected(test_case.expectation->expected_crc);

    if (test_case.is_negative_test && (expected_crc == actual_crc)) {
        logTestCase(test_case, expected_crc, actual_crc);
        ADD_FAILURE();
    } else if (!test_case.is_negative_test && (expected_crc != actual_crc)) {
        logTestCase(test_case, expected_crc, actual_crc);
        ADD_FAILURE();
    }
}

INSTANTIATE_TEST_SUITE_P(
    CrcSingleComputePositiveTest,
    CrcTest,
    ::testing::ValuesIn(make_crc_test_cases(kTestSuites)),
    [](const testing::TestParamInfo<TestCase>& info) {
        return info.param.expectation->name;
    }
);

INSTANTIATE_TEST_SUITE_P(
    CrcSingleComputeNegativeTest,
    CrcTest,
    ::testing::ValuesIn(make_crc_test_cases(kNegativeTestSuites, true)),
    [](const testing::TestParamInfo<TestCase>& info) {
        return info.param.expectation->name;
    }
);

