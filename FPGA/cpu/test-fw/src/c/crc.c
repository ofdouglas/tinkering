#include <stdint.h>
#include <stdbool.h>
#include "mem_map.h"
#include "assert.h"

#define NUM_TEST_WORDS 16


const uint32_t test_data[NUM_TEST_WORDS] = {
    0x12345678,
    0xfae23b1c,
    0x11223344,
    0x85a0b711,
    0x37c9e802,
    0x87654321,
    0x5d42ae99,
    0x44332211,
    0xdab48765,
    0x9f0b2c43,
    0xc59f1843,
    0x44332211,
    0xbeefcafe,
    0x0000abcd,
    0xfedcba98,
    0x13579bdf
};

volatile uint32_t test_results[NUM_TEST_WORDS] = {};

uint32_t crc32_word(uint32_t word) {
    uint32_t crc = 0xFFFFFFFF;

    for (int byte_index = 0; byte_index < 4; byte_index++) {
        uint32_t byte = word & 0xFFu;
        word >>= 8;
        crc ^= byte;
        for (int bit = 7; bit >= 0; bit--) {
            if (crc & 1u) {
                crc = (crc >> 1) ^ 0xEDB88320u;
            } else {
                crc >>= 1;
            }
        }
    }

    return ~crc;
}

bool is_valid_ram_addr(const volatile void *addr) {
    uintptr_t a = (uintptr_t)addr;
    return a >= RAM_BASE && a < RAM_END;
}

void store_u32(uint32_t offset, uint32_t value) __attribute__((noinline));
uint32_t load_u32(uint32_t offset) __attribute__((noinline));

void store_u32(uint32_t offset, uint32_t value) {
    volatile uint32_t *ptr = (volatile uint32_t *)((uintptr_t)RAM_BASE + offset);
    ASSERT(is_valid_ram_addr(ptr));
    *ptr = value;
}

uint32_t load_u32(uint32_t offset) {
    volatile uint32_t *ptr = (volatile uint32_t *)((uintptr_t)RAM_BASE + offset);
    ASSERT(is_valid_ram_addr(ptr));
    return *ptr;
}

void write_result(uint32_t index, uint32_t result) __attribute__((noinline));

void write_result(uint32_t index, uint32_t result) {
    test_results[index] = result;
}

int main(void) {

    for (int i = 0; i < NUM_TEST_WORDS; i++) {
        write_result(i, crc32_word(test_data[i]));
    }

    infinite_loop();
    return 0;
}
