#include "crc/test/crc_test_infra.h"

#include <algorithm>
#include <cstring>
#include <iostream>
#include <variant>

#include <gtest/gtest.h>

#include "util/ostream_helpers.h"

namespace crc::test {

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

uint64_t corruptExpectedCrc(uint64_t expected_crc, const CrcAlgorithm& algorithm) {
    const util::UintVariant wrapped = algorithm.wrapExpected(expected_crc);
    return std::visit(
        [](auto value) -> uint64_t {
            using T = std::decay_t<decltype(value)>;
            T corrupted = static_cast<T>(value ^ T{1});
            if (corrupted == value) {
                corrupted = static_cast<T>(value + T{1});
            }
            return static_cast<uint64_t>(corrupted);
        },
        wrapped);
}

} // namespace

TestInput TestInput::bytes(std::initializer_list<uint8_t> data) {
    TestInput input{};
    input.data_.assign(data.begin(), data.end());
    input.encoding_ = InputEncoding::kBytes;
    return input;
}

TestInput TestInput::ascii(const char* text) {
    TestInput input{};
    const auto* begin = reinterpret_cast<const uint8_t*>(text);
    input.data_.assign(begin, begin + std::strlen(text));
    input.encoding_ = InputEncoding::kAscii;
    return input;
}

util::Span<const uint8_t> TestInput::span() const {
    return util::Span<const uint8_t>{data_.data(), data_.size()};
}

std::string TestInput::label() const {
    if (encoding_ == InputEncoding::kAscii) {
        std::string s = "ASCII_";
        const size_t n = std::min(data_.size(), kMaxInputLabelBytes);
        s.append(reinterpret_cast<const char*>(data_.data()), n);
        return s;
    }
    return inputPrefixHex(data_);
}

void TestInput::print(std::ostream& os) const {
    if (encoding_ == InputEncoding::kAscii) {
        os << std::string(reinterpret_cast<const char*>(data_.data()), data_.size());
    } else {
        os << span();
    }
}

CrcExpectation::CrcExpectation(std::string name, CrcAlgorithm algorithm, uint64_t expected_crc)
    : name_(std::move(name))
    , algorithm_(algorithm)
    , expected_crc_(expected_crc) {}

CrcExpectation CrcExpectation::fromSpec(const TestInput& input, const ExpectationSpec& spec) {
    std::string name = spec.name_override != nullptr ? spec.name_override
                                                     : input.label() + "_" + spec.algorithm.name;
    return CrcExpectation{std::move(name), spec.algorithm, spec.expected_crc};
}

CrcExpectation CrcExpectation::withCorruptedCrc() const {
    return CrcExpectation{name_, algorithm_, corruptExpectedCrc(expected_crc_, algorithm_)};
}

CrcExpectation CrcExpectation::withNameSuffix(const char* suffix) const {
    return CrcExpectation{name_ + suffix, algorithm_, expected_crc_};
}

util::UintVariant CrcExpectation::wrapExpected() const {
    return algorithm_.wrapExpected(expected_crc_);
}

std::vector<CrcExpectation> TestVector::expectationsFromSpecs(const TestInput& input,
                                                              std::initializer_list<ExpectationSpec> specs) {
    std::vector<CrcExpectation> expectations{};
    expectations.reserve(specs.size());
    for (const ExpectationSpec& spec : specs) {
        expectations.push_back(CrcExpectation::fromSpec(input, spec));
    }
    return expectations;
}

TestVector::TestVector(const char* ascii_input, std::initializer_list<ExpectationSpec> specs)
    : input_(TestInput::ascii(ascii_input))
    , expectations_(expectationsFromSpecs(input_, specs)) {}

TestVector::TestVector(std::initializer_list<uint8_t> bytes, std::initializer_list<ExpectationSpec> specs)
    : input_(TestInput::bytes(bytes))
    , expectations_(expectationsFromSpecs(input_, specs)) {}

