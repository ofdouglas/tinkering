#ifndef ASSERT_H
#define ASSERT_H    

#include <stdint.h>

// x30 register signals OK / fault status
// x31 register contains line at which terminate was invoked

typedef enum {
    REG_STATUS_OK     = 0x12341234,
    REG_STATUS_ERROR  = 0xF00DF00D
} reg_status_e;


void infinite_loop() __attribute__((noreturn));
inline void infinite_loop() {
    while (1) {
        __asm__ volatile ("nop");
    }
}

void terminate_handler(uint32_t line_number) __attribute__((noreturn));
inline void terminate_handler(uint32_t line_number) {
    __asm__ volatile (
        "lui   x30, 0xF00DF\n\t"
        "addi  x30, x30, 0x00D\n\t"
        "add   x31, x0, %0"
        :
        : "r"(line_number)
        : "x30", "x31"
    );
    infinite_loop();
}

#define ASSERT(x) ((x) ? (void)0 : terminate_handler(__LINE__))
#endif