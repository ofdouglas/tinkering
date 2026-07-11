#include <stdint.h>
#include "mem_map.h"
#include "assert.h"

void write_result(uint32_t index, uint32_t value) __attribute__((noinline));

void write_result(uint32_t index, uint32_t value) {
    volatile uint32_t *const result = (volatile uint32_t *)RAM_BASE + index;
    *result = value;
}

int main(void) {
    write_result(0, 0xaf6d87d2);
    write_result(1, 0x87654321);
    infinite_loop();
    return 0;
}
