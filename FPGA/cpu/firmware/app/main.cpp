#include <stdint.h>
#include <stddef.h>

#include "system/intrinsics.h"
#include "drivers/time.h"
#include "drivers/uart.h"
#include "drivers/spi.h"
#include "drivers/mcp2515.h"
#include "drivers/gpio.h"
#include "system/debug.h"
#include "util/sw_timer.h"
#include "util/string_utils.h"

sw_timer_t timer_5s = {0, NS_PER_SEC * 5ULL};
sw_timer_t timer_500ms = {0, NS_PER_MILLISEC * 500ULL};
sw_timer_t timer_100ms = {0, NS_PER_MILLISEC * 100ULL};

uint32_t num_can_send_errors = 0;
uint32_t num_can_frames_received = 0;
uint32_t num_uart_bytes_received = 0;

void stats_report(void) {
    log_value("UART bytes received: ", num_uart_bytes_received);
    log_value("CAN frames received: ", num_can_frames_received);
    log_value("CAN send errors: ", num_can_send_errors);
}

void toggle_led3(void) {
    static bool led3_on = true;
    gpio_set_led(3U, led3_on);
    led3_on = !led3_on;
}

void task_100ms(void) {
    const uint32_t kUptimeCanId = 0x1A0;
    can_frame_t frame;
    uint64_t now_ns = mtim_read_nanosec();

    frame.can_id = kUptimeCanId;
    frame.is_extended_id = false;
    frame.dlc = 8U;
    my_memcpy(frame.data, (uint8_t*)&now_ns, 8U);

    if (!mcp2515_send(&frame)) {
        num_can_send_errors++;
    }
}

void task_500ms(void) {
    static bool led0_on = true;
    gpio_set_led(0U, led0_on);
    led0_on = !led0_on;
}

void task_5s(void) {
    const uint64_t now_ns = mtim_read_nanosec();
    const uint32_t uptime_s = now_ns / NS_PER_SEC;

    log_value("Uptime (s): ", uptime_s);
}

// typedef int (*)(size_t argc, char** argv) command_func_t;
typedef void (*command_func_t)(void);

typedef struct {
    const char* name;
    command_func_t func;
} command_t;

command_t stats_cmd = {"stats", stats_report};
command_t led_cmd = {"led", toggle_led3};

const size_t kNumCommands = 2U;
command_t* command_table[kNumCommands] = {&stats_cmd, &led_cmd};

void run_command_shell(void) {
    const size_t kBufferSize = 32U;
    static size_t buffer_index = 0U;
    static char buffer[kBufferSize] = {};
    bool ran_command = false;

    uint8_t data = 0;
    while ((buffer_index < kBufferSize) && uart_receive_byte(&data)) {
        num_uart_bytes_received++;
        buffer[buffer_index++] = data;

        if ((data == '\n') || (data == '\r')) {
            for (size_t i = 0; i < kNumCommands; i++) {
                if (my_strncmp(buffer, command_table[i]->name, (buffer_index - 1))) {
                    log_string("Running command: ", command_table[i]->name);
                    command_table[i]->func();
                    ran_command = true;
                    break;
                }
            }
            if (!ran_command) {
                send_string_nonblocking("ERROR: command not recognized\n");
            }
            buffer_index = 0;
            my_memfill((uint8_t*)buffer, kBufferSize, '\0');
        }
    }

    if (buffer_index >= kBufferSize) {
        send_string_nonblocking("ERROR: buffer filled\n");
        buffer_index = 0;
        my_memfill((uint8_t*)buffer, kBufferSize, '\0');
    }
}

void run_can_rx(void) {
    can_frame_t frame;
    while (mcp2515_receive(&frame)) {
        log_value("Received CAN frame: ", frame.can_id);
        num_can_frames_received++;
    }
}

int main(void) {
    mtim_delay_ns(NS_PER_SEC * 2ULL);

    uart_init();
    spi_init();
    if (!mcp2515_init_500kbps_8mhz()) {
        num_can_send_errors++;
        send_string_nonblocking("ERROR: MCP2515 initialization failed\n");
    }

    csr_write_mie(MIE_MEI | MIE_MTI);
    global_irq_enable();

    while (1) {
        run_command_shell();

        run_can_rx();

        if (!sw_timer_check_elapsed(&timer_100ms)) {
            continue;
        }
        task_100ms();

        if (!sw_timer_check_elapsed(&timer_500ms)) {
            continue;
        }
        task_500ms();

        if (!sw_timer_check_elapsed(&timer_5s)) {
            continue;
        }
        task_5s();
    }

    while (1) {
        ;  // Should not get here
    }
    return 0;
}