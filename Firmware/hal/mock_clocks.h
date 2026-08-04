#pragma once

#include "hal/clock.h"

namespace hal::mock {

// Host unit tests: link hal/mock_clocks.cpp instead of platform clock sources.

void resetPlatformClock() noexcept;
void advancePlatformClock(PlatformClock::Duration delta) noexcept;

void resetSchedulerClock() noexcept;
void advanceSchedulerClock(SchedulerClock::Duration delta) noexcept;

} // namespace hal::mock
