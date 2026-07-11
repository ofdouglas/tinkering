.section .text
.global _start
_start:
    lui   x1,  0x80000       /* verify x1  = 0x80000000 */
    addi  x1,  x1,  -1       /* verify x1  = 0x7fffffff */
    lui   x2,  0x80000       /* verify x2  = 0x80000000 */
    addi  x3,  x0,  -1       /* verify x3  = 0xffffffff */
    addi  x4,  x0,  0        /* verify x4  = 0 */

    addi  x5,  x1,  1        /* verify x5  = 0x80000000 */
    add   x6,  x2,  x3       /* verify x6  = 0x7fffffff */
    sub   x7,  x4,  x1       /* verify x7  = 0x80000001 */
    add   x8,  x1,  x1       /* verify x8  = 0xfffffffe */
    addi  x9,  x1,  -1       /* verify x9  = 0x7ffffffe */
    addi  x10, x2, -1        /* verify x10 = 0x7fffffff */
    addi  x11, x0,  2047     /* verify x11 = 0x000007ff */
    addi  x12, x0,  -2048    /* verify x12 = 0xfffff800 */
    xori  x13, x4,  -1       /* verify x13 = 0xffffffff */
    ori   x14, x4,  -1       /* verify x14 = 0xffffffff */
    andi  x15, x3,  2047     /* verify x15 = 0x000007ff */
    andi  x16, x3,  -2048    /* verify x16 = 0xfffff800 */

    slli  x17, x1,  0        /* verify x17 = 0x7fffffff */
    srli  x18, x1,  0        /* verify x18 = 0x7fffffff */
    srai  x19, x2,  0        /* verify x19 = 0x80000000 */
    srl   x21, x3,  x4       /* verify x21 = 0xffffffff */
    sra   x22, x3,  x4       /* verify x22 = 0xffffffff */

    addi  x26, x0,  31
    sll   x20, x3,  x26      /* verify x20 = 0x80000000 */
    sll   x23, x4,  x26      /* verify x23 = 0x00000000 */
    srl   x24, x1,  x26      /* verify x24 = 0x00000001 */
    sra   x25, x2,  x26      /* verify x25 = 0xffffffff */
    slli  x27, x1,  31       /* verify x27 = 0x80000000 */
    srli  x28, x2,  31       /* verify x28 = 0x00000001 */
    srai  x29, x2,  31       /* verify x29 = 0xffffffff */

    slti  x30, x2,  -1       /* verify x30 = 0x00000001 */
    sltiu x31, x3,  -1       /* verify x31 = 0x00000000 */

    nop
    nop
    nop
    nop
