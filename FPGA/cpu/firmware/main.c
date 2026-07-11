#include <stdint.h>
#include "mem_map.h"


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
        delay_loop(100U);
    }

    while (1) {
        ; // Should not get here
    }
    return 0;
}