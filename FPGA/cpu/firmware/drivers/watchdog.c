#include "drivers/watchdog.h"
#include "linker/mem_map.h"
#include <stdint.h>
#include <stdbool.h>

#define WDOG_CONTROL_REG ((volatile uint32_t*)(WDOG_BASE + 0))
#define WDOG_CONFIG_REG ((volatile uint32_t*)(WDOG_BASE + 4))
#define WDOG_SERVICE_REG ((volatile uint32_t*)(WDOG_BASE + 8))

#define WDOG_CONTROL_ENABLE_COUNTING (1U << 0U)
#define WDOG_CONTROL_RESET_MODULE (1U << 1U)

#define WDOG_CONFIG_PRESCALER_SHIFT (0U)
#define WDOG_CONFIG_WATCHDOG_SHIFT (16U)

#define WDOG_SERVICE_KEY1 (0xA703)
#define WDOG_SERVICE_KEY2 (0xF8FC)

#define WDOG_SERVICE_STOP_KEY1 (0x6AED)
#define WDOG_SERVICE_STOP_KEY2 (0x9106)

void watchdog_configure(uint16_t prescaler_max, uint16_t watchdog_max) {
    *WDOG_CONFIG_REG = ((uint32_t)prescaler_max << WDOG_CONFIG_PRESCALER_SHIFT) |
                       ((uint32_t)watchdog_max << WDOG_CONFIG_WATCHDOG_SHIFT);
}

void watchdog_start(void) {
    *WDOG_CONTROL_REG = WDOG_CONTROL_ENABLE_COUNTING;
}

void watchdog_service(void) {
    *WDOG_SERVICE_REG = WDOG_SERVICE_KEY1;
    *WDOG_SERVICE_REG = WDOG_SERVICE_KEY2;
}

void watchdog_stop(void) {
    *WDOG_SERVICE_REG = WDOG_SERVICE_STOP_KEY1;
    *WDOG_SERVICE_REG = WDOG_SERVICE_STOP_KEY2;
}

void watchdog_reset(void) {
    *WDOG_CONTROL_REG = WDOG_CONTROL_RESET_MODULE;
}
