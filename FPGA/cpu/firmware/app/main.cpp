#include <stdint.h>

#include "system/intrinsics.h"
#include "drivers/time.h"
#include "drivers/uart.h"
#include "drivers/gpio.h"
#include "system/debug.h"


void hello_message(void) {
    const char* const kHelloMsg = "Hello!\n";
    const uint32_t kHelloMsgLen = 7U;
    uart_send_string_blocking(kHelloMsg, kHelloMsgLen);
}

void toggle_led1_1hz(void) {
    // 1 Hz * 50% duty cycle = 500 ms toggle period
    constexpr uint64_t kTogglePeriodNs = NS_PER_MILLISEC * 500U;
    static uint64_t next_toggle_time_ns = 0;
    static bool led_on = true;

    uint64_t current_time_ns = mtim_read_nanosec();
    if (current_time_ns >= next_toggle_time_ns) {
        next_toggle_time_ns = current_time_ns + kTogglePeriodNs;

        gpio_set_led(1, led_on);
        led_on = !led_on;
        checkpoint(5);
    }
}

class ScopedLedToggle {
public:
    ScopedLedToggle(int led_num) : led_num_(led_num) {
        gpio_set_led(led_num_, true);
    }
    ~ScopedLedToggle(void) {
        gpio_set_led(led_num_, false);
    }
private:
    int led_num_;
};

class Foo {
public:
    Foo(int x, int offset) : x_(x) {
        // const volatile uint32_t *data = (const volatile uint32_t *)0x10000000;
        // y_ = data[offset];
        volatile int scramble = 0x12345678;
        y_ = offset ^ scramble;
    }
    int getX(void) const { return x_; }
    void setX(int x) { x_ = x; }

    int getY(void) const { return y_; }
    void setY(int y) { y_ = y; }

private:
    int x_;
    int y_;    
};    

Foo foo_obj(10, 0);

int main(void) {
    checkpoint(0);
    uart_init();

    csr_write_mie(MIE_MEI | MIE_MTI);
    global_irq_enable();
    
    checkpoint(1);
    // Foo foo_obj(10, 0);
    volatile int x = foo_obj.getX();
    foo_obj.setX(x + 1);
    volatile int y = foo_obj.getY();
    foo_obj.setY(y + 1);

    checkpoint(2);
    {
        ScopedLedToggle led0(0);
        mtim_delay_ns_irq(NS_PER_MICROSEC * 100U);
    }

    checkpoint(3);
    hello_message();

    // Toggle LED1 at 1 Hz forever
    checkpoint(4);
    while (1) {
        // checkpoint(5) inside toggle_led1_1hz()
        toggle_led1_1hz();

        // Echo up to 1 UART byte
        checkpoint(6);
        uint8_t data;
        if (uart_receive_byte(&data)) {
            if (uart_send_byte_nonblocking(data)) {
                checkpoint(7);
            }
        }
    }

    while (1) {
        ; // Should not get here
    }
    return 0;
}