#include <cstdint>

#include <gtest/gtest.h>

#include "hal/sw_extended_timer.h"
#include "hal/test/simulated_hardware_timer.h"

namespace {

/*
 * Test plan (HardwareTimer / SimulatedHardwareTimer / SoftwareExtendedTimer)
 *
 * Happy path:
 *   - Up counter starts at 0; down counter starts at modulus-1
 *   - advance updates the raw counter without wrapping
 *   - wrapOnce sets the wrap flag and a post-wrap raw value
 *   - checkWrapAround reports a wrap once, then clears the flag
 *   - SoftwareExtendedTimer::read reflects hardware ticks * TickDuration
 *   - Wrap consumed by read() extends the software counter
 *   - Wrap consumed by handleTimerWrapInterrupt is not double-counted on read
 *
 * Edge cases / boundary values:
 *   - counterValueFromRaw is identity for up counters and inverts down counters
 *   - TickDuration scales the extended value
 *
 * Adversarial / stress-style:
 *   - Many successive wraps accumulate a consistent extended tick count
 *   - CRTP dispatch through a HardwareTimer base reference reaches the simulated timer
 */

constexpr uint64_t kModulus16{16U};

using UpTimer16 = hal::test::SimulatedHardwareTimer<uint8_t, kModulus16, false>;
using DownTimer16 = hal::test::SimulatedHardwareTimer<uint8_t, kModulus16, true>;

template <typename HwTimer, typename ExtendedTimer>
void wrapWithIsr(HwTimer& hw, ExtendedTimer& timer, int count) {
    for (int i{0}; i < count; ++i) {
        hw.wrapOnce();
        timer.handleTimerWrapInterrupt();
    }
}

class SimulatedUpTimer16Test : public ::testing::Test {
protected:
    UpTimer16 hw_;
};

class SimulatedDownTimer16Test : public ::testing::Test {
protected:
    DownTimer16 hw_;
};

class ExtendedUpTimer16Test : public ::testing::Test {
protected:
    UpTimer16 hw_;
    hal::SoftwareExtendedTimer<UpTimer16, uint64_t, uint64_t{1}> timer_{hw_};
};

class ExtendedDownTimer16Test : public ::testing::Test {
protected:
    DownTimer16 hw_;
    hal::SoftwareExtendedTimer<DownTimer16, uint64_t, uint64_t{1}> timer_{hw_};
};

}  // namespace

// Verifies compile-time traits for an up-counting 16-tick timer.
TEST(HardwareTimerTraitsTest, UpCounterReportsModulusAndBitWidth) {
    EXPECT_EQ(kModulus16, UpTimer16::kCounterModulus);
    EXPECT_EQ(kModulus16, UpTimer16::ModulusValue);
    EXPECT_EQ(4U, UpTimer16::kTimeValueBitWidth);
    EXPECT_FALSE(UpTimer16::kIsDownCounter);
    EXPECT_TRUE(UpTimer16::kIsPow2CounterModulus);
}

// Verifies compile-time traits for a down-counting 16-tick timer.
TEST(HardwareTimerTraitsTest, DownCounterReportsDirection) {
    EXPECT_TRUE(DownTimer16::kIsDownCounter);
    EXPECT_TRUE(DownTimer16::kIsPow2CounterModulus);
    EXPECT_EQ(4U, DownTimer16::kTimeValueBitWidth);
}

// Verifies up-counter conversion leaves the raw value unchanged.
TEST(HardwareTimerConversionTest, UpCounterRawIsIdentity) {
    EXPECT_EQ(0, UpTimer16::counterValueFromRaw(0));
    EXPECT_EQ(5, UpTimer16::counterValueFromRaw(5));
    EXPECT_EQ(15, UpTimer16::counterValueFromRaw(15));
}

// Verifies down-counter conversion reports ticks elapsed since reload.
TEST(HardwareTimerConversionTest, DownCounterInvertsRawValue) {
    EXPECT_EQ(15, DownTimer16::counterValueFromRaw(0));
    EXPECT_EQ(10, DownTimer16::counterValueFromRaw(5));
    EXPECT_EQ(0, DownTimer16::counterValueFromRaw(15));
}

// Verifies conversion is its own inverse for every raw value in range.
TEST(HardwareTimerConversionTest, DownCounterConversionIsInvolution) {
    for (uint8_t raw{0}; raw < kModulus16; ++raw) {
        const uint8_t converted{DownTimer16::counterValueFromRaw(raw)};
        EXPECT_EQ(raw, DownTimer16::counterValueFromRaw(converted));
    }
}

// Verifies an up counter starts at 0 with no pending wrap.
TEST_F(SimulatedUpTimer16Test, StartsAtZeroWithoutWrap) {
    EXPECT_EQ(0, hw_.readRaw());
    EXPECT_EQ(0, hw_.readCounterValue());
    EXPECT_FALSE(hw_.checkWrapAround());
}

