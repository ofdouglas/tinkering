.section .text

.global _start
_start:
    1:
    auipc x10, %pcrel_hi(_trap_handler)
    addi  x10, x10, %pcrel_lo(1b)
    csrrw x11, mtvec, x10            /* verify x11 = 0x00000000 (old mtvec) */
    csrrs x12, mtvec, x0             /* verify x12 = handler address */

    lui   x4, %hi(0x0000beef)
    addi  x4, x4, %lo(0x0000beef)    /* verify x4  = pre-trap marker (survives trap) */
    ecall
    addi  x5, x0, 0x567              /* verify x5  = post-mret marker */
    addi  x6, x2, 0                   /* verify x6  = mepc read in handler (ecall PC) */

    /* Wait for all test results to reach registers */
    nop
    nop
    nop
    nop
1:  j     1b

.global _trap_handler
_trap_handler:
    lui   x1, %hi(0x0000eca1)
    addi  x1, x1, %lo(0x0000eca1)    /* verify x1  = handler-entered marker */
    csrrs x2, mepc, x0               /* verify x2  = saved trap PC (ecall PC - 4 in this RTL) */
    addi  x3, x2, 4                  /* verify x3  = resume PC after ecall (mepc + 4) */
    csrrw x0, mepc, x3
    nop
    nop
    nop
    mret
