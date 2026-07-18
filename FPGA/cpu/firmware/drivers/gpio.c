#include <stdint.h>
#include <stdbool.h>

#include "linker/mem_map.h"

#define LED_ON_REG ((volatile uint32_t*)(LED_BASE + 0))

void gpio_set_led(uint32_t index, bool value) {
    if (value) {
        *LED_ON_REG |= (1U << index);
    } else {
        *LED_ON_REG &= ~(1U << index);
    }
}
