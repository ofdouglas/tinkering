.section .text
.global _start
_start:
    addi  x1,  x0, 0x03F     /* verify x1  = 0x0000003f */
    addi  x2,  x0, -64       /* verify x2  = 0xffffffc0 */
    lui   x3,  0x12345       /* verify x3  = 0x12345000 */
    lui   x4,  0xEDCBA       /* verify x4  = 0xedcba000 */
    xori  x5,  x1, -820      /* verify x5  = 0xfffffcf3 */
    xori  x6,  x2, 0x333     /* verify x6  = 0xfffffcf3 */
    ori   x7,  x3, -1093     /* verify x7  = 0xfffffbbb */
    ori   x8,  x4, 0x222     /* verify x8  = 0xedcba222 */
    andi  x9,  x1, 0x111     /* verify x9  = 0x00000011 */
    andi  x10, x2, -274      /* verify x10 = 0xfffffec0 */
    addi  x11, x1, 17        /* verify x11 = 0x00000050 */
    add   x12, x1, x2        /* verify x12 = 0xffffffff */
    sub   x13, x1, x2        /* verify x13 = 0x0000007f */
    slli  x14, x1, 2         /* verify x14 = 0x000000fc */
    srli  x15, x2, 2         /* verify x15 = 0x3ffffff0 */
    srai  x16, x2, 2         /* verify x16 = 0xfffffff0 */
    sll   x17, x1, x9        /* verify x17 = 0x007e0000 */
    srl   x18, x2, x1        /* verify x18 = 0x00000001 */
    sra   x19, x2, x1        /* verify x19 = 0xffffffff */
    slti  x20, x2, -1        /* verify x20 = 0x00000001 */
    slti  x21, x1, -1        /* verify x21 = 0x00000000 */
    sltiu x22, x2, -1        /* verify x22 = 0x00000001 */
    sltiu x23, x2, 0         /* verify x23 = 0x00000000 */
    slt   x24, x2, x1        /* verify x24 = 0x00000001 */
    slt   x25, x1, x2        /* verify x25 = 0x00000000 */
    sltu  x26, x1, x2        /* verify x26 = 0x00000001 */
    sltu  x27, x2, x1        /* verify x27 = 0x00000000 */
    xor   x28, x1, x8        /* verify x28 = 0xedcba21d */
    or    x29, x1, x8        /* verify x29 = 0xedcba23f */
    and   x30, x1, x8        /* verify x30 = 0x00000022 */
    sub   x31, x4, x3        /* verify x31 = 0xdb975000 */

    addi  x20, x0, 0         /* verify x20 = 0x0000003f */
    beq   x1, x1, branch_beq_taken
    addi  x20, x20, -1
branch_beq_taken:
    addi  x20, x20, 1
    nop
    nop
    nop
    bne   x1, x2, branch_bne_taken
    addi  x20, x20, -2
branch_bne_taken:
    addi  x20, x20, 2
    nop
    nop
    nop
    blt   x2, x1, branch_blt_taken
    addi  x20, x20, -4
branch_blt_taken:
    addi  x20, x20, 4
    nop
    nop
    nop
    bge   x1, x2, branch_bge_taken
    addi  x20, x20, -8
branch_bge_taken:
    addi  x20, x20, 8
    nop
    nop
    nop
    bltu  x1, x2, branch_bltu_taken
    addi  x20, x20, -16
branch_bltu_taken:
    addi  x20, x20, 16
    nop
    nop
    nop
    bgeu  x2, x1, branch_bgeu_taken
    addi  x20, x20, -32
branch_bgeu_taken:
    addi  x20, x20, 32
    nop
    nop
    nop

    addi  x21, x0, 0         /* verify x21 = 0x0000003f */
    beq   x1, x2, branch_beq_not_taken
    addi  x21, x21, 1
    nop
    nop
    nop
branch_beq_not_taken:
    bne   x1, x1, branch_bne_not_taken
    addi  x21, x21, 2
    nop
    nop
    nop
branch_bne_not_taken:
    blt   x1, x2, branch_blt_not_taken
    addi  x21, x21, 4
    nop
    nop
    nop
branch_blt_not_taken:
    bge   x2, x1, branch_bge_not_taken
    addi  x21, x21, 8
    nop
    nop
    nop
branch_bge_not_taken:
    bltu  x2, x1, branch_bltu_not_taken
    addi  x21, x21, 16
    nop
    nop
    nop
branch_bltu_not_taken:
    bgeu  x1, x2, branch_bgeu_not_taken
    addi  x21, x21, 32
    nop
    nop
    nop
branch_bgeu_not_taken:

    jal   x22, jump_jal_target /* verify x22 = JAL return address */
    addi  x23, x0, -1
jump_jal_target:
    nop
    addi  x23, x0, 0x23      /* verify x23 = 0x00000023 */
    nop
    nop
    nop
    nop
    addi  x26, x22, 60       /* verify x26 = JALR target address */
    nop
    nop
    nop
    nop
    jalr  x24, 0(x26)        /* verify x24 = JALR return address */
    addi  x25, x0, -1
    nop
jump_jalr_target:
    nop
    addi  x25, x0, 0x25      /* verify x25 = 0x00000025 */

    /* Wait for all test results to reach registers */
    nop
    nop
    nop
    nop


