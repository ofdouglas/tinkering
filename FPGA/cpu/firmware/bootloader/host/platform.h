#ifndef BOOTLOADER_HOST_PLATFORM_H
#define BOOTLOADER_HOST_PLATFORM_H

#include <stddef.h>
#include <stdint.h>

void uart_host_init(void);
void host_idle_wait(void);
void host_boot_app(const uint8_t* image, size_t length);

#endif
