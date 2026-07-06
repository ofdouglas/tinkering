.section .text
.global _start

/* Five non-CSR instructions between consecutive CSR ops (no CSR control hazards yet). */
.macro csr_gap
    nop
    nop
    nop
    nop
    nop
.endm

_start:
    /* Case 1: read misa (read-only) */
    csrrs x1, misa, x0         /* verify x1  = misa reset value */
    csr_gap

    /* Case 2: read mvendorid (read-only) */
    csrrs x2, mvendorid, x0    /* verify x2  = 0x00000000 */
    csr_gap

    /* Case 3: read mie (writable, reset 0) */
    csrrs x3, mie, x0          /* verify x3  = 0x00000000 */
    csr_gap

    /* Case 4: csrrw mie = 0x1234 */
    lui   x30, 0x1
    addi  x30, x30, 0x234
    csrrw x4, mie, x30         /* verify x4  = 0x00000000 (old mie) */
    csr_gap
    csrrs x5, mie, x0          /* verify x5  = 0x00001234 */
    csr_gap

    /* Case 5: csrrs mie |= 0x800 */
    addi  x30, x0, 0x7ff
    addi  x30, x30, 1
    csrrs x6, mie, x30         /* verify x6  = 0x00001234 (old mie) */
    csr_gap
    csrrs x7, mie, x0          /* verify x7  = 0x00001a34 */
    csr_gap

    /* Case 6: csrrc mie &= ~0x1200 */
    lui   x30, 0x1
    addi  x30, x30, 0x200
    csrrc x8, mie, x30         /* verify x8  = 0x00001a34 (old mie) */
    csr_gap
    csrrs x9, mie, x0          /* verify x9  = 0x00000834 */
    csr_gap

    /* Case 7: read/write mtvec */
    csrrs x10, mtvec, x0       /* verify x10 = 0x00000000 */
    csr_gap
    lui   x30, 0x10
    csrrw x11, mtvec, x30      /* verify x11 = 0x00000000 (old mtvec) */
    csr_gap
    csrrs x12, mtvec, x0       /* verify x12 = 0x00010000 */
    csr_gap

    /* Case 8: read/write mip */
    csrrs x13, mip, x0         /* verify x13 = 0x00000000 */
    csr_gap
    lui   x30, 0x4
    addi  x30, x30, 0x321
    csrrw x14, mip, x30        /* verify x14 = 0x00000000 (old mip) */
    csr_gap
    csrrs x15, mip, x0         /* verify x15 = 0x00004321 */
    csr_gap

    /* Case 9: csrrs with rs1=x0 on read-only misa (read only, no write) */
    csrrs x16, misa, x0        /* verify x16 = misa reset value */
    csr_gap

    /* Case 10: second mtvec write returns previous value */
    lui   x30, 0x20
    csrrw x17, mtvec, x30      /* verify x17 = 0x00010000 (old mtvec) */
    csr_gap
    csrrs x18, mtvec, x0       /* verify x18 = 0x00020000 */
    csr_gap

    nop
    nop
    nop
    nop
