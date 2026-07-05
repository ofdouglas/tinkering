.section .text
.global _start
_start:
    /* ================================================================
     * LUI fundamentals
     * ================================================================ */
    lui   x1,  0x00000       /* verify x1  = 0x00000000 */
    lui   x2,  0x00001       /* verify x2  = 0x00001000 */
    lui   x3,  0x12345       /* verify x3  = 0x12345000 */
    lui   x4,  0xFFFFF       /* verify x4  = 0xfffff000 */

    /* ================================================================
     * AUIPC fundamentals
     * ================================================================ */
    auipc x5,  0x00000       /* verify x5  = PC of this instruction */
    auipc x6,  0x00001       /* verify x6  = PC + 0x00001000 */
    auipc x7,  0xFFFFF       /* verify x7  = PC - 0x00001000 */
    auipc x8,  0x54321       /* verify x8  = PC + 0x54321000 */

    /* ================================================================
     * Full 32-bit address building (auipc+addi, lui+addi)
     * ================================================================ */
    auipc x9,  0xABCDE
    addi  x9,  x9,  0x678    /* verify x9  = PC + 0xabcde678 (at auipc) */
    lui   x10, 0xFEDCB
    addi  x10, x10, 0x321    /* verify x10 = 0xfedcb321 */

    /* ================================================================
     * JAL — forward, backward, and x0 link
     * ================================================================ */
    addi  x11, x0, 0xB0      /* verify x11 = 0x000000b0, forward success marker */
    jal   x12, jal_fwd       /* verify x12 = JAL return address */
    addi  x11, x0, -1
jal_fwd:
    addi  x13, x0,  0xB1     /* verify x13 = 0x000000b1, forward landing marker */

    addi  x14, x0, 0xB2      /* verify x14 = 0x000000b2, backward success marker */
    jal   x0,  jal_bwd_skip
jal_bwd_tgt:
    addi  x15, x0, 0xB3      /* verify x15 = 0x000000b3, backward landing marker */
    beq   x0,  x0, jal_bwd_done
jal_bwd_skip:
    jal   x16, jal_bwd_tgt   /* verify x16 = backward JAL return address */
    addi  x14, x0, -1
jal_bwd_done:

    addi  x17, x0, 0xB4      /* verify x17 = 0x000000b4, JAL x0 success marker */
    jal   x0,  jal_x0_tgt
    addi  x17, x0, -1
jal_x0_tgt:
    addi  x18, x0, 0xB5      /* verify x18 = 0x000000b5, JAL x0 landing marker */

    /* ================================================================
     * JALR — imm=0, nonzero imm, x0 link, odd target (LSB cleared)
     * ================================================================ */
    addi  x19, x0, 0xC0      /* verify x19 = 0x000000c0, jalr imm=0 success marker */
    auipc x20, 0
    addi  x20, x20, 16
    jalr  x21, 0(x20)        /* verify x21 = JALR return address */
    addi  x19, x0, -1
jr0_tgt:
    addi  x22, x0, 0xC1      /* verify x22 = 0x000000c1 */

    addi  x23, x0, 0xC2
    auipc x24, 0
    addi  x24, x24, 20       /* x24 = jr_imm_tgt + 4 */
    jalr  x25, -4(x24)       /* verify x25 = JALR return address (negative imm) */
    addi  x23, x0, -1
jr_imm_tgt:
    addi  x26, x0, 0xC3

    addi  x27, x0, 0xC4
    auipc x28, 0
    addi  x28, x28, 16
    jalr  x0,  0(x28)
    addi  x27, x0, -1
jr_x0_tgt:
    addi  x28, x0, 0xC5      /* verify x28 = 0x000000c5, x0-link landing marker */

    addi  x29, x0, 0xC6
    auipc x30, 0
    addi  x30, x30, 20       /* x30 = &jr_odd_tgt (before +1) */
    addi  x30, x30, 1        /* odd address; JALR clears LSB before fetch */
    jalr  x31, 0(x30)        /* verify x31 = JALR return address */
    addi  x29, x0, -1
jr_odd_tgt:
    addi  x30, x0, 0xC7      /* verify x30 = 0x000000c7, odd-target landing marker */

    /* ================================================================
     * Corner cases — register forwarding into JALR base (rs1)
     * (repurposes x1-x8; fundamentals already exercised above)
     * ================================================================ */
    addi  x31, x0, 0xD0      /* verify x31 = 0x000000d0, N+1 forward success marker */
    auipc x1,  0
    addi  x1,  x1,  16
    jalr  x3,  0(x1)         /* verify x3  = return address, rs1 N+1 forwarded */
    addi  x31, x0, -1
jr_n1_tgt:
    addi  x4,  x0, 0xD1      /* verify x4  = 0x000000d1, N+1 forward landing marker */

    addi  x5,  x0, 0xD2
    auipc x6,  0
    addi  x6,  x6,  16
    jalr  x7,  0(x6)         /* verify x7  = return address, auipc N+1 forwarded */
    addi  x5,  x0, -1
jr_auipc_tgt:
    addi  x8,  x0, 0xD3      /* verify x8  = 0x000000d3, auipc forward landing marker */

    /* Wait for all test results to reach registers */
    nop
    nop
    nop
    nop
