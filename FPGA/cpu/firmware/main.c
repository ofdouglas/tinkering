#include <stdint.h>
#include "mem_map.h"

#define NS_PER_MILLISEC (1000UL * 1000UL)
#define NS_PER_SEC     (1000UL * NS_PER_MILLISEC)
#define MTIM_FREQ_HZ (50UL * 1000UL * 1000UL)
#define NS_PER_TICK  (NS_PER_SEC / MTIM_FREQ_HZ)

#define MTIM_TIME_LOW_REG  ((volatile uint32_t*)(MTIM_BASE + 0))
#define MTIM_TIME_HIGH_REG ((volatile uint32_t*)(MTIM_BASE + 4))
#define MTIM_COMP_LOW_REG  ((volatile uint32_t*)(MTIM_BASE + 8))
#define MTIM_COMP_HIGH_REG ((volatile uint32_t*)(MTIM_BASE + 12))

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
    return mtim_read_ticks() * NS_PER_TICK;
}

void mtim_write_compare(const uint64_t value) {
    // Write max value into upper 32 bits to prevent spurious IRQ during write
    *MTIM_COMP_HIGH_REG  = 0xFFFFFFFFUL;
    *MTIM_COMP_LOW_REG   = (uint32_t)(value & 0xFFFFFFFFUL);
    *MTIM_COMP_HIGH_REG  = (uint32_t)(value >> 32);
}

void mtim_delay_ns(const uint64_t ns) {
    const uint64_t start = mtim_read_nanosec();
    const uint64_t end = start + ns;
    while (mtim_read_nanosec() < end) {
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


    uint32_t led_value = 0U;
    
    while (1) {
        led_value ^= 1U;
        *LedRegister = led_value;

        if (led_value != 0U) {
            uart_send_string(kHelloMsg, kHelloMsgLen);
        }
        // delay_loop(50U);
        mtim_delay_ns(NS_PER_MILLISEC * 500U);
    }

    while (1) {
        ; // Should not get here
    }
    return 0;
}