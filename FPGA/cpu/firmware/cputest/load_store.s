.section .text
.global _start
_start:
    lui   x1,  0x00010       /* verify x1  = 0x00010000, RAM base pointer */
    addi  x2,  x0, -64       /* verify x2  = 0xffffffc0, byte test value */
    lui   x3,  0xDB975       /* verify x3  = 0xdb975000, word test value */
    addi  x4,  x0, -781      /* verify x4  = 0xfffffcf3, halfword test value */

    sw    x3,  0(x1)
    lw    x5,  0(x1)         /* verify x5  = 0xdb975000 */

    sb    x2,  4(x1)
    lbu   x6,  4(x1)         /* verify x6  = 0x000000c0 */
    lb    x7,  4(x1)         /* verify x7  = 0xffffffc0 */

    sh    x4,  8(x1)
    lhu   x8,  8(x1)         /* verify x8  = 0x0000fcf3 */
    lh    x9,  8(x1)         /* verify x9  = 0xfffffcf3 */

    sb    x4,  7(x1)
    lw    x10, 4(x1)         /* verify x10 = 0xf30000c0 */
    lbu   x11, 7(x1)         /* verify x11 = 0x000000f3 */
    lb    x12, 7(x1)         /* verify x12 = 0xfffffff3 */

    sh    x3,  10(x1)
    lhu   x13, 10(x1)        /* verify x13 = 0x00005000 */
    lh    x14, 10(x1)        /* verify x14 = 0x00005000 */
    lw    x15, 8(x1)         /* verify x15 = 0x5000fcf3 */

    /* --- Store-after-compute hazard coverage (EX→MEM forwarding) --- */

    lui   x16, 0x50000       /* case A: lui + addi + sw, same reg */
    addi  x16, x16, 0x123    /* verify x16 = 0x50000123 */
    sw    x16, 12(x1)

    lui   x17, 0x12345       /* case B: lui + sw, no addi (control) */
    sw    x17, 16(x1)        /* verify x17 = 0x12345000 */

    addi  x18, x17, 0x7FF    /* case C: addi(src=lui) + addi + sw */
    addi  x18, x18, -1194    /* verify x18 = 0x12345355 */
    sw    x18, 20(x1)
    sw    x18, 24(x1)        /* case D: back-to-back sw same reg (forward regression) */

    lui   x19, 0x87654       /* case E: lui + addi + sw, alt constant */
    addi  x19, x19, 0x321    /* verify x19 = 0x87654321 */
    sw    x19, 28(x1)

    lui   x20, 0x40000       /* case F: lui + negative addi + sw */
    addi  x20, x20, -1       /* verify x20 = 0x3fffffff */
    sw    x20, 32(x1)

    addi  x21, x0, 2047      /* case G: addi + addi + sw, no lui */
    addi  x21, x21, 1        /* verify x21 = 0x00000800 */
    sw    x21, 36(x1)

    lui   x22, 0xA0000       /* case H: lui + sw, upper-only pattern */
    sw    x22, 40(x1)        /* verify x22 = 0xa0000000 */

    addi  x23, x17, 0        /* case I: addi +0 copy, then sw (fwd from x17) */
    sw    x23, 44(x1)        /* verify x23 = 0x12345000 */

    lui   x24, 0xDEADD       /* case J: lui + addi with lo12 >= 2048 (via negative imm) */
    addi  x24, x24, -273     /* verify x24 = 0xdeadceef */
    sw    x24, 48(x1)

    lui   x27, 0xAF6D8       /* case K: CRC constant materialization (failed in C) */
    addi  x27, x27, 0x7D2    /* verify x27 = 0xaf6d87d2 */
    sw    x27, 52(x1)

    addi  x28, x0, 0x7D2     /* case L: single addi + sw, no lui */
    sw    x28, 56(x1)        /* verify x28 = 0x000007d2 */

    addi  x29, x17, 0x7FF    /* case M: addi + sw, no second addi (partial constant) */
    sw    x29, 60(x1)        /* verify x29 = 0x123457ff; catches addi->sw EX forward miss */

    addi  x30, x29, -1194    /* case N: addi + sw immediately after prior sw target reg */
    sw    x30, 64(x1)        /* verify x30 = 0x12345355; addi reads x29 same cycle as prior store */

    /* --- Store then load same address (memory visibility) --- */

    lui   x6,  0xA5A50       /* case O: sw then lw same word */
    addi  x6,  x6,  0x5A5    /* verify x6  = 0xa5a505a5 */
    sw    x6,  68(x1)
    lw    x7,  68(x1)        /* verify x7  = 0xa5a505a5 */

    lui   x6,  0x11111       /* case P: sb into word then lw full word */
    addi  x6,  x6,  0x11     /* verify x6  = 0x11111011 */
    sw    x6,  72(x1)
    addi  x7,  x0,  0x99
    sb    x7,  72(x1)
    lw    x8,  72(x1)        /* verify x8  = 0x11111099 */

    lui   x6,  0x22222       /* case Q: sh into word then lw full word */
    addi  x6,  x6,  0x22     /* verify x6  = 0x22222022 */
    sw    x6,  76(x1)
    addi  x7,  x0,  0xCD
    sh    x7,  78(x1)
    lw    x9,  76(x1)        /* verify x9  = 0x00cd2022 */

    /* Final SRAM layout (see load_store.sram.expected):
     *   word  0 @ 0x10000 = 0xdb975000
     *   word  1 @ 0x10004 = 0xf30000c0
     *   word  2 @ 0x10008 = 0x5000fcf3
     *   word  3 @ 0x1000c = 0x50000123
     *   word  4 @ 0x10010 = 0x12345000
     *   word  5 @ 0x10014 = 0x12345355
     *   word  6 @ 0x10018 = 0x12345355
     *   word  7 @ 0x1001c = 0x87654321
     *   word  8 @ 0x10020 = 0x3fffffff
     *   word  9 @ 0x10024 = 0x00000800
     *   word 10 @ 0x10028 = 0xa0000000
     *   word 11 @ 0x1002c = 0x12345000
     *   word 12 @ 0x10030 = 0xdeadceef
     *   word 13 @ 0x10034 = 0xaf6d87d2
     *   word 14 @ 0x10038 = 0x000007d2
     *   word 15 @ 0x1003c = 0x123457ff
     *   word 16 @ 0x10040 = 0x12345355
     *   word 17 @ 0x10044 = 0xa5a505a5
     *   word 18 @ 0x10048 = 0x11111099
     *   word 19 @ 0x1004c = 0x00cd2022
     */
    /* Wait for all test results to reach registers */
    nop
    nop
    nop
    nop
