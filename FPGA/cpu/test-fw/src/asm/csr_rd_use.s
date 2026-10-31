.section .text
.global _start

.macro load_msi rd
    addi \rd, x0, 0x008
.endm
.macro load_mti rd
    addi \rd, x0, 0x080
.endm

_start:
    lui   x9,  %hi(_sram_start) /* verify x9  = SRAM base */

    /* A: CSRRW rd -> addi, 0-gap (EX forward of CSR rd). */
    load_msi x2                 /* verify x2  = 0x00000008 */
    csrrw x1,  mie, x2          /* verify x1  = 0x00000000 (old mie) */
    addi  x3,  x1,  1           /* verify x3  = 0x00000001 */

    /* B: CSRRW rd -> addi, 1-gap (MEM/decode forward of CSR rd). */
    load_mti x4                 /* verify x4  = 0x00000080 */
    csrrw x5,  mie, x4          /* verify x5  = 0x00000008 (old mie) */
    nop
    addi  x6,  x5,  1           /* verify x6  = 0x00000009 */

    /* C: CSRRWI rd -> addi, 0-gap. */
    csrrwi x7, mie, 8           /* verify x7  = 0x00000080 (old mie) */
    addi  x8,  x7,  1           /* verify x8  = 0x00000081 */

    /* D: CSRRW rd -> sw rs2, 0-gap. */
    load_mti x10                /* verify x10 = 0x00000080 */
    csrrw x11, mie, x10         /* verify x11 = 0x00000008 (old mie) */
    sw    x11, 0(x9)
    lw    x12, 0(x9)            /* verify x12 = 0x00000008 */

    /* E: CSRRW rd -> beq taken (old=0, new!=0). */
    csrrw x13, mie, x0          /* verify x13 = 0x00000080 (old mie) */
    load_msi x16                /* verify x16 = 0x00000008 */
    csrrw x17, mie, x16         /* verify x17 = 0x00000000 (old mie) */
    addi  x15, x0,  0xBE        /* verify x15 = 0x000000be */
    beq   x17, x0,  e_ok
    addi  x15, x0,  -1
e_ok:
    addi  x18, x0,  0xBF        /* verify x18 = 0x000000bf */

    /* F: CSRRW rd -> beq not-taken (old!=0, new=0). */
    csrrw x20, mie, x0          /* verify x20 = 0x00000008 (old mie) */
    beq   x20, x0,  f_fail
    addi  x21, x0,  0xC0        /* verify x21 = 0x000000c0 */
    jal   x0,  f_done
f_fail:
    addi  x21, x0,  -1
f_done:

    nop
    nop
    nop
    nop
