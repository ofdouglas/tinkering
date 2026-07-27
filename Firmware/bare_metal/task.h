#ifndef BARE_METAL_TASK_H
#define BARE_METAL_TASK_H

#include "hal/clock.h"

namespace bare_metal {

class PeriodicTask {
public:
    using ClockType = hal::SchedulerClock;

    PeriodicTask(ClockType::Duration period) : period_(period), next_tick_time_(ClockType::kTimeMax) {}

    ~PeriodicTask() noexcept = default;
    
    virtual bool start() noexcept {
        next_tick_time_ = ClockType::now() + period_;
        return true;
    }

    /**
        * @brief Run the tick() if it is time to do so.
        * @note  The next tick time is relative to the current time, so delayed
        *        ticks can shift the cadence.
        */
    virtual void poll() noexcept {
        const auto now = ClockType::now();
        if (now >= next_tick_time_) {
            tick();
            next_tick_time_ = now + period_;
        }
    }

    /**
        * @brief Actual work to be done.
        */
    virtual void tick() noexcept = 0;

private:
    const ClockType::Duration period_;
    ClockType::TimePoint next_tick_time_{ClockType::kTimeMax};
};

} // namespace bare_metal

#endif // BARE_METAL_TASK_H