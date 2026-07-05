#include <stdint.h>
#include <stdbool.h>
#include "mem_map.h"
#include "assert.h"

#define SCRATCH_OFFSET 0xF00

uint32_t crc32_word(uint32_t word) __attribute__((noinline));

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
    volatile uint32_t *const result_register = (volatile uint32_t *)RAM_BASE + index;
    ASSERT(is_valid_ram_addr(result_register));
    *result_register = result;
}

int main(void) {
    store_u32(SCRATCH_OFFSET, 0x12345678);
    write_result(0, crc32_word(load_u32(SCRATCH_OFFSET)));

    store_u32(SCRATCH_OFFSET, 0x87654321);
    write_result(1, crc32_word(load_u32(SCRATCH_OFFSET)));

    store_u32(SCRATCH_OFFSET, 0x11223344);
    write_result(2, crc32_word(load_u32(SCRATCH_OFFSET)));

    store_u32(SCRATCH_OFFSET, 0x44332211);
    write_result(3, crc32_word(load_u32(SCRATCH_OFFSET)));

    infinite_loop();
    return 0;
}
