.section .text
.global _start
_start:
    lui   x1,  0x00010       /* verify x1  = 0x00010000, RAM base */

    /* Seed word 0 = 0xdeadceef; byte @+12 = 0x80; byte @+13 = 0x80 (signed -128) */
    lui   x10, 0xDEADD
    addi  x10, x10, -273
    sw    x10, 0(x1)
    addi  x24, x0,  0x80
    sb    x24, 12(x1)
    addi  x27, x0,  -128
    sb    x27, 13(x1)
    nop
    nop
    nop
    nop

    /* Case 1: lw -> addi (no gap) */
    lw    x11, 0(x1)         /* verify x11 = 0xdeadceef */
    addi  x12, x11, 1        /* verify x12 = 0xdeadcef0 */

    /* Case 2: lw -> sw (store loaded value next instruction) */
    lw    x13, 0(x1)         /* verify x13 = 0xdeadceef */
    sw    x13, 4(x1)

    /* Case 3: lw -> beq taken */
    lw    x14, 4(x1)
    beq   x14, x10, beq_taken
    addi  x15, x0, 0
beq_taken:
    addi  x15, x0, 0x51      /* verify x15 = 0x00000051 */

    /* Case 4: lw -> bne taken (loaded != immediate) */
    lw    x16, 0(x1)
    addi  x17, x0, 1
    bne   x16, x17, bne_taken
    addi  x18, x0, -1
bne_taken:
    addi  x18, x0, 0x52      /* verify x18 = 0x00000052 */

    /* Case 5: lw -> jalr (loaded target address) */
    auipc x19, 0
    addi  x20, x19, 24       /* jr_tgt @ auipc+0x18 */
    sw    x20, 8(x1)
    lw    x21, 8(x1)
    jalr  x22, 0(x21)
    addi  x23, x0, -1
jr_tgt:
    addi  x23, x0, 0x53      /* verify x23 = 0x00000053 */

    /* Case 6: lbu -> addi (memory pre-seeded above) */
    lbu   x25, 12(x1)        /* verify x25 = 0x00000080 */
    addi  x26, x25, 1        /* verify x26 = 0x00000081 */

    /* Case 7: lb -> addi (sign-extended byte, memory pre-seeded above) */
    lb    x28, 13(x1)        /* verify x28 = 0xffffff80 */
    addi  x29, x28, 1        /* verify x29 = 0xffffff81 */

    /* Case 8: lw -> xori */
    lw    x30, 0(x1)
    xori  x31, x30, 0xFF     /* verify x31 = 0xdeadce10 */

    /* Final SRAM layout (see load_use.sram.expected):
     *   word 0 @ 0x10000 = 0xdeadceef
     *   word 1 @ 0x10004 = 0xdeadceef
     *   word 2 @ 0x10008 = 0x0000007c (jalr target)
     *   word 3 @ 0x1000c = 0x00008080
     */

    nop
    nop
    nop
    nop
