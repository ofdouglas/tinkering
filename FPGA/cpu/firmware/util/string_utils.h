#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "drivers/uart.h"

inline void my_memcpy(uint8_t* dest, const uint8_t* src, size_t n) {
    for (size_t i = 0; i < n; i++) {
        dest[i] = src[i];
    }
}

inline void my_memfill(uint8_t* buf, size_t n, uint8_t value) {
    for (size_t i = 0; i < n; i++) {
        buf[i] = value;
    }
}

inline bool my_strncmp(const char* s1, const char* s2, size_t n) {
    for (size_t i = 0; i < n; i++) {
        if (*s1 != *s2) {
            return false;
        }
        if (*s1 == '\0') {
            return true;
        }
        s1++;
        s2++;
    }
    return true;
}

inline size_t my_strnlen(const char* str, size_t n) {
    for (size_t i = 0; i < n; i++) {
        if (*str == '\0') {
            return i;
        }
        str++;
    }
    return n;
}

inline void send_string_nonblocking(const char* str) {
    uart_send_string_nonblocking(str, my_strnlen(str, 100));  // lol
}

inline void print_number(uint32_t value) {
    char digits[10];
    unsigned count = 0;
    do {
        digits[count++] = '0' + value % 10;
        value /= 10;
    } while (value);
    while (count) uart_send_byte_nonblocking(digits[--count]);
}

inline void log_value(const char* message, uint32_t value) {
    send_string_nonblocking(message);
    print_number(value);
    uart_send_byte_nonblocking('\n');
}

inline void log_string(const char* message, const char* string) {
    send_string_nonblocking(message);
    send_string_nonblocking(string);
    uart_send_byte_nonblocking('\n');
}