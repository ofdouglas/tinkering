#pragma once
/*
 * @file  bsp.h
 * @brief STM32F746 Discovery board initialization.
 */

#include <cstdint>

#include "stm32f746xx.h"

#include "hal/reset_manager_interface.h"
#include "hal/watchdog_interface.h"
#include "interfaces/stream_interface.h"
#include "logging/logging.h"
#include "uart.h"
#include "watchdog.h"

class ResetManager : public hal::ResetManagerInterface {
public:
    void readResetCause() noexcept override {
        value_ = RCC->CSR;
        RCC->CSR |= RCC_CSR_RMVF;
    }

    void logResetCause() noexcept override {
        LOG_INFO() << "Reset Cause:" << ((value_ & RCC_CSR_LPWRRSTF) ? " LPWR" : "")
                   << ((value_ & RCC_CSR_WWDGRSTF) ? " WWDG" : "")
                   << ((value_ & RCC_CSR_IWDGRSTF) ? " IWDG" : "")
                   << ((value_ & RCC_CSR_SFTRSTF) ? " SFT" : "")
                   << ((value_ & RCC_CSR_PORRSTF) ? " POR" : "")
                   << ((value_ & RCC_CSR_PINRSTF) ? " PIN" : "")
                   << ((value_ & RCC_CSR_BORRSTF) ? " BOR" : "");
    }

    void reset() noexcept override {
        NVIC_SystemReset();
    }

private:
    uint32_t value_;
};

/**
 * @brief Initializes board-level CPU, clock, SysTick, and GPIO resources.
 */
class Bsp {
public:
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
    void initCpu() noexcept;
    void initClocks() noexcept;
    void initSysTick() noexcept;
    void initGpio() noexcept;

    ResetManager reset_manager_;
    [[maybe_unused]] Watchdog watchdog_;
    Uart uart_;
};
