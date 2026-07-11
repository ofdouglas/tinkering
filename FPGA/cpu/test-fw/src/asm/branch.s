.section .text
.global _start
_start:
    addi  x1,  x0,  5         /* verify x1  = 5 */
    addi  x2,  x0,  5         /* verify x2  = 5 */
    addi  x3,  x0,  -3        /* verify x3  = -3 */
    addi  x4,  x0,  7         /* verify x4  = 7 */
    addi  x5,  x0,  16        /* verify x5  = 16 */

    /* --- Basic taken branches --- */
    beq   x1,  x2,  beq_taken_ok
    addi  x5,  x0,  -1
beq_taken_ok:
    addi  x6,  x0,  17        /* verify x6  = 17 */
    bne   x1,  x3,  bne_taken_ok
    addi  x6,  x0,  -1
bne_taken_ok:
    addi  x7,  x0,  18        /* verify x7  = 18 */
    blt   x3,  x1,  blt_taken_ok
    addi  x7,  x0,  -1
blt_taken_ok:
    addi  x8,  x0,  19        /* verify x8  = 19 */
    bge   x4,  x1,  bge_taken_ok
    addi  x8,  x0,  -1
bge_taken_ok:
    addi  x9,  x0,  20        /* verify x9  = 20 */
    bltu  x1,  x3,  bltu_taken_ok
    addi  x9,  x0,  -1
bltu_taken_ok:
    addi  x10, x0,  21        /* verify x10 = 21 */
    bgeu  x3,  x4,  bgeu_taken_ok
    addi  x10, x0,  -1
bgeu_taken_ok:
    addi  x11, x0,  32        /* verify x11 = 32 */

    /* --- Not-taken branches --- */
    beq   x1,  x4,  1f
    addi  x11, x0,  33        /* verify x11 = 33 */
1:
    addi  x12, x0,  32
    bne   x1,  x2,  2f
    addi  x12, x0,  34        /* verify x12 = 34 */
2:
    addi  x13, x0,  32
    blt   x4,  x1,  3f
    addi  x13, x0,  35        /* verify x13 = 35 */
3:
    addi  x14, x0,  32
    bge   x3,  x1,  4f
    addi  x14, x0,  36        /* verify x14 = 36 */
4:
    addi  x15, x0,  32
    bltu  x3,  x1,  5f
    addi  x15, x0,  37        /* verify x15 = 37 */
5:
    addi  x16, x0,  32
    bgeu  x1,  x3,  6f
    addi  x16, x0,  38        /* verify x16 = 38 */
6:
    addi  x17, x0,  3         /* verify x17 = 3 */
    addi  x18, x0,  0         /* verify x18 = 0 */
    addi  x19, x0,  0         /* loop accumulator; do not rely on reset value */

branch_loop:
    addi  x18, x18, 1         /* verify x18 = 3 */
    addi  x17, x17, -1
    addi  x19, x19, 2         /* verify x19 = 6 */
    bne   x17, x0,  branch_loop

    /* --- Back-to-back taken branches --- */
    addi  x20, x0,  160       /* verify x20 = 0xa0 */
    beq   x1,  x2,  btb_taken_b2
    addi  x20, x0,  -1
    beq   x0,  x0,  btb_taken_end
btb_taken_b2:
    beq   x1,  x2,  btb_taken_end
    addi  x20, x0,  -2
btb_taken_end:
    addi  x21, x0,  176       /* verify x21 = 0xb0 */

    /* --- Back-to-back not-taken (miss) branches --- */
    beq   x1,  x4,  btb_miss_fail_first
    beq   x1,  x4,  btb_miss_fail_second
    beq   x0,  x0,  btb_miss_end
btb_miss_fail_first:
    addi  x21, x0,  -1
    beq   x0,  x0,  btb_miss_end
btb_miss_fail_second:
    addi  x21, x0,  -2
btb_miss_end:
    addi  x22, x0,  192       /* verify x22 = 0xc0 */

    /* --- Taken then not-taken --- */
    beq   x1,  x2,  btb_tn_b2
    addi  x22, x0,  -1
    beq   x0,  x0,  btb_tn_end
btb_tn_b2:
    beq   x1,  x4,  btb_tn_fail_second
    beq   x0,  x0,  btb_tn_end
btb_tn_fail_second:
    addi  x22, x0,  -2
btb_tn_end:
    addi  x23, x0,  208       /* verify x23 = 0xd0 */

    /* --- Not-taken then taken --- */
    beq   x1,  x4,  btb_nt_fail_first
    beq   x1,  x2,  btb_nt_end
    addi  x23, x0,  -2
    beq   x0,  x0,  btb_nt_end
btb_nt_fail_first:
    addi  x23, x0,  -1
btb_nt_end:

    /* --- Forwarding into taken branch --- */
    addi  x24, x1,  0         /* verify x24 = 5 */
    beq   x24, x2,  fwd_taken_end
    addi  x24, x0,  -1
fwd_taken_end:
    addi  x25, x4,  0         /* verify x25 = 7 */
    bne   x25, x2,  fwd_miss_end
    addi  x25, x0,  -1
fwd_miss_end:
    addi  x28, x0,  40        /* verify x28 = 40 */
    addi  x26, x1,  0
    addi  x27, x2,  0
    beq   x26, x27, fwd_dual_end
    addi  x28, x0,  -1
fwd_dual_end:

    /* --- Mixed taken / not-taken chain --- */
    addi  x29, x0,  656       /* verify x29 = 0x290 */
    bne   x1,  x3,  mix_b2
    addi  x29, x0,  -1
    beq   x0,  x0,  mix_end
mix_b2:
    blt   x3,  x1,  mix_end
    addi  x29, x0,  -2
mix_end:

    /* --- Forwarding with back-to-back branches --- */
    addi  x30, x0,  48        /* verify x30 = 0x30 */
    addi  x26, x1,  0
    addi  x17, x17, 0
    beq   x26, x2,  fwd_n2_taken_end
    addi  x30, x0,  -1
fwd_n2_taken_end:
    addi  x31, x0,  49        /* verify x31 = 0x31 */
    addi  x26, x4,  0
    addi  x17, x17, 0
    bne   x26, x2,  fwd_n2_miss_end
    addi  x31, x0,  -1
fwd_n2_miss_end:

    nop
    nop
    nop
    nop
