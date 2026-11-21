#define _POSIX_C_SOURCE 200809L

#include "drivers/time.h"

#include <time.h>

uint64_t mtim_read_nanosec(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ((uint64_t)ts.tv_sec * NS_PER_SEC) + (uint64_t)ts.tv_nsec;
}

uint64_t mtim_read_ticks(void) {
    return mtim_read_nanosec() / NS_PER_TICK;
}

void mtim_write_compare(const uint64_t value) {
    (void)value;
}

void mtim_delay_ns(const uint64_t ns) {
    const uint64_t start_ns = mtim_read_nanosec();
    while (mtim_read_nanosec() < (start_ns + ns)) {
        ;
    }
}

void mtim_delay_ns_irq(const uint64_t ns) {
    mtim_delay_ns(ns);
}

void delay_loop(const uint32_t count) {
    volatile uint32_t remaining = count;
    while (remaining > 0U) {
        remaining--;
    }
}
