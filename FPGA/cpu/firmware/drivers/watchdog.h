#ifndef WATCHDOG_H
#define WATCHDOG_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/*
 * @brief Configures the watchdog. Cannot be called while the watchdog is running.
 * @note  The watchdog timeout period is (T_clk * prescaler * watchdog_timeout)
 * @param[in] prescaler_max The prescaler value - 1.
 * @param[in] watchdog_max  The watchdog_timeout value - 1.
 */
void watchdog_configure(uint16_t prescaler_max, uint16_t watchdog_max);

/*
 * @brief Starts the watchdog.
 * @note Must be called after configuring the watchdog.
 */
void watchdog_start(void);

/*
 * @brief Services the watchdog.
 */
void watchdog_service(void);

/*
 * @brief Stops the watchdog.
 */
void watchdog_stop(void);

/*
 * @brief Resets the watchdog module registers. Must be stopped first.
 */
void watchdog_reset(void);

#endif  // WATCHDOG_H