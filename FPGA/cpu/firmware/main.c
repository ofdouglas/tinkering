#include <stdint.h>
#include "mem_map.h"
#include "time.h"


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