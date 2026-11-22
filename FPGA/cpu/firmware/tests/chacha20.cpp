#include "drivers/time.h"
#include "support/test_uart.h"

static uint32_t rotate_left(uint32_t x, unsigned n) {
    return (x << n) | (x >> (32 - n));
}

static void quarter_round(uint32_t &a, uint32_t &b, uint32_t &c, uint32_t &d) {
    a += b;
    d = rotate_left(d ^ a, 16);
    c += d;
    b = rotate_left(b ^ c, 12);
    a += b;
    d = rotate_left(d ^ a, 8);
    c += d;
    b = rotate_left(b ^ c, 7);
}

// RFC 8439 section 2.3.2, with a variable block counter.
static void block(uint32_t counter, volatile uint8_t *output) {
    uint32_t initial[16] = {
        0x61707865, 0x3320646e, 0x79622d32, 0x6b206574,
        0x03020100, 0x07060504, 0x0b0a0908, 0x0f0e0d0c,
        0x13121110, 0x17161514, 0x1b1a1918, 0x1f1e1d1c,
        counter, 0x09000000, 0x4a000000, 0
    };
    uint32_t x[16];
    for (unsigned i = 0; i < 16; ++i) x[i] = initial[i];
    for (unsigned i = 0; i < 10; ++i) {
        for (unsigned j = 0; j < 4; ++j)
            quarter_round(x[j], x[4+j], x[8+j], x[12+j]);
        for (unsigned j = 0; j < 4; ++j)
            quarter_round(x[j], x[4+(j+1)%4], x[8+(j+2)%4], x[12+(j+3)%4]);
    }
    for (unsigned i = 0; i < 16; ++i) {
        uint32_t word = x[i] + initial[i];
        for (unsigned j = 0; j < 4; ++j)
            output[4*i + j] = word >> (8*j);
    }
}

int main() {
    uart_init();
    // Volatile inputs keep the RFC 8439 section 2.1.1 check on the CPU.
    static const volatile uint32_t input[] = {0x11111111, 0x01020304, 0x9b8d6f43, 0x01234567};
    uint32_t a = input[0], b = input[1], c = input[2], d = input[3];
    quarter_round(a, b, c, d);
    if (a != 0xea2a92f4 || b != 0xcb1cf8ce || c != 0x4581472e || d != 0x5881c4bb) {
        print_text("FAIL ChaCha20 quarter round\n");
        return 1;
    }

    static const uint8_t expected[64] = {
        0x10,0xf1,0xe7,0xe4,0xd1,0x3b,0x59,0x15,0x50,0x0f,0xdd,0x1f,0xa3,0x20,0x71,0xc4,
        0xc7,0xd1,0xf4,0xc7,0x33,0xc0,0x68,0x03,0x04,0x22,0xaa,0x9a,0xc3,0xd4,0x6c,0x4e,
        0xd2,0x82,0x64,0x46,0x07,0x9f,0xaa,0x09,0x14,0xc2,0xd7,0x05,0xd9,0x8b,0x02,0xa2,
        0xb5,0x12,0x9c,0xd1,0xde,0x16,0x4e,0xb9,0xcb,0xd0,0x83,0xe8,0xa2,0x50,0x3c,0x4e
    };
    volatile uint8_t output[64];
    block(1, output);
    for (unsigned i = 0; i < sizeof(expected); ++i) {
        if (output[i] != expected[i]) {
            print_text("FAIL ChaCha20 block byte ");
            print_number(i);
            print_text("\n");
            return 1;
        }
    }

    constexpr unsigned blocks = 256;
    uint64_t start = mtim_read_ticks();
    for (unsigned counter = 1; counter <= blocks; ++counter) block(counter, output);
    uint64_t elapsed = mtim_read_ticks() - start;
    print_text("ChaCha20 ticks/block: ");
    print_number(elapsed / blocks);
    print_text("\nChaCha20 PASS\n");
    return 0;
}
