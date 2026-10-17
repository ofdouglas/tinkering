#pragma once
/*
 * @file  watchdog_interface.h
 * @brief Platform-independent watchdog interface.
 */

namespace hal {

/**
 * @brief Interface for starting and servicing a hardware watchdog.
 */
class WatchdogInterface {
public:
    virtual ~WatchdogInterface() = default;

    /** @brief Starts the watchdog. */
    virtual void start() noexcept = 0;

    /** @brief Services the watchdog to prevent a reset. */
    virtual void kick() noexcept = 0;
};

}  // namespace hal
