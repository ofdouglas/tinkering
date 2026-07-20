#include "system/debug.h"

#define LOG_BUFFER_SIZE_B 32

uint8_t log_buffer[LOG_BUFFER_SIZE_B] __attribute__((section(".log_buffer")));

void checkpoint(uint32_t index) {
    if (index >= LOG_BUFFER_SIZE_B) {
        return;
    }

    log_buffer[index] = 1;
}
