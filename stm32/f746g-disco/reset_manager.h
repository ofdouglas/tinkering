#pragma once
/*
 * @file  bsp.h
 * @brief STM32F746 reset cause and soft reset.
 */

#include "stm32f746xx.h"

#include "hal/reset_manager_interface.h"
#include "logging/logging.h"

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