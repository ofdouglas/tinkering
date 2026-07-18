.section .text
.global _exception_handler

_exception_handler:
    sw t0, -4(sp)
    sw t1, -8(sp)

    /* Determine trap vs ISR */
    csrrs t0, mcause, x0
    lui   t1, 0x80000
    and   t1, t1, t0
    bne   t1, x0, _isr_handler

_trap_handler:
    /* TODO */
    addi x30, x0, 42
    j _trap_handler

_isr_handler:
    /* MTI handler */
    la t0, _mtime_isr_counter
    lw t1,  0(t0)
    addi t1, t1, 1
    sw t1,  0(t0)
    csrrci x0, mstatus, 8       /* Global interrupt disable  */ 

    lw t1, -8(sp)
    lw t0, -4(sp)
    mret
