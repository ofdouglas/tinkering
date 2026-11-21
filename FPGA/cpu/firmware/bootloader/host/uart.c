#define _DEFAULT_SOURCE
#define _POSIX_C_SOURCE 200809L

#include "drivers/uart.h"

#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <termios.h>
#include <unistd.h>

static int uart_fd = STDIN_FILENO;

void uart_host_init(void) {
    uart_fd = STDIN_FILENO;

    if (isatty(STDIN_FILENO)) {
        struct termios tio;
        if (tcgetattr(STDIN_FILENO, &tio) == 0) {
            cfmakeraw(&tio);
            tio.c_cc[VMIN] = 0;
            tio.c_cc[VTIME] = 0;
            tcsetattr(STDIN_FILENO, TCSANOW, &tio);
        }
    }

    const int in_flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    if (in_flags >= 0) {
        fcntl(STDIN_FILENO, F_SETFL, in_flags | O_NONBLOCK);
    }
    const int out_flags = fcntl(STDOUT_FILENO, F_GETFL, 0);
    if (out_flags >= 0) {
        fcntl(STDOUT_FILENO, F_SETFL, out_flags | O_NONBLOCK);
    }
}

bool uart_putchar_nonblocking(char c) {
    const ssize_t written = write(STDOUT_FILENO, &c, 1);
    if (written == 1) {
        return true;
    }
    if ((written < 0) && ((errno == EAGAIN) || (errno == EWOULDBLOCK))) {
        return false;
    }
    return false;
}

int uart_getchar_nonblocking(void) {
    unsigned char byte = 0;
    const ssize_t n_read = read(uart_fd, &byte, 1);
    if (n_read == 1) {
        return (int)byte;
    }
    return -1;
}

void uart_send_byte_blocking(uint8_t data) {
    while (!uart_putchar_nonblocking((char)data)) {
        ;
    }
}

void uart_send_string_blocking(const char* str, const uint32_t length) {
    for (uint32_t i = 0U; i < length; i++) {
        uart_send_byte_blocking((uint8_t)str[i]);
    }
}

bool uart_send_string_nonblocking(const char* str, size_t length) {
    for (size_t i = 0; i < length; i++) {
        if (!uart_putchar_nonblocking(str[i])) {
            return false;
        }
    }
    return true;
}

bool uart_send_byte_nonblocking(uint8_t data) {
    return uart_putchar_nonblocking((char)data);
}

void uart_init(void) {
    uart_host_init();
}

bool uart_receive_byte(uint8_t* data) {
    const int rx_byte = uart_getchar_nonblocking();
    if (rx_byte < 0) {
        return false;
    }
    *data = (uint8_t)rx_byte;
    return true;
}