// Verifies advance updates the counter without setting the wrap flag.
TEST_F(SimulatedUpTimer16Test, AdvanceWithoutWrap) {
    hw_.advance(5);
    EXPECT_EQ(5, hw_.readRaw());
    EXPECT_FALSE(hw_.checkWrapAround());
}

// Verifies wrapOnce reloads to 0 and reports a single wrap event.
TEST_F(SimulatedUpTimer16Test, WrapOnceSetsFlagAndPostWrapValue) {
    hw_.setRaw(12);
    hw_.wrapOnce();
    EXPECT_EQ(0, hw_.readRaw());
    EXPECT_TRUE(hw_.checkWrapAround());
    EXPECT_FALSE(hw_.checkWrapAround());
}

// Verifies CRTP forwarding through a HardwareTimer base reference.
TEST_F(SimulatedUpTimer16Test, CrtpDispatchThroughBaseReference) {
    hw_.advance(3);
    hal::HardwareTimer<UpTimer16, uint8_t, kModulus16, false>& base{hw_};
    EXPECT_EQ(3, base.readRaw());
    EXPECT_EQ(3, base.readCounterValue());
}

// Verifies a down counter starts at the reload value (ticks-since-wrap = 0).
TEST_F(SimulatedDownTimer16Test, StartsAtReloadValue) {
    EXPECT_EQ(15, hw_.readRaw());
    EXPECT_EQ(0, hw_.readCounterValue());
    EXPECT_FALSE(hw_.checkWrapAround());
}

// Verifies down-counting advance decreases the raw value and increases ticks.
TEST_F(SimulatedDownTimer16Test, AdvanceDecrementsRaw) {
    hw_.advance(5);
    EXPECT_EQ(10, hw_.readRaw());
    EXPECT_EQ(5, hw_.readCounterValue());
    EXPECT_FALSE(hw_.checkWrapAround());
}

// Verifies a down-counter wrap reloads to modulus-1.
TEST_F(SimulatedDownTimer16Test, WrapOnceReloadsToMax) {
    hw_.advance(5);
    hw_.wrapOnce();
    EXPECT_EQ(15, hw_.readRaw());
    EXPECT_EQ(0, hw_.readCounterValue());
    EXPECT_TRUE(hw_.checkWrapAround());
}

// Verifies the extended timer reads 0 before any ticks elapse.
TEST_F(ExtendedUpTimer16Test, InitialReadIsZero) {
    EXPECT_EQ(0U, timer_.read());
}

// Verifies read returns elapsed hardware ticks when TickDuration is 1.
TEST_F(ExtendedUpTimer16Test, ReadReflectsElapsedTicks) {
    hw_.advance(7);
    EXPECT_EQ(7U, timer_.read());
}

// Verifies a wrap observed inside read() extends the software counter.
TEST_F(ExtendedUpTimer16Test, ReadConsumesWrapWithoutIsr) {
    hw_.wrapOnce();
    EXPECT_EQ(kModulus16, timer_.read());
    EXPECT_EQ(kModulus16, timer_.read());
}

// Verifies an ISR-handled wrap is not applied a second time on read.
TEST_F(ExtendedUpTimer16Test, IsrWrapIsNotDoubleCounted) {
    hw_.wrapOnce();
    timer_.handleTimerWrapInterrupt();
    EXPECT_EQ(kModulus16, timer_.read());
}

// Verifies a wrap interrupt with no pending wrap leaves the count unchanged.
TEST_F(ExtendedUpTimer16Test, IsrWithoutWrapDoesNothing) {
    hw_.advance(4);
    timer_.handleTimerWrapInterrupt();
    EXPECT_EQ(4U, timer_.read());
}

// Verifies successive wraps accumulate as modulus * wrap_count + remainder.
TEST_F(ExtendedUpTimer16Test, MultipleWrapsAccumulate) {
    wrapWithIsr(hw_, timer_, 3);
    hw_.advance(5);
    EXPECT_EQ(3U * kModulus16 + 5U, timer_.read());
}

// Verifies TickDuration multiplies the combined tick count.
TEST(SoftwareExtendedTimerTest, TickDurationScalesResult) {
    UpTimer16 hw{};
    hal::SoftwareExtendedTimer<UpTimer16, uint64_t, uint64_t{10}> timer{hw};
    hw.advance(5);
    EXPECT_EQ(50U, timer.read());
    hw.wrapOnce();
    EXPECT_EQ(10U * kModulus16, timer.read());
}

// Verifies a down-counting hardware timer reports the same elapsed ticks.
TEST_F(ExtendedDownTimer16Test, ReadMatchesTicksSinceReload) {
    hw_.advance(5);
    EXPECT_EQ(5U, timer_.read());
    hw_.wrapOnce();
    EXPECT_EQ(kModulus16, timer_.read());
}
