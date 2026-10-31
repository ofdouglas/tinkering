.section .text
.global _start
_start:
    addi  x1,  x0,  5          /* verify x1  = 5 */
    addi  x2,  x0,  5          /* verify x2  = 5 */
    addi  x3,  x0,  -1         /* verify x3  = -1 */
    addi  x4,  x0,  -1         /* verify x4  = -1 */
    addi  x5,  x0,  0          /* verify x5  = 0 */
    addi  x6,  x0,  0          /* verify x6  = 0 */

    /* bge equal (5,5) taken */
    addi  x7,  x0,  0xB0       /* verify x7  = 0xb0 */
    bge   x1,  x2,  bge_pos_ok
    addi  x7,  x0,  -1
bge_pos_ok:

    /* blt equal (5,5) not taken */
    addi  x8,  x0,  0xB1
    blt   x1,  x2,  blt_pos_fail
    addi  x8,  x0,  0xB2       /* verify x8  = 0xb2 */
    jal   x0,  blt_pos_done
blt_pos_fail:
    addi  x8,  x0,  -1
blt_pos_done:

    /* bgeu equal (5,5) taken */
    addi  x9,  x0,  0xB3       /* verify x9  = 0xb3 */
    bgeu  x1,  x2,  bgeu_pos_ok
    addi  x9,  x0,  -1
bgeu_pos_ok:

    /* bltu equal (5,5) not taken */
    addi  x10, x0,  0xB4
    bltu  x1,  x2,  bltu_pos_fail
    addi  x10, x0,  0xB5       /* verify x10 = 0xb5 */
    jal   x0,  bltu_pos_done
bltu_pos_fail:
    addi  x10, x0,  -1
bltu_pos_done:

    /* bge equal (-1,-1) taken */
    addi  x11, x0,  0xB6       /* verify x11 = 0xb6 */
    bge   x3,  x4,  bge_neg_ok
    addi  x11, x0,  -1
bge_neg_ok:

    /* blt equal (-1,-1) not taken */
    addi  x12, x0,  0xB7
    blt   x3,  x4,  blt_neg_fail
    addi  x12, x0,  0xB8       /* verify x12 = 0xb8 */
    jal   x0,  blt_neg_done
blt_neg_fail:
    addi  x12, x0,  -1
blt_neg_done:

    /* bgeu equal (unsigned -1,-1) taken */
    addi  x13, x0,  0xB9       /* verify x13 = 0xb9 */
    bgeu  x3,  x4,  bgeu_neg_ok
    addi  x13, x0,  -1
bgeu_neg_ok:

    /* bltu equal (unsigned -1,-1) not taken */
    addi  x14, x0,  0xBA
    bltu  x3,  x4,  bltu_neg_fail
    addi  x14, x0,  0xBB       /* verify x14 = 0xbb */
    jal   x0,  bltu_neg_done
bltu_neg_fail:
    addi  x14, x0,  -1
bltu_neg_done:

    /* Equal zeros */
    addi  x15, x0,  0xBC       /* verify x15 = 0xbc */
    bge   x5,  x6,  bge_z_ok
    addi  x15, x0,  -1
bge_z_ok:
    addi  x16, x0,  0xBD
    blt   x5,  x6,  blt_z_fail
    addi  x16, x0,  0xBE       /* verify x16 = 0xbe */
    jal   x0,  blt_z_done
blt_z_fail:
    addi  x16, x0,  -1
blt_z_done:
    addi  x17, x0,  0xBF       /* verify x17 = 0xbf */
    bgeu  x5,  x6,  bgeu_z_ok
    addi  x17, x0,  -1
bgeu_z_ok:
    addi  x18, x0,  0xC0
    bltu  x5,  x6,  bltu_z_fail
    addi  x18, x0,  0xC1       /* verify x18 = 0xc1 */
    jal   x0,  bltu_z_done
bltu_z_fail:
    addi  x18, x0,  -1
bltu_z_done:

    addi  x19, x0,  0xC2       /* verify x19 = 0xc2 */

    nop
    nop
    nop
    nop
