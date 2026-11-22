#include "drivers/time.h"
#include "drivers/uart.h"

extern "C" void coremark_uart_init() { uart_init(); }
extern "C" void coremark_putchar(char c) { uart_send_byte_blocking(c); }
extern "C" uint64_t coremark_milliseconds() {
    return mtim_read_ticks() / (MTIM_FREQ_HZ / 1000);
}
