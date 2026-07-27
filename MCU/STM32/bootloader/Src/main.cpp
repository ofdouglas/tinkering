/**
  ******************************************************************************
  * @file    main.cpp
  * @brief   Bootloader main program
  ******************************************************************************
  */

#include "main.h"
#include "bsp.h"

#include "bare_metal/task.h"
#include "bare_metal/scheduler.h"
#include "bootloader/image_header.h"
#include "hal/clock.h"
#include "hal/delay.h"
#include "logging/log.hpp"
#include "stm32f7xx_it.h"
#include "stm32f746xx.h"

///////////////////////////////////////////////////////////////////////////////
// Task implementations
///////////////////////////////////////////////////////////////////////////////
class Task1000Ms : public bare_metal::PeriodicTask {
public:
    Task1000Ms() : bare_metal::PeriodicTask(std::chrono::milliseconds(1000U)) {}

    void tick() noexcept override {
        const auto now = hal::SchedulerClock::now();
        LOG_INFO() << "Task1000Ms tick. Scheduler clock: " << now.time_since_epoch().count();
    }
};

class Task500Ms : public bare_metal::PeriodicTask {
public:
    Task500Ms(BaseBsp& bsp) : bare_metal::PeriodicTask(std::chrono::milliseconds(500U)), bsp_(bsp) {}

    void tick() noexcept override {
        bsp_.toggleDebugLed();
    }
private:
    BaseBsp& bsp_;
};

void runTasks() noexcept {
    Task1000Ms task1000ms{};
    Task500Ms task500ms{bsp};
    bare_metal::Scheduler<2U> scheduler{};

    scheduler.addTask(&task1000ms);
    scheduler.addTask(&task500ms);
    scheduler.start();
    scheduler.run();    // Does not return
}


///////////////////////////////////////////////////////////////////////////////
// Main function
///////////////////////////////////////////////////////////////////////////////
int main(void) {
    if (!bsp.earlyInit()) {
        LOG_FATAL() << "BSP initialization failed";
        return 1;
    }
    
    LOG_INFO() << "Bootloader started.";
    runTasks();

    while (true) {} // Shouldn't get here
    return 2;
}
