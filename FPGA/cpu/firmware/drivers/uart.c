#include "drivers/uart.h"
#include "linker/mem_map.h"
#include "system/debug.h"
#include "util/ringbuf.h"
#include "system/intrinsics.h"
#include <stdint.h>
#include <stdbool.h>

#define UART_STATUS_REG     ((volatile uint32_t*)(UART_BASE + 0))
#define UART_IRQ_ENABLE_REG ((volatile uint32_t*)(UART_BASE + 4))
#define UART_TX_DATA_REG    ((volatile uint8_t*)(UART_BASE + 8))
#define UART_RX_DATA_REG    ((volatile uint8_t*)(UART_BASE + 12))

#define UART_STATUS_TX_READY (1U << 0)
#define UART_STATUS_RX_VALID (1U << 1)

#define UART_IRQ_ENABLE_TX_READY (1U << 0)
#define UART_IRQ_ENABLE_RX_VALID (1U << 1)


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
#define UART_TX_BUFFER_SIZE 16
volatile uint8_t uart_rx_buffer[UART_RX_BUFFER_SIZE];
volatile uint8_t uart_tx_buffer[UART_TX_BUFFER_SIZE];
ringbuf_t uart_rx_ringbuf;
ringbuf_t uart_tx_ringbuf;

void uart_init(void) {
    *UART_IRQ_ENABLE_REG = UART_IRQ_ENABLE_RX_VALID;

    ringbuf_init(&uart_rx_ringbuf, uart_rx_buffer, UART_RX_BUFFER_SIZE);
    ringbuf_init(&uart_tx_ringbuf, uart_tx_buffer, UART_TX_BUFFER_SIZE);
}

bool uart_receive_byte(uint8_t* data) {
    if (ringbuf_dequeue(&uart_rx_ringbuf, data)) {
        return true;
    }
    return false;
}

volatile bool tx_pending = false;

bool uart_send_byte_nonblocking(uint8_t data) {
    global_irq_disable();
    const bool enqueue_ok = ringbuf_enqueue(&uart_tx_ringbuf, data);
    checkpoint(16);
    if (enqueue_ok) {
        tx_pending = true;
        *UART_IRQ_ENABLE_REG |= UART_IRQ_ENABLE_TX_READY;
        checkpoint(17);
    }
    global_irq_enable();
    return enqueue_ok;
}

bool uart_send_string_nonblocking(const char* data, size_t length) {
    bool success = true;
    for (size_t i = 0; i < length; i++) {
        if (!uart_send_byte_nonblocking(data[i])) {
            success = false;
            break;
        }
    }
    return success;
}

#ifdef __cplusplus
extern "C" {
#endif

void mei_isr(void) {
    checkpoint(8);
    const uint32_t status = *UART_STATUS_REG;
    checkpoint(9);
    if (status & UART_STATUS_RX_VALID) {
        checkpoint(10);
        const uint8_t data = *UART_RX_DATA_REG;
        checkpoint(11);
        ringbuf_enqueue(&uart_rx_ringbuf, data);
        checkpoint(12);
    }

    if (tx_pending && (status & UART_STATUS_TX_READY)) {
        if (ringbuf_is_empty(&uart_tx_ringbuf)) {
            *UART_IRQ_ENABLE_REG &= ~UART_IRQ_ENABLE_TX_READY;
            tx_pending = false;
            checkpoint(15);
        } else {
            uint8_t data;
            ringbuf_dequeue(&uart_tx_ringbuf, &data);
            *UART_TX_DATA_REG = data;
            checkpoint(14);
        }
    }
}

#ifdef __cplusplus
}
#endif
