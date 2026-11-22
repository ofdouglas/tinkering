#include <stdint.h>
#include "drivers/uart.h"

template <typename T>
struct ArithmeticCase {
    T lhs, rhs, product, quotient, remainder;
};

static void print(const char *s) {
    while (*s) uart_send_byte_blocking(*s++);
}

static void print_hex(uint64_t value) {
    static const char digits[] = "0123456789abcdef";
    for (int shift = 60; shift >= 0; shift -= 4)
        uart_send_byte_blocking(digits[(value >> shift) & 15]);
}

template <typename T>
static bool check(const char *type, char op, T lhs, T rhs, T actual, T expected) {
    if (actual == expected) return true;
    print("FAIL ");
    print(type);
    print(" ");
    print_hex(lhs);
    uart_send_byte_blocking(op);
    print_hex(rhs);
    print(" got ");
    print_hex(actual);
    print(" expected ");
    print_hex(expected);
    print("\n");
    return false;
}

template <typename T, unsigned N>
static bool run(const char *type, const ArithmeticCase<T> (&cases)[N]) {
    for (const auto &c : cases) {
        // Force runtime operands even if the compiler unrolls the whole table.
        volatile T input_a = c.lhs, input_b = c.rhs;
        T a = input_a, b = input_b;
        if (!check(type, '*', a, b, T(a * b), c.product) ||
            !check(type, '/', a, b, T(a / b), c.quotient) ||
            !check(type, '%', a, b, T(a % b), c.remainder))
            return false;
    }
    return true;
}

// Expected values calculated with host integer arithmetic. Signed products fit
// their type and signed quotients truncate toward zero.
static constexpr ArithmeticCase<int32_t> i32[] = {
    {-101, 7, -707, -14, -3},
    {101, -7, -707, -14, 3},
    {-101, -7, 707, 14, -3},
    {-1, -1, 1, 1, 0},
    {0, -13, 0, 0, 0},
    {INT32_MIN, 1, INT32_MIN, INT32_MIN, 0},
    {65535, 32767, 2147385345, 2, 1},
};

static constexpr ArithmeticCase<uint32_t> u32[] = {
    {0x0U, 0x9U, 0x0U, 0x0U, 0x0U},
    {0xffffffffU, 0x1U, 0xffffffffU, 0xffffffffU, 0x0U},
    {0xfffffffeU, 0x7U, 0xfffffff2U, 0x24924924U, 0x2U},
    {0xffffffffU, 0xffffffffU, 0x1U, 0x1U, 0x0U},
    {0x7U, 0xffffffffU, 0xfffffff9U, 0x0U, 0x7U},
    {0x80000000U, 0x2U, 0x0U, 0x40000000U, 0x0U},
};

static constexpr ArithmeticCase<int64_t> i64[] = {
    {-101LL, 7LL, -707LL, -14LL, -3LL},
    {101LL, -7LL, -707LL, -14LL, 3LL},
    {-101LL, -7LL, 707LL, 14LL, -3LL},
    {-1LL, -1LL, 1LL, 1LL, 0LL},
    {0LL, -13LL, 0LL, 0LL, 0LL},
    {INT64_MIN, 1LL, INT64_MIN, INT64_MIN, 0LL},
    {4294967295LL, 2147483647LL, 9223372030412324865LL, 2LL, 1LL},
    {-4294967301LL, 17LL, -73014444117LL, -252645135LL, -6LL},
    {81LL, -4294967315LL, -347892352515LL, 0LL, 81LL},
};

static constexpr ArithmeticCase<uint64_t> u64[] = {
    {0x0ULL, 0x9ULL, 0x0ULL, 0x0ULL, 0x0ULL},
    {0xffffffffffffffffULL, 0x1ULL, 0xffffffffffffffffULL, 0xffffffffffffffffULL, 0x0ULL},
    {0xfffffffffffffffeULL, 0x7ULL, 0xfffffffffffffff2ULL, 0x2492492492492492ULL, 0x0ULL},
    {0xffffffffffffffffULL, 0xffffffffffffffffULL, 0x1ULL, 0x1ULL, 0x0ULL},
    {0x7ULL, 0xffffffffffffffffULL, 0xfffffffffffffff9ULL, 0x0ULL, 0x7ULL},
    {0x8000000000000000ULL, 0x2ULL, 0x0ULL, 0x4000000000000000ULL, 0x0ULL},
    {0xfedcba9876543210ULL, 0x123456789ULL, 0xcccccccce1833a90ULL, 0xe0000000ULL, 0x96543210ULL},
    {0xffffffffULL, 0xffffffffULL, 0xfffffffe00000001ULL, 0x1ULL, 0x0ULL},
};

int main() {
    uart_init();
    if (run("i32", i32) && run("u32", u32) &&
        run("i64", i64) && run("u64", u64))
        print("Arithmetic helpers PASS\n");
    return 0;
}
