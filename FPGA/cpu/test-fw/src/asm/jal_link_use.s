.section .text
.global _start
_start:
    /* JAL link consumed at the target (0-gap). */
    auipc x5,  0
    jal   x1,  jal0_tgt
    addi  x31, x0, -1
jal0_tgt:
    addi  x2,  x1,  0
    addi  x3,  x5,  8
    sub   x4,  x2,  x3         /* verify x4  = 0 */
    addi  x6,  x0,  0xE1       /* verify x6  = 0xe1 */

    /* JAL link consumed after a 1-instruction gap. */
    auipc x13, 0
    jal   x14, jal1_tgt
    addi  x31, x0, -2
jal1_tgt:
    addi  x15, x0,  0xE3       /* verify x15 = 0xe3 */
    addi  x16, x14, 0
    addi  x17, x13, 8
    sub   x18, x16, x17        /* verify x18 = 0 */

    /* JALR link consumed at the target (0-gap). */
    addi  x7,  x0,  0xE2       /* verify x7  = 0xe2 */
    auipc x8,  0
    addi  x8,  x8,  16
    jalr  x9,  0(x8)
    addi  x7,  x0,  -1
jr0_tgt:
    addi  x10, x9,  0
    addi  x11, x8,  -4
    sub   x12, x10, x11        /* verify x12 = 0 */

    /* JALR link consumed after a 1-instruction gap. */
    auipc x28, 0
    addi  x28, x28, 20
    jalr  x29, 0(x28)
    addi  x31, x0, -3
    nop
jr1_tgt:
    addi  x30, x0,  0xE7       /* verify x30 = 0xe7 */
    addi  x31, x29, 0
    addi  x25, x28, -8
    sub   x26, x31, x25        /* verify x26 = 0 */

    /* jalr x1, 0(x1): rd and rs1 are the same register. */
    addi  x19, x0,  0xE4       /* verify x19 = 0xe4 */
    auipc x27, 0
    addi  x1,  x27, 16
    jalr  x1,  0(x1)
    addi  x19, x0,  -1
same_tgt:
    addi  x20, x1,  0
    addi  x27, x27, 12
    sub   x27, x20, x27        /* verify x27 = 0 */

    /* Compiler two-instruction call: auipc rd / jalr rd, off(rd). */
    addi  x21, x0,  0xE5       /* verify x21 = 0xe5 */
    auipc x22, 0
    jalr  x22, 16(x22)
    addi  x21, x0,  -1
    nop
call_tgt:
    addi  x23, x22, 0
    addi  x24, x0,  0xE6       /* verify x24 = 0xe6 */

    nop
    nop
    nop
    nop
