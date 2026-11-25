#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// External Interrupts (Starting from 16)
// This is the numbering used in the MIE/MIP registers.
typedef enum {
    EXT_IRQ_UART0 = 16,
    EXT_IRQ_RESERVED1 = 17,
    EXT_IRQ_RESERVED2 = 18,
    EXT_IRQ_RESERVED3 = 19,
    EXT_IRQ_RESERVED4 = 20,
    EXT_IRQ_RESERVED5 = 21,
    EXT_IRQ_RESERVED6 = 22,
    EXT_IRQ_RESERVED7 = 23,
    EXT_IRQ_RESERVED8 = 24,
    EXT_IRQ_RESERVED9 = 25,
    EXT_IRQ_RESERVED10 = 26,
    EXT_IRQ_RESERVED11 = 27,
    EXT_IRQ_RESERVED12 = 28,
    EXT_IRQ_RESERVED13 = 29,
    EXT_IRQ_RESERVED14 = 30,
    EXT_IRQ_RESERVED15 = 31,
} external_irq_e;
