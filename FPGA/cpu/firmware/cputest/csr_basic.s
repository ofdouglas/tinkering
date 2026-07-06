.section .text
.global _start

_start:
    /* --- Basic CSR access (back-to-back where safe) --- */
    csrrs x1, misa, x0         /* verify x1  = misa reset value */
    csrrs x2, mvendorid, x0    /* verify x2  = 0x00000000 */

    lui   x30, 0x1
    addi  x30, x30, 0x234
    csrrw x3, mie, x30         /* verify x3  = 0x00000000 (old mie) */
    csrrs x4, mie, x0          /* verify x4  = 0x00001234 (write -> read same CSR) */

    addi  x30, x0, 0x7ff
    addi  x30, x30, 1
    csrrs x5, mie, x30         /* verify x5  = 0x00001234 (old mie) */
    csrrs x6, mie, x0          /* verify x6  = 0x00001a34 (set -> read same CSR) */

    lui   x30, 0x1
    addi  x30, x30, 0x200
    csrrc x7, mie, x30         /* verify x7  = 0x00001a34 (old mie) */
    csrrs x8, mie, x0          /* verify x8  = 0x00000834 (clear -> read same CSR) */

    csrrs x9, mtvec, x0        /* verify x9  = 0x00000000 */
    lui   x30, 0x10
    csrrw x10, mtvec, x30      /* verify x10 = 0x00000000 (old mtvec) */
    csrrs x11, mtvec, x0       /* verify x11 = 0x00010000 */

    csrrs x12, mip, x0         /* verify x12 = 0x00000000 */
    lui   x30, 0x4
    addi  x30, x30, 0x321
    csrrw x13, mip, x30        /* verify x13 = 0x00000000 (old mip) */
    csrrs x14, mip, x0         /* verify x14 = 0x00004321 */

    csrrs x15, misa, x0        /* verify x15 = misa reset value (read-only reread) */

    /* --- CSR control hazards (back-to-back same CSR unless noted) --- */

    /* H1: CSRRW -> CSRRS read same CSR */
    lui   x30, 0x3
    addi  x30, x30, 0x045
    csrrw x16, mie, x30        /* verify x16 = 0x00000834 (old mie) */
    csrrs x17, mie, x0         /* verify x17 = 0x00003045 */

    /* H2: CSRRW -> CSRRW same CSR (second rd sees first write) */
    addi  x30, x0, 0x055
    csrrw x18, mie, x30        /* verify x18 = 0x00003045 (old mie) */
    csrrw x19, mie, x30        /* verify x19 = 0x00000055 (old mie after prior write) */

    /* H3: CSRRW -> CSRRS set -> CSRRS read same CSR */
    lui   x30, 0x2
    csrrw x20, mie, x30        /* verify x20 = 0x00000055 (old mie) */
    addi  x30, x0, 0x033
    csrrs x21, mie, x30        /* verify x21 = 0x00002000 (old mie) */
    csrrs x22, mie, x0         /* verify x22 = 0x00002033 (set -> read same CSR) */

    /* H4: CSRRW -> CSRRS set -> CSRRS read same CSR (different set bits) */
    lui   x30, 0x3
    csrrw x23, mie, x30        /* verify x23 = 0x00002033 (old mie) */
    addi  x30, x0, 0x0c0
    csrrs x24, mie, x30        /* verify x24 = 0x00003000 (old mie) */
    csrrs x25, mie, x0         /* verify x25 = 0x000030c0 (set -> read same CSR) */

    /* H5: write CSR A -> read CSR B (different address, no CSR WB hazard) */
    addi  x30, x0, 0x07f
    csrrw x26, mie, x30        /* verify x26 = 0x000030c0 (old mie) */
    csrrs x27, mip, x0         /* verify x27 = 0x00004321 (unchanged mip) */

    /* H6: CSRRC clear -> read same CSR (rs1 from prior addi) */
    addi  x30, x0, 0x040
    csrrc x28, mie, x30        /* verify x28 = 0x0000007f (old mie) */
    csrrs x29, mie, x0         /* verify x29 = 0x0000003f (clear -> read same CSR) */

    /* H7: mtvec write chain */
    lui   x30, 0x20
    csrrw x18, mtvec, x30      /* verify x18 = 0x00010000 (old mtvec; overwrites H2 x18) */
    csrrw x19, mtvec, x30      /* verify x19 = 0x00020000 (old mtvec after prior write) */
    csrrs x20, mtvec, x0       /* verify x20 = 0x00020000 (overwrites H3 x20) */

    /* H8: CSR read result used immediately by ALU (x-register hazard) */
    csrrs x30, mie, x0         /* verify x30 = 0x0000003f */
    addi  x31, x30, 1          /* verify x31 = 0x00000040 */

    /* --- rs1/x0 semantics (overwrites x16-x27; mie=0x3f before this block) --- */

    /* Z1: CSRRW rs1=x0 writes zero to writable CSR */
    csrrw x16, mie, x0         /* verify x16 = 0x0000003f (old mie) */
    csrrs x17, mie, x0         /* verify x17 = 0x00000000 */

    /* Z2: restore mie, then CSRRS rs1!=x0 with rs1=0 (write path, data unchanged) */
    addi  x30, x0, 0x03f
    csrrw x18, mie, x30        /* verify x18 = 0x00000000 (old mie) */
    addi  x30, x0, 0
    csrrs x19, mie, x30        /* verify x19 = 0x0000003f (old mie) */
    csrrs x20, mie, x0         /* verify x20 = 0x0000003f (unchanged) */

    /* Z3: CSRRC rs1=x0 is read-only (no CSR write) */
    csrrc x21, mie, x0         /* verify x21 = 0x0000003f (old mie) */
    csrrs x22, mie, x0         /* verify x22 = 0x0000003f (unchanged) */

    /* Z4: CSRRC rs1!=x0 with rs1=0 (write path, data unchanged) */
    addi  x30, x0, 0
    csrrc x23, mie, x30        /* verify x23 = 0x0000003f (old mie) */
    csrrs x24, mie, x0         /* verify x24 = 0x0000003f (unchanged) */

    /* Z5: CSRRW rs1=x0 to read-only misa (write attempted; value unchanged) */
    csrrw x25, misa, x0        /* verify x25 = 0x40000080 (old misa) */
    csrrs x26, misa, x0        /* verify x26 = 0x40000080 (unchanged) */

    /* Z6: CSRRS rs1!=x0 on read-only misa (write attempted; value unchanged) */
    lui   x30, 0x1
    csrrs x27, misa, x30        /* verify x27 = 0x40000080 (old misa) */

    nop
    nop
    nop
    nop
