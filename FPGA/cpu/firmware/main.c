#include <stdint.h>
#include "mem_map.h"

#define NS_PER_MICROSEC (1000UL)
#define NS_PER_MILLISEC (NS_PER_MICROSEC * 1000UL)
#define NS_PER_SEC      (1000UL * NS_PER_MILLISEC)
#define MTIM_FREQ_HZ    (50UL * 1000UL * 1000UL)
#define NS_PER_TICK     (NS_PER_SEC / MTIM_FREQ_HZ)

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

    return ((uint64_t)high << 32) | low;
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

void mtim_delay_ns(const uint64_t ns) {
    const uint64_t start_ns = mtim_read_nanosec();
    const uint64_t end_ns = start_ns + ns;
    while (mtim_read_nanosec() < end_ns) {
        __asm__ volatile ("" ::: "memory");
    }
}

// In start.S: _mtime_isr_counter is incremented by 1 when a MTI IRQ is taken
extern volatile uint32_t _mtime_isr_counter;


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

void uart_send_string(const char* str, const uint32_t length) {
    volatile uint32_t* const UartStatusReg = (volatile uint32_t*)UART_BASE;
    volatile uint32_t* const UartTxDataReg = (volatile uint32_t*)(UART_BASE + 4U);
    volatile uint32_t* const UartRxDataReg = (volatile uint32_t*)(UART_BASE + 8U);

    for (uint32_t i = 0U; i < length; i++) {
        for (uint32_t j = 0U; j < 10000U; j++) { // wait for TX ready (bit 0)
            if (*UartStatusReg & 0x01U) {
                break;
            }
        }
        *UartTxDataReg = (uint32_t)str[i];
    }
}

int main(void) {

    volatile uint32_t* const LedRegister = (volatile uint32_t*)LED_BASE;

    const char* const kHelloMsg = "Hello!\n";
    const uint32_t kHelloMsgLen = 7U;
    uint32_t led_value = 0;

    // Pulse LED0 for 100 usec
    *LedRegister = 1U;
    // mtim_delay_ns(NS_PER_MICROSEC * 100U);
    mtim_delay_ns_irq(NS_PER_MICROSEC * 100U);
    *LedRegister = 0U;
    
    // Toggle LED1 at 1 Hz forever
    while (1) {
        led_value ^= 2U;
        *LedRegister = led_value;

        if (led_value != 0U) {
            uart_send_string(kHelloMsg, kHelloMsgLen);
        }
        mtim_delay_ns(NS_PER_MILLISEC * 500U);
    }

    while (1) {
        ; // Should not get here
    }
    return 0;
}