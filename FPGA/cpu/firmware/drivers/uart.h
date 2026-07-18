#ifndef UART_H
#define UART_H

#include <stdint.h>
#include <stdbool.h>

void uart_send_string_blocking(const char* str, const uint32_t length);

bool uart_putchar_nonblocking(char c);

int uart_getchar_nonblocking(void);

#endif