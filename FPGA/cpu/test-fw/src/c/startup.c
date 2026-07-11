#include <stdint.h>
#include <stdbool.h>

#include "mem_map.h"
#include "assert.h"


volatile uint32_t results[8] = {};

volatile uint32_t data[4] = {0x12345678, 0x87654321, 0x11223344, 0x44332211};

void test_data(void) {
    results[0] = (data[0] == 0x12345678) ? 1 : 0;
    results[1] = (data[1] == 0x87654321) ? 1 : 0;
    results[2] = (data[2] == 0x11223344) ? 1 : 0;
    results[3] = (data[3] == 0x44332211) ? 1 : 0;

    for (int i = 0; i < 4; i++) {
        results[i + 4] = data[i];
    }
}

int main(void) {
    test_data();

    infinite_loop();
    return 0;
}