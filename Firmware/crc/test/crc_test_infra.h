#pragma once
/*
 * @file  crc_test_infra.h
 * @brief Data-driven CRC unit test model (inputs, vectors, cases, catalog).
 * @note  Not intended for production / on-target use.
 */

#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <iosfwd>
#include <string>
#include <vector>

#include "crc/test/crc_test_algorithms.h"
#include "util/integer.h"
#include "util/span.h"

namespace crc::test {

/** How a TestInput should be labeled in gtest names and failure logs. */
enum class InputEncoding { kBytes, kAscii };

/**
 * Payload bytes for one CRC compute, plus metadata for naming and printing.
 * kBytes inputs use a hex label (e.g. "0xDEADBEEF"); kAscii uses "ASCII_" + up to 8 chars.
 */
class TestInput {
public:
    static TestInput bytes(std::initializer_list<uint8_t> data);
    static TestInput ascii(const char* text);

    util::Span<const uint8_t> span() const;
    std::string label() const;
    void print(std::ostream& os) const;

    InputEncoding encoding() const { return encoding_; }

private:
    std::vector<uint8_t> data_{};
    InputEncoding encoding_{InputEncoding::kBytes};
};

/**
 * Authoring-time row in a test table: algorithm + golden CRC, before names are resolved.
 * expected_crc is stored as uint64_t; only the algorithm's value_type width is compared at run time.
 * name_override, when non-null, skips auto-naming from the input label and algorithm name.
 */
struct ExpectationSpec {
    CrcAlgorithm algorithm{};
    uint64_t     expected_crc{};
    const char*  name_override{};
};

/**
 * One algorithm + expected CRC + gtest name, after resolving from ExpectationSpec.
 * withCorruptedCrc() flips the expected value in the algorithm's integer type (for harness tests).
 */
class CrcExpectation {
public:
    static CrcExpectation fromSpec(const TestInput& input, const ExpectationSpec& spec);

    CrcExpectation withCorruptedCrc() const;
    CrcExpectation withNameSuffix(const char* suffix) const;

    const std::string& name() const { return name_; }
    const CrcAlgorithm& algorithm() const { return algorithm_; }
    uint64_t expectedCrc() const { return expected_crc_; }

    util::UintVariant wrapExpected() const;

private:
    CrcExpectation(std::string name, CrcAlgorithm algorithm, uint64_t expected_crc);

    std::string  name_{};
    CrcAlgorithm algorithm_{};
    uint64_t     expected_crc_{};
};

/**
 * One input exercised against several algorithms (one CrcExpectation per algorithm).
 * Construct with raw bytes or a C string; the latter sets InputEncoding::kAscii.
 */
class TestVector {
public:
    TestVector() = default;

    TestVector(const char* ascii_input, std::initializer_list<ExpectationSpec> specs);
    TestVector(std::initializer_list<uint8_t> bytes, std::initializer_list<ExpectationSpec> specs);

    const TestInput& input() const { return input_; }
    const std::vector<CrcExpectation>& expectations() const { return expectations_; }

    TestVector asNegativeHarness() const;
    static std::vector<TestVector> negativeHarness(const std::vector<TestVector>& positive_vectors);

private:
    static std::vector<CrcExpectation> expectationsFromSpecs(const TestInput& input,
                                                             std::initializer_list<ExpectationSpec> specs);

    TestInput input_{};
    std::vector<CrcExpectation> expectations_{};
};

/** Whether a parametrized case should pass on CRC match or on intentional mismatch. */
enum class TestPolarity {
    kExpectMatch,
    kExpectMismatch,
};

class CrcTestCatalog;

/**
 * Single gtest parameter: one input, one expectation, and how to judge pass/fail.
 * Holds indices into a CrcTestCatalog; the catalog must outlive all CrcTestCase copies
 * (e.g. keep the catalog in a function-local static).
 */
class CrcTestCase {
public:
    CrcTestCase() = default;

    CrcTestCase(const CrcTestCatalog* catalog,
                size_t vector_index,
                size_t expectation_index,
                bool negative_harness,
                TestPolarity polarity);

    std::string gtestName() const;
    void run() const;

private:
    const CrcTestCatalog* catalog_{};
    size_t vector_index_{};
    size_t expectation_index_{};
    bool negative_harness_{};
    TestPolarity polarity_{TestPolarity::kExpectMatch};
};

/**
 * Owns positive test vectors, auto-derived negative harness vectors, and flattened CrcTestCase lists.
 * Negative cases reuse the same inputs but corrupt each expected CRC and append "_negative" to the name.
 */
class CrcTestCatalog {
public:
    explicit CrcTestCatalog(std::vector<TestVector> positive_vectors);

    const std::vector<CrcTestCase>& positiveCases() const { return positive_cases_; }
    const std::vector<CrcTestCase>& negativeCases() const { return negative_cases_; }

private:
    friend class CrcTestCase;

    const TestVector& vectorAt(size_t vector_index, bool negative_harness) const;
    const CrcExpectation& expectationAt(size_t vector_index, size_t expectation_index, bool negative_harness) const;

    void appendCases(const std::vector<TestVector>& vectors, bool negative_harness, TestPolarity polarity);

    std::vector<TestVector> positive_vectors_{};
    std::vector<TestVector> negative_vectors_{};
    std::vector<CrcTestCase> positive_cases_{};
    std::vector<CrcTestCase> negative_cases_{};
};

} // namespace crc::test
