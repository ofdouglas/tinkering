#pragma once
#include "drivers/uart.h"

template <unsigned N>
static void print_text(const char (&text)[N]) {
    uart_send_string_blocking(text, N - 1);
}

static void print_number(uint32_t value) {
    char digits[10];
    unsigned count = 0;
    do {
        digits[count++] = '0' + value % 10;
        value /= 10;
    } while (value);
    while (count) uart_send_byte_blocking(digits[--count]);
}
