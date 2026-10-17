#pragma once
/*
 * @file  bsp.h
 * @brief STM32F746 Discovery board initialization.
 */

#include <cstdint>

#include "stm32f746xx.h"

#include "hal/sw_extended_timer.h"
#include "hal/reset_manager_interface.h"
#include "hal/watchdog_interface.h"
#include "interfaces/stream_interface.h"
#include "lptimer.h"
#include "reset_manager.h"
#include "uart.h"
#include "watchdog.h"

/**
 * @brief Initializes board-level CPU, clock, SysTick, and GPIO resources.
 */
class Bsp {
public:
    // @warning The LSI oscillator is not very accurate.
    static constexpr uint64_t kLptimerTickDurationNs = 31'250U;  // 1 / 32 kHz == 31.25 us.

    using ExtendedTimerType =
        hal::SoftwareExtendedTimer<Lptimer, uint64_t, kLptimerTickDurationNs, uint64_t>;

    Bsp() noexcept = default;

    Bsp(const Bsp&) = delete;
    Bsp& operator=(const Bsp&) = delete;
    Bsp(Bsp&&) = delete;
    Bsp& operator=(Bsp&&) = delete;

    /** @brief Initializes board resources required by the application. */
    void init() noexcept;

    hal::WatchdogInterface& getWatchdog() noexcept {
        return watchdog_;
    }

    hal::ResetManagerInterface& getResetManager() noexcept {
        return reset_manager_;
    }

    Stream::StreamInterface* getUart() noexcept {
        return &uart_;
    }

private:
    void initLptimer() noexcept;
    void initCpu() noexcept;
    void initClocks() noexcept;
    void initSysTick() noexcept;
    void initGpio() noexcept;

    ResetManager reset_manager_;
    [[maybe_unused]] Watchdog watchdog_;
    Uart uart_;
};