TestVector TestVector::asNegativeHarness() const {
    TestVector negative{};
    negative.input_ = input_;
    negative.expectations_.reserve(expectations_.size());
    for (const CrcExpectation& expectation : expectations_) {
        negative.expectations_.push_back(expectation.withCorruptedCrc().withNameSuffix("_negative"));
    }
    return negative;
}

std::vector<TestVector> TestVector::negativeHarness(const std::vector<TestVector>& positive_vectors) {
    std::vector<TestVector> negative_vectors{};
    negative_vectors.reserve(positive_vectors.size());
    for (const TestVector& positive_vector : positive_vectors) {
        negative_vectors.push_back(positive_vector.asNegativeHarness());
    }
    return negative_vectors;
}

CrcTestCase::CrcTestCase(const CrcTestCatalog* catalog,
                         size_t vector_index,
                         size_t expectation_index,
                         bool negative_harness,
                         TestPolarity polarity)
    : catalog_(catalog),
      vector_index_(vector_index),
      expectation_index_(expectation_index),
      negative_harness_(negative_harness),
      polarity_(polarity) {}

std::string CrcTestCase::gtestName() const {
    return catalog_->expectationAt(vector_index_, expectation_index_, negative_harness_).name();
}

void CrcTestCase::run() const {
    const CrcExpectation& expectation = catalog_->expectationAt(vector_index_, expectation_index_, negative_harness_);
    const TestInput& input = catalog_->vectorAt(vector_index_, negative_harness_).input();

    SCOPED_TRACE(expectation.name());

    const util::UintVariant actual_crc = expectation.algorithm().compute(input.span());
    const util::UintVariant expected_crc = expectation.wrapExpected();

    const bool matches = (expected_crc == actual_crc);
    const bool negative_test = polarity_ == TestPolarity::kExpectMismatch;
    const bool failed = negative_test ? matches : !matches;

    if (!failed) {
        return;
    }

    std::cout << (input.encoding() == InputEncoding::kAscii ? "Input (ASCII): " : "Input (HEX):   ");
    input.print(std::cout);
    std::cout << std::endl;
    std::cout << "Expected CRC:  " << expected_crc << std::endl;
    std::cout << "Actual CRC:    " << actual_crc << std::endl;
    if (negative_test) {
        std::cout << "Negative Test: TRUE" << std::endl;
    }
    ADD_FAILURE();
}

CrcTestCatalog::CrcTestCatalog(std::vector<TestVector> positive_vectors)
    : positive_vectors_(std::move(positive_vectors))
    , negative_vectors_(TestVector::negativeHarness(positive_vectors_)) {
    appendCases(positive_vectors_, false, TestPolarity::kExpectMatch);
    appendCases(negative_vectors_, true, TestPolarity::kExpectMismatch);
}

const TestVector& CrcTestCatalog::vectorAt(size_t vector_index, bool negative_harness) const {
    return negative_harness ? negative_vectors_.at(vector_index) : positive_vectors_.at(vector_index);
}

const CrcExpectation& CrcTestCatalog::expectationAt(size_t vector_index,
                                                    size_t expectation_index,
                                                    bool negative_harness) const {
    return vectorAt(vector_index, negative_harness).expectations().at(expectation_index);
}

void CrcTestCatalog::appendCases(const std::vector<TestVector>& vectors,
                                 bool negative_harness,
                                 TestPolarity polarity) {
    std::vector<CrcTestCase>* destination = negative_harness ? &negative_cases_ : &positive_cases_;
    for (size_t vector_index = 0U; vector_index < vectors.size(); ++vector_index) {
        const size_t expectation_count = vectors[vector_index].expectations().size();
        for (size_t expectation_index = 0U; expectation_index < expectation_count; ++expectation_index) {
            destination->emplace_back(this, vector_index, expectation_index, negative_harness, polarity);
        }
    }
}

} // namespace crc::test
