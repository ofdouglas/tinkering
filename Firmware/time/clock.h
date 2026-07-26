#ifndef TIME_CLOCK_H
#define TIME_CLOCK_H

#include <stdint.h>
#include <chrono>

#include "bsp/base_bsp.h"

namespace time {

/**
 * @brief  Type for a clock.
 * @tparam ClockT  The clock type.
 * @tparam rep     The type of the clock's representation.
 * @tparam period  The period of the clock.
 * @tparam is_steady  Whether the clock is steady.
 */
template<typename ClockT, typename rep, typename period, bool is_steady = true>
struct ClockType {
    using Rep = typename ClockT::rep;
    using Period = typename ClockT::period;
    using Duration = typename ClockT::duration;
    using TimePoint = typename ClockT::time_point;
    static constexpr bool IsSteady = is_steady;

    static TimePoint now() noexcept;
};

// @brief Default clock for FW - monotonic 64-bit nanosecond clock
using PlatformClock = ClockType<std::chrono::steady_clock, uint64_t, std::nano, true>;

// @brief RTOS tick counter for scheduling tasks - monotonic 32-bit millisecond clock
using SchedulerClock = ClockType<std::chrono::steady_clock, uint32_t, std::milli, true>;

} // namespace time
#endif // TIME_CLOCK_H