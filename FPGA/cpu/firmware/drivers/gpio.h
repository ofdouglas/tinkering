#ifndef GPIO_H
#define GPIO_H

#include <stdint.h>
#include <stdbool.h>

void gpio_set_led(uint32_t index, bool value);

#endif