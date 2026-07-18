#ifndef UART_H
#define UART_H

#include <stdint.h>

void uart_send_string(const char* str, const uint32_t length);
// void uart_send_char(const char c); // TODO

#endif