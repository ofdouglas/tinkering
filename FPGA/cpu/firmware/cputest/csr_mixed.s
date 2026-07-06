.section .text
.global _start

_start:
    /* RAM base + clear mie for deterministic state */
    lui   x1,  0x00010       /* verify x1  = 0x00010000 */
    csrrw x2,  mie, x0       /* verify x2  = 0x00000000 (old mie) */

    /* --- L1: lw -> CSRRW (load-use into CSR rs1) --- */
    addi  x10, x0,  0x2A4
    sw    x10, 0(x1)
    lw    x11, 0(x1)         /* verify x11 = 0x000002a4 */
    csrrw x12, mie, x11      /* verify x12 = 0x00000000 (old mie); mie = 0x2a4 */
    csrrs x13, mie, x0       /* verify x13 = 0x000002a4 */

    /* --- L2: lw -> CSRRS set (load-use into CSR rs1) --- */
    addi  x10, x0,  0x010
    sw    x10, 4(x1)
    lw    x14, 4(x1)         /* verify x14 = 0x00000010 */
    csrrs x15, mie, x14      /* verify x15 = 0x000002a4 (old mie); mie = 0x2b4 */

    /* --- L3: lw -> CSRRC clear (load-use into CSR rs1) --- */
    addi  x10, x0,  0x004
    sw    x10, 8(x1)
    lw    x16, 8(x1)         /* verify x16 = 0x00000004 */
    csrrc x17, mie, x16      /* verify x17 = 0x000002b4 (old mie); mie = 0x2b0 */

    /* --- G1: addi -> CSRRW (ALU result -> CSR rs1, no gap) --- */
    addi  x18, x0,  0x055
    csrrw x19, mie, x18      /* verify x19 = 0x000002b0 (old mie); mie = 0x55 */

    /* --- G2: lui/addi -> CSRRS set (ALU result -> CSR rs1, no gap) --- */
    lui   x20, 0x3
    addi  x20, x20, 0x0C0
    csrrs x21, mie, x20      /* verify x21 = 0x00000055 (old mie); mie = 0x30d5 */

    /* --- B1: CSRRS rd -> beq taken (branch uses CSR readback) --- */
    csrrs x22, mie, x0       /* verify x22 = 0x000030d5 */
    lui   x23, 0x3
    addi  x23, x23, 0x0d5
    beq   x22, x23, b1_taken
    addi  x24, x0,  -1
b1_taken:
    addi  x24, x0,  0xBE     /* verify x24 = 0x000000be */

    /* --- S1: CSRRS rd -> sw rs2 (store uses CSR readback next insn) --- */
    csrrs x25, mie, x0       /* verify x25 = 0x000030d5 */
    sw    x25, 12(x1)
    lw    x26, 12(x1)        /* verify x26 = 0x000030d5 */

    /* --- C1: CSRRW rd -> CSRRS rs1=rd (CSR rd feeds CSR rs1, same CSR) --- */
    addi  x27, x0,  0x008
    csrrw x28, mie, x27      /* verify x28 = 0x000030d5 (old mie); mie = 0x8 */
    csrrs x29, mie, x28      /* verify x29 = 0x00000008 (old mie); mie = 0x30dd */

    /* --- X1: CSR WB hazard + load-use overlap (csrrw -> lw -> csrrw mie, rs1) --- */
    addi  x10, x0,  0x100
    sw    x10, 16(x1)
    csrrw x30, mie, x27      /* verify x30 = 0x000030dd (old mie); mie = 0x8 */
    lw    x31, 16(x1)        /* verify x31 = 0x00000100 */
    csrrw x2,  mie, x31      /* verify x2  = 0x00000008 (old mie); mie = 0x100 */

    /* --- M1: CSRRWI -> CSRRS (immediate then register CSR, same address) --- */
    csrrwi x3, mie, 7        /* verify x3  = 0x00000100 (old mie); mie = 0x7 */
    csrrs  x4, mie, x0       /* verify x4  = 0x00000007 */
    csrrwi x5, mie, 3        /* verify x5  = 0x00000007 (old mie); mie = 0x3 */
    csrrs  x6, mie, x0       /* verify x6  = 0x00000003 */

    /* --- M2: CSRRS register -> CSRRSI immediate (mixed forms, same CSR) --- */
    addi  x7,  x0,  0x010
    csrrs  x8, mie, x7       /* verify x8  = 0x00000003 (old mie); mie = 0x13 */
    csrrsi x9, mie, 4        /* verify x9  = 0x00000013 (old mie); mie = 0x17 */
    csrrs  x10, mie, x0      /* verify x10 = 0x00000017 */

    /* --- B2: CSRRS rd -> bne taken --- */
    csrrs x11, mie, x0       /* verify x11 = 0x00000017 */
    addi  x12, x0,  0x018
    bne   x11, x12, b2_taken
    addi  x13, x0,  -1
b2_taken:
    addi  x13, x0,  0xC2     /* verify x13 = 0x000000c2 */

    /* --- G3: addi -> CSRRC (ALU mask -> CSR rs1, no gap) --- */
    addi  x14, x0,  0x002
    csrrc x15, mie, x14      /* verify x15 = 0x00000017 (old mie); mie = 0x15 */

    /* --- G4: lui -> CSRRW (wide ALU result -> CSR rs1, no gap) --- */
    lui   x16, 0x4
    addi  x16, x16, 0x020
    csrrw x17, mie, x16      /* verify x17 = 0x00000015 (old mie); mie = 0x4020 */

    /* --- C2: CSRRW rd -> CSRRC rs1=rd (clear using prior CSR readback) --- */
    addi  x18, x0,  0x020
    csrrw x19, mie, x18      /* verify x19 = 0x00004020 (old mie); mie = 0x20 */
    csrrc x20, mie, x19      /* verify x20 = 0x00000020 (old mie); mie = 0x0 */
    csrrs x21, mie, x0       /* verify x21 = 0x00000000 */

    nop
    nop
    nop
    nop
