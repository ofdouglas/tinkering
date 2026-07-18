#include <stdint.h>
#include <stdbool.h>
#include "drivers/time.h"
#include "drivers/uart.h"
#include "drivers/gpio.h"

void hello_message(void) {
    const char* const kHelloMsg = "Hello!\n";
    const uint32_t kHelloMsgLen = 7U;
    uart_send_string(kHelloMsg, kHelloMsgLen);
}

int main(void) {
    // Pulse LED0 for 100 usec
    gpio_set_led(0, true);
    // mtim_delay_ns(NS_PER_MICROSEC * 100U);
    mtim_delay_ns_irq(NS_PER_MICROSEC * 100U);
    gpio_set_led(0, false);
    
    // Toggle LED1 at 1 Hz forever
    bool led_on = false;
    while (1) {
        led_on = !led_on;
        gpio_set_led(1, led_on);

        if (led_on) {
            hello_message();
        }
        mtim_delay_ns(NS_PER_MILLISEC * 500U);
    }

    while (1) {
        ; // Should not get here
    }
    return 0;
}