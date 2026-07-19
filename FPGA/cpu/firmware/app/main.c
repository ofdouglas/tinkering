#include <stdint.h>
#include <stdbool.h>
#include "system/intrinsics.h"
#include "drivers/time.h"
#include "drivers/uart.h"
#include "drivers/gpio.h"

void hello_message(void) {
    const char* const kHelloMsg = "Hello!\n";
    const uint32_t kHelloMsgLen = 7U;
    uart_send_string_blocking(kHelloMsg, kHelloMsgLen);
}

void toggle_led1(void) {
    static bool led_on = true;
    gpio_set_led(1, led_on);
    led_on = !led_on;
}

int main(void) {
    uart_rx_init();

    csr_write_mie(MIE_MEI);
    global_irq_enable();
    
    // Pulse LED0 for 100 usec
    gpio_set_led(0, true);
    mtim_delay_ns(NS_PER_MICROSEC * 100U);
    // mtim_delay_ns_irq(NS_PER_MICROSEC * 100U);
    gpio_set_led(0, false);
    
    hello_message();

    // Echo back up to 16 UART bytes
    uint8_t data;
    for (int i = 0; i < 16; i++) {
        if (uart_receive_byte(&data)) {
            uart_send_byte_blocking(data);
        }
    }

    // Toggle LED1 at 1 Hz forever
    while (1) {
        toggle_led1();

        mtim_delay_ns(NS_PER_MILLISEC * 500U);
    }

    while (1) {
        ; // Should not get here
    }
    return 0;
}