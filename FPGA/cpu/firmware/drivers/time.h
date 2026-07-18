#ifndef TIME_H
#define TIME_H

#include <stdint.h>

#define NS_PER_MICROSEC (1000UL)
#define NS_PER_MILLISEC (NS_PER_MICROSEC * 1000UL)
#define NS_PER_SEC      (1000UL * NS_PER_MILLISEC)
#define MTIM_FREQ_HZ    (50UL * 1000UL * 1000UL)
#define NS_PER_TICK     (NS_PER_SEC / MTIM_FREQ_HZ)

uint64_t mtim_read_ticks(void);

uint64_t mtim_read_nanosec(void);

void mtim_write_compare(const uint64_t value);

void mtim_delay_ns(const uint64_t ns);

void mtim_delay_ns_irq(const uint64_t ns);

void delay_loop(const uint32_t count);

#endif