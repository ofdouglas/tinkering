.section .text
.global _start

_start:
    /* --- CSRRSI / CSRRWI / CSRRCI on implemented CSRs (uimm is 5 bits) --- */

    /* Case 1: read-only CSRs via uimm=0 */
    csrrsi x1, misa, 0         /* verify x1  = misa reset value */
    csrrwi x2, mvendorid, 0     /* verify x2  = 0x00000000 */

    /* Case 2: CSRRWI write and read-back */
    csrrwi x3, mie, 0           /* verify x3  = 0x00000000 (read mie, uimm=0 no write) */
    csrrwi x4, mie, 18          /* verify x4  = 0x00000000 (old mie); mie = 0x12 */
    csrrsi x5, mie, 0           /* verify x5  = 0x00000012 (write -> read same CSR) */

    /* Case 3: CSRRSI set -> read */
    csrrsi x6, mie, 5           /* verify x6  = 0x00000012 (old mie) */
    csrrsi x7, mie, 0           /* verify x7  = 0x00000017 (set -> read same CSR) */

    /* Case 4: CSRRCI clear -> read */
    csrrci x8, mie, 2           /* verify x8  = 0x00000017 (old mie) */
    csrrsi x9, mie, 0           /* verify x9  = 0x00000015 (clear -> read same CSR) */

    /* Case 5: mtvec via CSRRWI */
    csrrwi x10, mtvec, 0        /* verify x10 = 0x00000000 */
    csrrwi x11, mtvec, 16       /* verify x11 = 0x00000000 (old mtvec); mtvec = 0x10 */
    csrrsi x12, mtvec, 0        /* verify x12 = 0x00000010 */

    /* Case 6: mip via CSRRWI / CSRRSI */
    csrrwi x13, mip, 0          /* verify x13 = 0x00000000 */
    csrrwi x14, mip, 21         /* verify x14 = 0x00000000 (old mip); mip = 0x15 */
    csrrsi x15, mip, 0          /* verify x15 = 0x00000015 */

    /* Case 7: uimm=0 does not write writable CSR (mie unchanged) */
    csrrwi x16, mie, 0          /* verify x16 = 0x00000015 (old mie) */
    csrrsi x17, mie, 0          /* verify x17 = 0x00000015 (unchanged) */

    /* Case 8: read-only misa — uimm=0 read, nonzero uimm write attempted */
    csrrwi x18, misa, 0         /* verify x18 = misa reset value */
    csrrsi x19, misa, 15        /* verify x19 = misa reset value (unchanged) */
    csrrci x20, misa, 7         /* verify x20 = misa reset value (unchanged) */

    /* Case 9: back-to-back immediate CSR on same register */
    csrrwi x21, mie, 31         /* verify x21 = 0x00000015 (old mie); mie = 0x1f */
    csrrsi x22, mie, 0          /* verify x22 = 0x0000001f (write -> read same CSR) */
    csrrci x23, mie, 16         /* verify x23 = 0x0000001f (old mie); mie = 0x1f & ~16 = 0x0f */
    csrrsi x24, mie, 0          /* verify x24 = 0x0000000f */

    nop
    nop
    nop
    nop
