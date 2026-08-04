#include "hal/mock_clocks.h"

namespace hal {

namespace {

uint64_t& platformTicks() {
    static uint64_t ticks{0U};
    return ticks;
}

uint32_t& schedulerTicks() {
    static uint32_t ticks{0U};
    return ticks;
}

} // namespace

PlatformClock::TimePoint PlatformClock::now() noexcept {
    return PlatformClock::TimePoint(PlatformClock::Duration(platformTicks()));
}

SchedulerClock::TimePoint SchedulerClock::now() noexcept {
    return SchedulerClock::TimePoint(SchedulerClock::Duration(schedulerTicks()));
}

namespace mock {

void resetPlatformClock() noexcept {
    platformTicks() = 0U;
}

void advancePlatformClock(PlatformClock::Duration delta) noexcept {
    platformTicks() += static_cast<uint64_t>(delta.count());
}

void resetSchedulerClock() noexcept {
    schedulerTicks() = 0U;
}

void advanceSchedulerClock(SchedulerClock::Duration delta) noexcept {
    schedulerTicks() += static_cast<uint32_t>(delta.count());
}

} // namespace mock

} // namespace hal
