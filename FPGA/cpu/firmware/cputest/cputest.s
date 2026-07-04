.section .text
.global _start
_start:
    addi  x1, x0, 0x03F
    addi  x2, x0, -64
    lui   x3, 0x12345
    lui   x4, 0xEDCBA
    xori  x5, x1, -820
    xori  x6, x2, 0x333
    ori   x7, x3, -1093
    ori   x8, x4, 0x222
    andi  x9, x1, 0x111
    andi x10, x2, -274

    /* Wait for all test results to reach registers */
    nop
    nop
    nop
    nop


