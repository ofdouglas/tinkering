.section .text
.global _start

/* Immediate uimm is 5 bits; mie/mip store bits 11, 7, 3 only. uimm=8 sets MSI. */
.macro load_mei rd
    addi \rd, x0, 2047
    addi \rd, \rd, 1
.endm

_start:
    /* --- CSRRSI / CSRRWI / CSRRCI on implemented CSRs (uimm is 5 bits) --- */

    /* Case 1: read-only CSRs via uimm=0 */
    csrrsi x1, misa, 0         /* verify x1  = misa reset value */
    csrrwi x2, mvendorid, 0    /* verify x2  = 0x00000000 */

    /* Case 2: CSRRWI write MSI (uimm=8) and read-back */
    csrrwi x3, mie, 0          /* verify x3  = 0x00000000 (read mie, uimm=0 no write) */
    csrrwi x4, mie, 8          /* verify x4  = 0x00000000 (old mie); mie = MSI */
    csrrsi x5, mie, 0          /* verify x5  = 0x00000008 (write -> read same CSR) */

    /* Case 3: CSRRW MEI then CSRRSI uimm=0 read-back */
    load_mei x10
    csrrw x6, mie, x10         /* verify x6  = 0x00000008 (old mie); mie = MEI|MSI = 0x808 */
    csrrsi x7, mie, 0          /* verify x7  = 0x00000808 (read back) */

    /* Case 4: CSRRCI clear MSI */
    csrrci x8, mie, 8          /* verify x8  = 0x00000808 (old mie) */
    csrrsi x9, mie, 0          /* verify x9  = 0x00000800 (clear MSI -> read same CSR) */

    /* Case 5: mtvec via CSRRWI (low bits land in base field) */
    csrrwi x10, mtvec, 0       /* verify x10 = 0x00000000 */
    csrrwi x11, mtvec, 16      /* verify x11 = 0x00000000 (old mtvec); mtvec = 0x10 */
    csrrsi x12, mtvec, 0       /* verify x12 = 0x00000010 */

    /* Case 6: mip via CSRRWI / CSRRSI (sparse pending bits) */
    csrrwi x13, mip, 0         /* verify x13 = 0x00000000 */
    csrrwi x14, mip, 8         /* verify x14 = 0x00000000 (old mip); mip MSI pending */
    csrrsi x15, mip, 0         /* verify x15 = 0x00000008 */

    /* Case 7: uimm=0 does not write writable CSR (mie unchanged) */
    csrrwi x16, mie, 0         /* verify x16 = 0x00000800 (old mie) */
    csrrsi x17, mie, 0         /* verify x17 = 0x00000800 (unchanged) */

    /* Case 8: read-only misa — uimm=0 read, nonzero uimm write attempted */
    csrrwi x18, misa, 0        /* verify x18 = misa reset value */
    csrrsi x19, misa, 15       /* verify x19 = misa reset value (unchanged) */
    csrrci x20, misa, 7        /* verify x20 = misa reset value (unchanged) */

    /* Case 9: back-to-back immediate CSR on same register */
    csrrwi x21, mie, 8         /* verify x21 = 0x00000800 (old mie); mie = MSI|MEI = 0x808 */
    csrrsi x22, mie, 0         /* verify x22 = 0x00000808 (write -> read same CSR) */
    csrrci x23, mie, 8         /* verify x23 = 0x00000808 (old mie); clear MSI */
    csrrsi x24, mie, 0         /* verify x24 = 0x00000800 */

    nop
    nop
    nop
    nop
