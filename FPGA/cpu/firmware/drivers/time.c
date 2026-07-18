#include "drivers/time.h"
#include "linker/mem_map.h"

#define MTIM_TIME_LOW_REG  ((volatile uint32_t*)(MTIM_BASE + 0))
#define MTIM_TIME_HIGH_REG ((volatile uint32_t*)(MTIM_BASE + 4))
#define MTIM_COMP_LOW_REG  ((volatile uint32_t*)(MTIM_BASE + 8))
#define MTIM_COMP_HIGH_REG ((volatile uint32_t*)(MTIM_BASE + 12))

uint64_t ns_from_ticks(const uint64_t ticks) {
    return ticks * NS_PER_TICK;
}

uint64_t ticks_from_ns(const uint64_t ns) {
    return ns / NS_PER_TICK;
}

uint64_t mtim_read_ticks(void) {
    uint32_t low;
    uint32_t high;
    uint32_t check;

    high  = *MTIM_TIME_HIGH_REG;
    low   = *MTIM_TIME_LOW_REG;
    check = *MTIM_TIME_HIGH_REG;
    if (high != check) {
        low = *MTIM_TIME_LOW_REG;
        high = check;
    }

    return ((uint64_t)high << 32U) | low;
}

uint64_t mtim_read_nanosec(void) {
    return ns_from_ticks(mtim_read_ticks());
}

void mtim_write_compare(const uint64_t value) {
    // Write max value into upper 32 bits to prevent spurious IRQ during write
    *MTIM_COMP_HIGH_REG  = 0xFFFFFFFFUL;
    *MTIM_COMP_LOW_REG   = (uint32_t)(value & 0xFFFFFFFFUL);
    *MTIM_COMP_HIGH_REG  = (uint32_t)(value >> 32U);
}


// Valid modes: "machine", "supervisor", or "user"
void mtim_isr(void) __attribute__((interrupt("machine")));

volatile uint32_t _mtime_isr_counter;

void mtim_isr(void) {
    // Increment the ISR counter
    _mtime_isr_counter++;

    // Clear the interrupt
    *MTIM_COMP_HIGH_REG = 0xFFFFFFFFUL;
    *MTIM_COMP_LOW_REG = 0xFFFFFFFFUL;
}

void mtim_delay_ns(const uint64_t ns) {
    const uint64_t start_ns = mtim_read_nanosec();
    const uint64_t end_ns = start_ns + ns;
    while (mtim_read_nanosec() < end_ns) {
        __asm__ volatile ("" ::: "memory");
    }
}

void mtim_delay_ns_irq(const uint64_t ns) {
    const uint64_t start_ns = mtim_read_nanosec();
    const uint64_t end_ns = start_ns + ns;
    const uint32_t isr_counter = _mtime_isr_counter;
    mtim_write_compare(ticks_from_ns(end_ns));

    while (_mtime_isr_counter == isr_counter) {
        __asm__ volatile ("" ::: "memory");
    }
}

void delay_loop(const uint32_t count) {
    const uint32_t kDelayLoopCycles = count * 100UL * 1000UL;
    for (uint32_t i = 0U; i < kDelayLoopCycles; i++) {
        __asm__ volatile ("" ::: "memory");
    }
}