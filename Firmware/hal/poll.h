#pragma once
/*
 * @file  poll.h
 * @brief Platform-independent polling interface.
 */

#include <chrono>

#include "hal/clock.h"
#include "logging/logging.h"

namespace hal {

static constexpr auto kDefaultPollingTimeout{std::chrono::milliseconds(10U)};

/*
 * @brief Polls a condition until a timeout is reached. The condition is always
 * evaluated at least once.
 *
 * @param end_time_point The end time point.
 * @param condition The condition to poll.
 * @return True if the condition evaluates to true before the end time point is reached.
 */
template <typename ClockType, typename Condition>
bool pollUntil(typename ClockType::TimePoint end_time_point, Condition condition) {
    do {
        if (condition()) {
            return true;
        }
    } while (ClockType::now() < end_time_point);
    return false;
}

/*
 * @brief Polls a condition for a given duration. The condition is always
 * evaluated at least once.
 *
 * @param duration The duration to poll.
 * @param condition The condition to poll.
 * @return True if the condition evaluates to true before the duration is reached.
 */
template <typename ClockType, typename Condition>
bool pollFor(typename ClockType::Duration duration, Condition condition) {
    return pollUntil<ClockType>(typename ClockType::TimePoint(ClockType::now()) + duration,
                                condition);
}

template <typename ClockType, typename Condition>
bool pollOrLogError(const char *name, typename ClockType::Duration duration, Condition condition) {
    if (hal::pollFor<ClockType>(duration, condition)) {
        return true;
    }
    LOG_ERROR() << name << " timed out";
    return false;
}

}  // namespace hal