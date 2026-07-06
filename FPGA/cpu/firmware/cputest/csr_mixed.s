.section .text
.global _start

.macro load_mei rd
    addi \rd, x0, 2047
    addi \rd, \rd, 1
.endm
.macro load_mti rd
    addi \rd, x0, 0x080
.endm
.macro load_msi rd
    addi \rd, x0, 0x008
.endm
.macro load_all rd
    addi \rd, x0, 2047
    addi \rd, \rd, 0x089
.endm

_start:
    /* RAM base + clear mie for deterministic state */
    lui   x1,  0x00010       /* verify x1  = 0x00010000 */
    csrrw x2,  mie, x0       /* verify x2  = 0x00000000 (old mie) */

    /* --- L1: lw -> CSRRW (load-use into CSR rs1) --- */
    load_mei x10
    sw    x10, 0(x1)
    lw    x11, 0(x1)         /* verify x11 = 0x00000800 */
    csrrw x12, mie, x11      /* verify x12 = 0x00000000 (old mie); mie = MEI */
    csrrs x13, mie, x0       /* verify x13 = 0x00000800 */

    /* --- L2: lw -> CSRRS set (load-use into CSR rs1) --- */
    load_msi x10
    sw    x10, 4(x1)
    lw    x14, 4(x1)         /* verify x14 = 0x00000008 */
    csrrs x15, mie, x14      /* verify x15 = 0x00000800 (old mie); mie = MEI|MSI */

    /* --- L3: lw -> CSRRC clear (load-use into CSR rs1) --- */
    load_mti x10
    sw    x10, 8(x1)
    lw    x16, 8(x1)         /* verify x16 = 0x00000080 */
    csrrc x17, mie, x16      /* verify x17 = 0x00000808 (old mie); mie = MSI only */

    /* --- G1: addi -> CSRRW (ALU result -> CSR rs1, no gap) --- */
    load_mti x18
    csrrw x19, mie, x18      /* verify x19 = 0x00000008 (old mie); mie = MTI|MSI = 0x88 */

    /* --- G2: addi -> CSRRS set (ALU result -> CSR rs1, no gap) --- */
    load_mei x20
    csrrs x21, mie, x20      /* verify x21 = 0x00000088 (old mie); mie = all three = 0x888 */

    /* --- B1: CSRRS rd -> beq taken (branch uses CSR readback) --- */
    csrrs x22, mie, x0       /* verify x22 = 0x00000888 */
    load_all x23
    beq   x22, x23, b1_taken
    addi  x24, x0,  -1
b1_taken:
    addi  x24, x0,  0xBE     /* verify x24 = 0x000000be */

    /* --- S1: CSRRS rd -> sw rs2 (store uses CSR readback next insn) --- */
    csrrs x25, mie, x0       /* verify x25 = 0x00000888 */
    sw    x25, 12(x1)
    lw    x26, 12(x1)        /* verify x26 = 0x00000888 */

    /* --- C1: CSRRW rd -> CSRRS rs1=MEI mask (CSR rd feeds separate set bits) --- */
    load_msi x27
    csrrw x28, mie, x27      /* verify x28 = 0x00000888 (old mie); mie = MSI only */
    load_mei x10
    csrrs x29, mie, x10      /* verify x29 = 0x00000008 (old mie); mie = MEI|MSI = 0x808 */

    /* --- X1: CSR WB hazard + load-use overlap --- */
    load_mti x10
    sw    x10, 16(x1)
    csrrw x30, mie, x27      /* verify x30 = 0x00000808 (old mie); mie = MSI */
    lw    x31, 16(x1)        /* verify x31 = 0x00000080 */
    csrrw x2,  mie, x31      /* verify x2  = 0x00000008 (old mie); mie = MTI|MSI */

    /* --- M1: CSRRWI -> CSRRS (immediate then register CSR, same address) --- */
    csrrwi x3, mie, 8        /* verify x3  = 0x00000080 (old mie); mie = MSI|MTI = 0x88 */
    csrrs  x4, mie, x0       /* verify x4  = 0x00000088 */
    csrrwi x5, mie, 8        /* verify x5  = 0x00000088 (old mie); mie unchanged shape */
    csrrs  x6, mie, x0       /* verify x6  = 0x00000088 */

    /* --- M2: CSRRS register -> CSRRSI immediate (mixed forms, same CSR) --- */
    load_mti x7
    csrrs  x8, mie, x7       /* verify x8  = 0x00000088 (old mie); mie unchanged (MTI already set) */
    csrrsi x9, mie, 8        /* verify x9  = 0x00000088 (old mie); OR uimm MSI unchanged */
    csrrs  x10, mie, x0      /* verify x10 = 0x00000088 */

    /* --- B2: CSRRS rd -> bne taken --- */
    csrrs x11, mie, x0       /* verify x11 = 0x00000088 */
    load_all x12
    bne   x11, x12, b2_taken
    addi  x13, x0,  -1
b2_taken:
    addi  x13, x0,  0xC2     /* verify x13 = 0x000000c2 */

    /* --- G3: addi -> CSRRC (ALU mask -> CSR rs1, no gap) --- */
    load_msi x14
    csrrc x15, mie, x14      /* verify x15 = 0x00000088 (old mie); mie = MTI only */

    /* --- G4: addi -> CSRRW (ALU result -> CSR rs1, no gap) --- */
    load_mei x16
    csrrw x17, mie, x16      /* verify x17 = 0x00000080 (old mie); mie = MEI only */

    /* --- C2: CSRRW rd -> CSRRC rs1=rd (clear using prior CSR readback) --- */
    load_mti x18
    csrrw x19, mie, x18      /* verify x19 = 0x00000800 (old mie); mie = MTI only = 0x80 */
    csrrc x20, mie, x19      /* verify x20 = 0x00000080 (old mie); clear MEI mask from x19 (no-op on MTI-only mie) */
    csrrs x21, mie, x0       /* verify x21 = 0x00000080 */

    nop
    nop
    nop
    nop
