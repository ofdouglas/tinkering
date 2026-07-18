#include "drivers/uart.h"
#include "linker/mem_map.h"

#define UART_STATUS_REG  ((volatile uint32_t*)(UART_BASE + 0))
#define UART_TX_DATA_REG ((volatile uint32_t*)(UART_BASE + 4))
#define UART_RX_DATA_REG ((volatile uint32_t*)(UART_BASE + 8))

void uart_send_string(const char* str, const uint32_t length) {
    for (uint32_t i = 0U; i < length; i++) {
        for (uint32_t j = 0U; j < 10000U; j++) { // wait for TX ready (bit 0)
            if (*UART_STATUS_REG & 0x01U) {
                break;
            }
        }
        *UART_TX_DATA_REG = (uint32_t)str[i];
    }
}
