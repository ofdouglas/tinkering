#pragma once
/*
 * @file  bsp.h
 * @brief STM32F746 Discovery board initialization.
 */

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

private:
    void initCpu() noexcept;
    void initClocks() noexcept;
    void initSysTick() noexcept;
    void initGpio() noexcept;
};
