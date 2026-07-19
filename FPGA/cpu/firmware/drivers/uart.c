#include "drivers/uart.h"
#include "linker/mem_map.h"
#include "util/ringbuf.h"
#include <stdint.h>
#include <stdbool.h>

#define UART_STATUS_REG     ((volatile uint32_t*)(UART_BASE + 0))
#define UART_IRQ_ENABLE_REG ((volatile uint32_t*)(UART_BASE + 4))
#define UART_TX_DATA_REG    ((volatile uint8_t*)(UART_BASE + 8))
#define UART_RX_DATA_REG    ((volatile uint8_t*)(UART_BASE + 12))

#define UART_STATUS_TX_READY (1U << 0)
#define UART_STATUS_RX_VALID (1U << 1)

#define UART_IRQ_ENABLE_RX_VALID (1U << 0)

bool uart_putchar_nonblocking(char c) {
    if (!(*UART_STATUS_REG & UART_STATUS_TX_READY)) {
        return false;
    }
    *UART_TX_DATA_REG = c;
    return true;
}

int uart_getchar_nonblocking(void) {
    if (*UART_STATUS_REG & UART_STATUS_RX_VALID) {
        return *UART_RX_DATA_REG;
    }
    return -1;
}

void uart_send_byte_blocking(uint8_t data) {
    while (!uart_putchar_nonblocking(data)) {
        ;
    }
}

void uart_send_string_blocking(const char* str, const uint32_t length) {
    for (uint32_t i = 0U; i < length; i++) {
        uart_send_byte_blocking(str[i]);
    }
}


#define UART_RX_BUFFER_SIZE 16
volatile uint8_t uart_rx_buffer[UART_RX_BUFFER_SIZE];
ringbuf_t uart_rx_ringbuf;

void uart_rx_init(void) {
    *UART_IRQ_ENABLE_REG = UART_IRQ_ENABLE_RX_VALID;

    ringbuf_init(&uart_rx_ringbuf, uart_rx_buffer, UART_RX_BUFFER_SIZE);
}

// Valid modes: "machine", "supervisor", or "user"
void mei_isr(void) __attribute__((interrupt("machine")));

void mei_isr(void) {
    // TODO: check valid?
    uint8_t data = (uint8_t)*UART_RX_DATA_REG;
    ringbuf_enqueue(&uart_rx_ringbuf, data);
}

bool uart_receive_byte(uint8_t* data) {
    if (ringbuf_dequeue(&uart_rx_ringbuf, data)) {
        return true;
    }
    return false;
}

