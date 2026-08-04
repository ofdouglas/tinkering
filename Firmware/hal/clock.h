#pragma once

#include <stdint.h>
#include <chrono>

#include "bsp/base_bsp.h"

namespace hal {

// @brief Default clock for FW - monotonic 64-bit nanosecond clock
struct PlatformClock {
    using Rep = uint64_t;
    using Period = std::nano;
    using Duration = std::chrono::nanoseconds;
    using TimePoint = std::chrono::time_point<PlatformClock, Duration>;
    
    static constexpr bool IsSteady = true;
    static constexpr TimePoint kTimeZero = TimePoint(Duration(0));
    static constexpr TimePoint kTimeMax = TimePoint(Duration(std::numeric_limits<Duration::rep>::max()));


    static TimePoint now() noexcept;
};

// @brief RTOS tick counter for scheduling tasks - monotonic 32-bit millisecond clock
struct SchedulerClock {
    using Rep = uint32_t;
    using Period = std::milli;
    using Duration = std::chrono::milliseconds;
    using TimePoint = std::chrono::time_point<SchedulerClock, Duration>;

    static constexpr bool IsSteady = true;
    static constexpr TimePoint kTimeZero = TimePoint(Duration(0));
    static constexpr TimePoint kTimeMax = TimePoint(Duration(std::numeric_limits<Duration::rep>::max()));
    
    static TimePoint now() noexcept;
};

} // namespace hal
