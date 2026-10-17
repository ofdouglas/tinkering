#pragma once
/*
 * @file  simulated_hardware_timer.h
 * @brief Simulated HardwareTimer for host unit tests.
 * @note  This file is not intended for use in production code.
 */

#include "hal/sw_extended_timer.h"

#include <cstdint>

namespace hal::test {

/**
 * @brief Programmable hardware timer with an explicit wrap flag.
 *
 * Advance and wrapOnce update the counter and wrap flag together so a wrap
 * indication always corresponds to a post-wrap raw value, matching the
 * HardwareTimer contract used by SoftwareExtendedTimer.
 *
 * @tparam ValueType      Unsigned counter storage type.
 * @tparam CounterModulus Power-of-two number of ticks per wrap.
 * @tparam DownCounter    True if the counter counts down.
 */
template <typename ValueType, uint64_t CounterModulus, bool DownCounter = false>
class SimulatedHardwareTimer
    : public HardwareTimer<SimulatedHardwareTimer<ValueType, CounterModulus, DownCounter>,
                           ValueType, CounterModulus, DownCounter> {
public:
    using HwValueType = ValueType;
    static constexpr HwValueType kMaxRaw{static_cast<HwValueType>(CounterModulus - 1U)};

    SimulatedHardwareTimer() noexcept
        : raw_value_{DownCounter ? kMaxRaw : HwValueType{0}} {}

    bool checkWrapAround() noexcept {
        const bool wrapped{wrap_pending_};
        wrap_pending_ = false;
        return wrapped;
    }

    HwValueType readRaw() noexcept { return raw_value_; }

    /**
     * @brief Sets the raw counter without changing the wrap flag.
     * @param[in] value Raw counter value; must be in [0, CounterModulus).
     */
    void setRaw(HwValueType value) noexcept { raw_value_ = value; }

    /**
     * @brief Advances the timer by tick_count ticks in the counting direction.
     * @param[in] tick_count Number of hardware ticks to apply.
     */
    void advance(uint64_t tick_count) noexcept {
        for (uint64_t i{0}; i < tick_count; ++i) {
            advanceOne();
        }
    }

    /**
     * @brief Advances exactly to the next wrap, leaving a post-wrap raw value.
     */
    void wrapOnce() noexcept {
        if constexpr (DownCounter) {
            advance(static_cast<uint64_t>(raw_value_) + 1U);
        } else {
            advance(CounterModulus - static_cast<uint64_t>(raw_value_));
        }
    }

private:
    void advanceOne() noexcept {
        if constexpr (DownCounter) {
            if (raw_value_ == HwValueType{0}) {
                raw_value_ = kMaxRaw;
                wrap_pending_ = true;
            } else {
                --raw_value_;
            }
        } else if (raw_value_ == kMaxRaw) {
            raw_value_ = HwValueType{0};
            wrap_pending_ = true;
        } else {
            ++raw_value_;
        }
    }

    HwValueType raw_value_{};
    bool wrap_pending_{false};
};

}  // namespace hal::test
