#pragma once
/*
 * @file  watchdog.h
 * @brief STM32F746 independent watchdog driver.
 */

#include "hal/watchdog_interface.h"

/**
 * @brief Controls the STM32F746 independent watchdog.
 */
class Watchdog final : public hal::WatchdogInterface {
public:
    Watchdog() noexcept = default;

    Watchdog(const Watchdog&) = delete;
    Watchdog& operator=(const Watchdog&) = delete;
    Watchdog(Watchdog&&) = delete;
    Watchdog& operator=(Watchdog&&) = delete;

    /** @brief Starts the independent watchdog. */
    void start() noexcept override;

    /** @brief Reloads the independent watchdog counter. */
    void kick() noexcept override;
};
