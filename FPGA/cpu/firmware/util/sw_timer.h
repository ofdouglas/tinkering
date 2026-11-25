#pragma once
/*
 * @file sw_timer.h
 * @brief Software timer framework for FPGA C firmware.
 */

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "drivers/time.h"

typedef struct {
    uint64_t target_time_ns;
    uint64_t period_ns;
} sw_timer_t;

inline void sw_timer_init(sw_timer_t* timer, uint64_t target_time_ns, uint64_t period_ns) {
    timer->target_time_ns = target_time_ns;
    timer->period_ns = period_ns;
}

inline bool sw_timer_check_elapsed(sw_timer_t* timer) {
    uint64_t current_time_ns = mtim_read_nanosec();
    if (current_time_ns >= timer->target_time_ns) {
        timer->target_time_ns = current_time_ns + timer->period_ns;
        return true;
    }
    return false;
}
