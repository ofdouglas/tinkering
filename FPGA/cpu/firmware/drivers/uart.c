#include "drivers/uart.h"
#include "linker/mem_map.h"

#include <stdint.h>
#include <stdbool.h>

#define UART_STATUS_REG  ((volatile uint32_t*)(UART_BASE + 0))
#define UART_TX_DATA_REG ((volatile uint32_t*)(UART_BASE + 4))
#define UART_RX_DATA_REG ((volatile uint32_t*)(UART_BASE + 8))

#define UART_STATUS_TX_READY (1U << 0)
#define UART_STATUS_RX_VALID (1U << 1)

bool uart_putchar_nonblocking(char c) {
    if (!(*UART_STATUS_REG & UART_STATUS_TX_READY)) {
        return false;
    }
    *UART_TX_DATA_REG = (uint32_t)c;
    return true;
}

int uart_getchar_nonblocking(void) {
    if (*UART_STATUS_REG & UART_STATUS_RX_VALID) {
        return *UART_RX_DATA_REG;
    }
    return -1;
}

void uart_send_string_blocking(const char* str, const uint32_t length) {
    for (uint32_t i = 0U; i < length; i++) {
        while (!uart_putchar_nonblocking(str[i])) {
            ;
        }
    }
}
