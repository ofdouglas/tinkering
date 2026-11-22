#include "support/test_uart.h"

static uint32_t next(uint32_t x) {
    x ^= x << 13;
    x ^= x >> 17;
    return x ^ (x << 5);
}

static volatile uint32_t scratch[256];
// GCC alias type permits halfword access to the storage of a uint32_t.
using Halfword [[gnu::may_alias]] = uint16_t;

int main() {
    uart_init();
    static const volatile uint32_t seed = 0x12345678;
    uint32_t state = seed;
    static const uint32_t first[] = {0x87985aa5, 0x155b24a3, 0x4820f4c4, 0x81b3ac98};
    for (uint32_t expected : first) {
        state = next(state);
        if (state != expected) {
            print_text("FAIL xorshift32 initial sequence\n");
            return 1;
        }
    }

    state = seed;
    for (auto &word : scratch) word = 0;
    for (unsigned i = 0; i < 4096; ++i) {
        state = next(state);
        unsigned offset = state & (sizeof(scratch) - 1);
        volatile uint32_t *word = &scratch[offset / 4];
        state = next(state);
        switch (i % 3) {
        case 0:
            reinterpret_cast<volatile uint8_t *>(word)[offset % 4] = state;
            break;
        case 1:
            reinterpret_cast<volatile Halfword *>(word)[(offset % 4) / 2] = state;
            break;
        case 2:
            *word = state;
            break;
        }
    }
    if (state != 0xa233b366) {
        print_text("FAIL xorshift32 final state\n");
        return 1;
    }

    // Host byte-array model writes each truncated value little-endian at the
    // naturally aligned offset. zlib.crc32 of its final 1024 bytes is below.
    uint32_t crc = 0xffffffff;
    for (auto &word : scratch) {
        auto bytes = reinterpret_cast<volatile uint8_t *>(&word);
        for (unsigned i = 0; i < sizeof(word); ++i) {
            crc ^= bytes[i];
            for (unsigned bit = 0; bit < 8; ++bit)
                crc = (crc >> 1) ^ ((crc & 1) ? 0xedb88320U : 0U);
        }
    }
    if (~crc != 0x8bade997U) {
        print_text("FAIL xorshift32 SRAM CRC32 got ");
        print_number(~crc);
        print_text("\n");
        return 1;
    }
    print_text("xorshift32 SRAM PASS\n");
    return 0;
}
