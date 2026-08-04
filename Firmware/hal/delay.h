#pragma once

#include "hal/clock.h"

namespace hal {

/**
 * @brief  Delay for a given duration.
 * @tparam ClockType  The clock type.
 * @param d  The duration to delay for.
 */
template<typename ClockType>
void delayFor(typename ClockType::Duration d) {
    const auto end = typename ClockType::TimePoint(ClockType::now()) + d;
    while (typename ClockType::TimePoint(ClockType::now()) < end) {
        __asm__ volatile ("" ::: "memory");
    }
}

} // namespace hal
