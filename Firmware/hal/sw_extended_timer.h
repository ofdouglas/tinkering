#pragma once
/**
 * @file  sw_extended_timer.h
 * @brief Software extended timer
 */

#include <atomic>
#include <cstdint>
#include <cstddef>
#include <limits>
#include <type_traits>

#include "osal/critical_section.h"
#include "util/bit.h"

namespace hal {

/**
 * @brief  Hardware timer interface for time-keeping. The timer must provide the raw timer value,
 * as well as indication of wrap around events.
 *
 * @tparam HwValueType The integer type of the hardware timer value (e.g. uint32_t, uint16_t, etc.)
 * @tparam CounterModulus The modulus of the counter (e.g. number of ticks per wrap)
 * @tparam DownCounter Whether the timer is a down counter.
 */
template <typename Derived, typename ValueType, uint64_t CounterModulus, bool DownCounter = false>
class HardwareTimer {
public:
    using HwValueType = ValueType;
    static constexpr uint64_t ModulusValue{CounterModulus};

    static_assert(std::is_integral_v<HwValueType> && std::is_unsigned_v<HwValueType>,
                  "HwValueType must be an unsigned integer");
    static_assert(CounterModulus > 1U, "Counter modulus must be greater than 1");

    static constexpr uint64_t kCounterModulus = CounterModulus;
    static constexpr size_t kTimeValueBitWidth = util::bitWidth(CounterModulus - 1U);
    static constexpr bool kIsDownCounter = DownCounter;
    static constexpr bool kIsPow2CounterModulus = (CounterModulus & (CounterModulus - 1U)) == 0U;

    /**
     * @brief Checks for (and clears) a wrap around event. Returns true if the timer has wrapped
     * around since the last call to checkWrapAround(). A read of the timer immediately after
     * this function returns true *must* return a post-wrap value: if the counter update and
     * overflow event are not atomic, the implementation must block until the counter is updated.
     * @return True if the timer has wrapped around, false otherwise.
     */
    bool checkWrapAround() noexcept {
        return static_cast<Derived*>(this)->checkWrapAround();
    }

    /**
     * @brief Reads the raw value of the timer.
     * @return The raw value of the timer.
     */
    HwValueType readRaw() noexcept {
        return static_cast<Derived*>(this)->readRaw();
    }

    /**
     * @brief Reads the counter value of the timer.
     * @return The counter value of the timer.
     */
    HwValueType readCounterValue() noexcept {
        return counterValueFromRaw(readRaw());
    }

    /**
     * @brief Converts a down counter value to an up counter value, or vice versa.
     * @param[in] timer_value The value to convert.
     * @return The converted value.
     */
    static constexpr HwValueType counterValueFromRaw(HwValueType raw_value) noexcept {
        if constexpr (DownCounter) {
            return static_cast<HwValueType>(CounterModulus - 1U) - raw_value;
        }
        return raw_value;
    }
};

/**
 * @brief Extends a hardware timer using a software wrap counter.
 *
 * @note User code must handle interrupt integration. Call handleTimerWrapInterrupt()
 * from the timer wrap interrupt.
 *
 * @warning The timer value may overflow if the extension type or return type is too small. The
 * user must ensure the return type is large enough to avoid overflow, or handle the overflow
 * appropriately (e.g. use modular time arithmetic).
 *
 * @tparam HardwareTimerType The hardware timer type.
 * @tparam ReturnType The return type.
 * @tparam TickDuration The tick duration.
 * @tparam ExtensionType The extension type.
 */
template <typename HardwareTimerType, typename ReturnType, ReturnType TickDuration,
          typename ExtensionType = uint32_t>
class SoftwareExtendedTimer {
    using HwTimerType = HardwareTimerType;
    using HwTimerBaseType = HardwareTimer<HwTimerType, typename HwTimerType::HwValueType,
                                          HwTimerType::ModulusValue, HwTimerType::kIsDownCounter>;

    static_assert(std::is_base_of_v<HwTimerBaseType, HwTimerType>,
                  "HardwareTimerType must inherit from HardwareTimer");
    static_assert(HwTimerType::kIsPow2CounterModulus == true,
                  "Extension requires a power-of-two counter modulus for bit-shifting");

    static_assert(std::is_integral_v<ExtensionType> && std::is_integral_v<ReturnType>,
                  "Extension and Return types must be integral");
    static_assert(std::is_unsigned_v<ExtensionType>, "Extension type must be unsigned");
    static_assert(std::numeric_limits<ReturnType>::max() >=
                      std::numeric_limits<ExtensionType>::max(),
                  "Return type cannot be smaller than the extension type");
    static_assert(TickDuration > 0, "Tick duration must be positive");

public:
    /**
     * @brief Constructs a software-extended timer.
     * @param[in] timer The hardware timer to extend.
     */
    explicit SoftwareExtendedTimer(HardwareTimerType& timer) noexcept : timer_(timer) {}

    /**
     * @brief Handles the timer wrap around interrupt.
     */
    void handleTimerWrapInterrupt() noexcept {
        handleTimerWrapAround();
    }

    /**
     * @brief Reads the extended timer value.
     * @return The extended timer value.
     */
    ReturnType read() noexcept {
        HwValueType timer_value{};
        ExtensionType num_wraps{};
        {
            osal::RaiiCriticalSection cs{};

            timer_value = timer_.counterValueFromRaw(timer_.readRaw());
            if (handleTimerWrapAround()) {
                timer_value = timer_.counterValueFromRaw(timer_.readRaw());
            }
            num_wraps = num_wraps_;
        }
        return combineValues(timer_value, num_wraps);
    }

private:
    using HwValueType = typename HardwareTimerType::HwValueType;
    static constexpr size_t kHwValueTypeBitWidth = HardwareTimerType::kTimeValueBitWidth;

    /*
     * @brief  Check for hardware timer wrap around, and increment the wrap counter.
     * @return True if the wrap flag was set, false otherwise.
     */
    bool handleTimerWrapAround() noexcept {
        if (timer_.checkWrapAround()) {
            num_wraps_++;
            return true;
        }
        return false;
    }

    /*
     *  @brief Combine the hardware timer value and the software wrap counter, allowing overflow.
     *  @param[in] timer_value The hardware timer value.
     *  @param[in] num_wraps The number of wraps.
     *  @return The converted return value.
     */
    static constexpr ReturnType combineValues(HwValueType timer_value,
                                              ExtensionType num_wraps) noexcept {
        ReturnType ticks{(static_cast<ReturnType>(num_wraps) << kHwValueTypeBitWidth) +
                         static_cast<ReturnType>(timer_value)};
        return ticks * TickDuration;
    }

    HardwareTimerType& timer_;
    ExtensionType num_wraps_{};
};

}  // namespace hal