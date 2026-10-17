#pragma once

#include "stm32f746xx.h"

#include "hal/sw_extended_timer.h"

class Lptimer : public hal::HardwareTimer<Lptimer, uint16_t, 0x10000U, false> {
public:
    Lptimer(LPTIM_TypeDef* lptim) noexcept : lptim_(lptim) {}
    ~Lptimer() noexcept = default;

    void init() noexcept {
        // Interrupt on reload value
        lptim_->IER |= LPTIM_IER_ARRMIE;

        lptim_->CR |= LPTIM_CR_ENABLE;
        while ((lptim_->CR & LPTIM_CR_ENABLE) == 0U) {
            ;  // wait for the timer to be enabled.
        }

        // Configure LPTIM as a 16-bit upcounter. No prescaler.
        lptim_->ARR |= 0xFFFFU;

        // Start the timer.
        lptim_->CR |= LPTIM_CR_CNTSTRT;
    }

    bool checkWrapAround() noexcept {
        const uint32_t isr = lptim_->ISR;
        lptim_->ICR |= isr;
        return (isr & LPTIM_ISR_ARRM) != 0U;
    }

    /**
     * @brief Reads the value of the LPTIM counter.
     *
     * The counter is read multiple times to account for the asynchronous clock. From the reference
     * manual: "When the LPTIM is running with an asynchronous clock, reading the LPTIM_CNT register
     * may return unreliable values. So in this case it is necessary to perform two consecutive read
     * accesses and verify that the two returned values are identical."
     */
    uint16_t readRaw() noexcept {
        uint16_t read_first = lptim_->CNT;
        uint16_t read_second{};

        for (size_t i = 0U; i < kMaxReadRetries; i++) {
            read_second = lptim_->CNT;
            if (read_first == read_second) {
                break;
            }
            read_first = read_second;
        }
        return read_second;
    }

private:
    static constexpr size_t kMaxReadRetries = 10U;
    LPTIM_TypeDef* const lptim_;
};