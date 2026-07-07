.section .text

.global _start
_start:
    /* ================================================================
     * mtvec setup
     * ================================================================ */
    1:
    auipc x10, %pcrel_hi(_exception_entry)
    addi  x10, x10, %pcrel_lo(1b)
    csrrw x11, mtvec, x10            /* verify x11 = 0x00000000 (old mtvec) */
    csrrs x12, mtvec, x0             /* verify x12 = handler address */

    /* Enable MEI in mie (per-interrupt enable). Global MIE stays off until
     * after the synchronous ECALL trap is exercised. */
    addi  x21, x0, 2047
    addi  x21, x21, 1              /* x21 = 0x00000800 (MEI mask) */
    csrrs x22, mie, x21            /* verify x22 = 0x00000000 (old mie) */
    csrrs x23, mie, x0             /* verify x23 = 0x00000800 */

    /* ================================================================
     * Synchronous ECALL trap
     * ================================================================ */
    lui   x4, %hi(0x0000beef)
    addi  x4, x4, %lo(0x0000beef)    /* verify x4  = pre-trap marker */
    ecall
    addi  x17, x0, 0x567             /* verify x17 = post-mret marker */
    addi  x6,  x2,  0                 /* verify x6  = mepc saved by trap handler */

    /* ================================================================
     * External interrupt (cpu_tb asserts ext_irq at _irq_wait when +CPUTEST_IRQ)
     * ================================================================ */
    addi  x18, x0,  0x10d              /* verify x18 = pre-IRQ marker */
    csrrsi x25, mstatus, 8           /* enable MIE; x25 = pre-enable mstatus (mpie=1) */
_irq_wait:
    addi  x0,  x0,  0                 /* IRQ mepc = PC of this instruction */
    j     _irq_wait

    nop
    nop
    nop
    nop
2:  j     2b

.global _exception_entry
_exception_entry:
    csrrs x13, mcause, x0
    lui   x31, 0x80000
    and   x31, x31, x13
    bne   x0,  x31, _irq_handler

_trap_handler:
    lui   x15, %hi(0x0000eca1)
    addi  x15, x15, %lo(0x0000eca1)  /* verify x15 = trap-handler marker */
    csrrs x2,  mepc, x0               /* verify x2  = ecall PC */
    addi  x3,  x2,  4                 /* verify x3  = ecall PC + 4 */
    csrrw x0,  mepc, x3
    csrrs x14, mcause, x0             /* verify x14 = 0x0000000B (sync ecall) */
    csrrs x20, mstatus, x0            /* verify x20 = mie cleared, mpie saved */
    mret

_irq_handler:
    csrrs x8,  mepc, x0               /* verify x8  = interrupted PC (_irq_wait) */
    csrrs x7,  mcause, x0             /* verify x7  = 0x8000000B (MEI) */
    addi  x9,  x0,  0xFF              /* verify x9  = IRQ-handler marker */
    csrrs x24, mstatus, x0            /* verify x24 = mstatus after IRQ entry */
    mret
