.section .text
.global _start

/* mie/mip sparse IRQ bits: MEI=11 (0x800), MTI=7 (0x80), MSI=3 (0x8). */
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
    /* --- Basic CSR access (back-to-back where safe) --- */
    csrrs x1, misa, x0         /* verify x1  = misa reset value */
    csrrs x2, mvendorid, x0    /* verify x2  = 0x00000000 */

    load_mei x30
    csrrw x3, mie, x30         /* verify x3  = 0x00000000 (old mie) */
    csrrs x4, mie, x0          /* verify x4  = 0x00000800 (write -> read same CSR) */

    load_msi x30
    csrrs x5, mie, x30         /* verify x5  = 0x00000800 (old mie) */
    csrrs x6, mie, x0          /* verify x6  = 0x00000808 (set -> read same CSR) */

    load_mei x30
    csrrc x7, mie, x30         /* verify x7  = 0x00000808 (old mie) */
    csrrs x8, mie, x0          /* verify x8  = 0x00000008 (clear -> read same CSR) */

    csrrs x9, mtvec, x0        /* verify x9  = 0x00000000 */
    lui   x30, 0x10
    csrrw x10, mtvec, x30      /* verify x10 = 0x00000000 (old mtvec) */
    csrrs x11, mtvec, x0       /* verify x11 = 0x00010000 */

    csrrs x12, mip, x0         /* verify x12 = 0x00000000 */
    load_all x30
    csrrw x13, mip, x30        /* verify x13 = 0x00000000 (old mip) */
    csrrs x14, mip, x0         /* verify x14 = 0x00000888 */

    csrrs x15, misa, x0        /* verify x15 = misa reset value (read-only reread) */

    /* --- CSR control hazards (back-to-back same CSR unless noted) --- */

    /* H1: CSRRW -> CSRRS read same CSR */
    load_mti x30
    csrrw x16, mie, x30        /* verify x16 = 0x00000008 (old mie) */
    csrrs x17, mie, x0         /* verify x17 = 0x00000080 (write MTI only) */

    /* H2: CSRRW -> CSRRW same CSR (second rd sees first write) */
    load_msi x30
    csrrw x18, mie, x30        /* verify x18 = 0x00000080 (old mie) */
    csrrw x19, mie, x30        /* verify x19 = 0x00000008 (old mie after prior write) */

    /* H3: CSRRW -> CSRRS set -> CSRRS read same CSR */
    load_mei x30
    csrrw x20, mie, x30        /* verify x20 = 0x00000008 (old mie) */
    load_msi x30
    csrrs x21, mie, x30        /* verify x21 = 0x00000800 (old mie) */
    csrrs x22, mie, x0         /* verify x22 = 0x00000808 (set -> read same CSR) */

    /* H4: CSRRW -> CSRRS set MTI -> read same CSR */
    load_mei x30
    csrrw x23, mie, x30        /* verify x23 = 0x00000808 (old mie) */
    load_mti x30
    csrrs x24, mie, x30        /* verify x24 = 0x00000800 (old mie) */
    csrrs x25, mie, x0         /* verify x25 = 0x00000880 (set -> read same CSR) */

    /* H5: write CSR A -> read CSR B (different address, no CSR WB hazard) */
    load_msi x30
    csrrw x26, mie, x30        /* verify x26 = 0x00000880 (old mie) */
    csrrs x27, mip, x0         /* verify x27 = 0x00000888 (unchanged mip) */

    /* H6: CSRRC clear MSI -> read same CSR */
    load_msi x30
    csrrc x28, mie, x30        /* verify x28 = 0x00000008 (old mie) */
    csrrs x29, mie, x0         /* verify x29 = 0x00000000 (clear -> read same CSR) */

    /* H7: mtvec write chain */
    lui   x30, 0x20
    csrrw x18, mtvec, x30      /* verify x18 = 0x00010000 (old mtvec; overwrites H2 x18) */
    csrrw x19, mtvec, x30      /* verify x19 = 0x00020000 (old mtvec after prior write) */
    csrrs x20, mtvec, x0       /* verify x20 = 0x00020000 (overwrites H3 x20) */

    /* H8: CSR read result used immediately by ALU (x-register hazard) */
    load_msi x30
    csrrw x30, mie, x30        /* verify x30 = 0x00000000 (old mie); mie = MSI only */
    csrrs x30, mie, x0         /* verify x30 = 0x00000008 */
    addi  x31, x30, 1          /* verify x31 = 0x00000009 */

    /* --- rs1/x0 semantics (mie=0x8 before this block) --- */

    /* Z1: CSRRW rs1=x0 writes zero to writable CSR */
    csrrw x16, mie, x0         /* verify x16 = 0x00000008 (old mie) */
    csrrs x17, mie, x0         /* verify x17 = 0x00000000 */

    /* Z2: restore mie, then CSRRS rs1!=x0 with rs1=0 (write path, data unchanged) */
    load_msi x30
    csrrw x18, mie, x30        /* verify x18 = 0x00000000 (old mie) */
    addi  x30, x0, 0
    csrrs x19, mie, x30        /* verify x19 = 0x00000008 (old mie) */
    csrrs x20, mie, x0         /* verify x20 = 0x00000008 (unchanged) */

    /* Z3: CSRRC rs1=x0 is read-only (no CSR write) */
    csrrc x21, mie, x0         /* verify x21 = 0x00000008 (old mie) */
    csrrs x22, mie, x0         /* verify x22 = 0x00000008 (unchanged) */

    /* Z4: CSRRC rs1!=x0 with rs1=0 (write path, data unchanged) */
    addi  x30, x0, 0
    csrrc x23, mie, x30        /* verify x23 = 0x00000008 (old mie) */
    csrrs x24, mie, x0         /* verify x24 = 0x00000008 (unchanged) */

    /* Z5: CSRRW rs1=x0 to read-only misa (write attempted; value unchanged) */
    csrrw x25, misa, x0        /* verify x25 = 0x40000080 (old misa) */
    csrrs x26, misa, x0        /* verify x26 = 0x40000080 (unchanged) */

    /* Z6: CSRRS rs1!=x0 on read-only misa (write attempted; value unchanged) */
    load_mei x30
    csrrs x27, misa, x30       /* verify x27 = 0x40000080 (old misa) */

    nop
    nop
    nop
    nop
