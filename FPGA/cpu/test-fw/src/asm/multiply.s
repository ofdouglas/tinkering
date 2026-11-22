.section .text
.global _start
_start:
    addi  x1, x0, 0          /* verify x1  = 0x00000000 */
    addi  x2, x0, 1          /* verify x2  = 0x00000001 */
    addi  x3, x0, 7          /* verify x3  = 0x00000007 */
    addi  x4, x0, -1         /* verify x4  = 0xffffffff */
    addi  x5, x0, -2         /* verify x5  = 0xfffffffe */
    lui   x6, 0x80000        /* verify x6  = 0x80000000 */
    lui   x7, 0x80000
    addi  x7, x7, -1         /* verify x7  = 0x7fffffff */
    lui   x8, 0x12345
    addi  x8, x8, 0x678      /* verify x8  = 0x12345678 */

    /* Low product: zero, signs, overflow, and signed boundary values. */
    mul   x9,  x1, x4        /* verify x9  = 0x00000000 */
    mul   x10, x2, x3        /* verify x10 = 0x00000007 */
    mul   x31, x10, x3       /* verify x31 = 0x00000031; zero-gap rd -> rs1 */
    mul   x11, x3, x4        /* verify x11 = 0xfffffff9 */
    mul   x12, x5, x4        /* verify x12 = 0x00000002 */
    mul   x13, x6, x4        /* verify x13 = 0x80000000 */
    mul   x14, x7, x7        /* verify x14 = 0x00000001 */

    /* High signed product. */
    mulh  x15, x2, x3        /* verify x15 = 0x00000000 */
    mulh  x16, x3, x4        /* verify x16 = 0xffffffff */
    mulh  x17, x5, x4        /* verify x17 = 0x00000000 */
    mulh  x18, x6, x4        /* verify x18 = 0x00000000 */
    mulh  x19, x6, x6        /* verify x19 = 0x40000000 */
    mulh  x20, x7, x7        /* verify x20 = 0x3fffffff */

    /* High signed-by-unsigned product. */
    mulhsu x21, x4, x3       /* verify x21 = 0xffffffff */
    mulhsu x22, x5, x4       /* verify x22 = 0xfffffffe */
    mulhsu x23, x6, x4       /* verify x23 = 0x80000000 */
    mulhsu x24, x7, x4       /* verify x24 = 0x7ffffffe */
    mulhsu x25, x8, x4       /* verify x25 = 0x12345677 */

    /* High unsigned product. */
    mulhu x26, x4, x4        /* verify x26 = 0xfffffffe */
    mulhu x27, x6, x3        /* verify x27 = 0x00000003 */
    mulhu x28, x7, x4        /* verify x28 = 0x7ffffffe */
    mulhu x29, x8, x8        /* verify x29 = 0x014b66dc */
    mulhu x30, x2, x4        /* verify x30 = 0x00000000 */

    /* A multiply write to x0 must be discarded. */
    mul   x0, x8, x8

    nop
    nop
    nop
    nop

loop:
    beq   x0, x0, loop
