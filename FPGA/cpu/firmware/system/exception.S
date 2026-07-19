.equ MSI_IRQ_VALUE, 3
.equ MTI_IRQ_VALUE, 7
.equ MEI_IRQ_VALUE, 11 

.section .text
.global _exception_handler

_exception_handler:
    /* Dispatch to trap or ISR handler,
       with interrupt bit cleared         */
    csrrs t0, mcause, x0
    lui   t1, 0x80000
    and   t1, t1, t0
    beq   t1, x0, _trap_handler
    xor   t0, t0, t1
    j     _isr_handler

_trap_handler:
    /* TODO */
    addi x30, x0, 42
    j    _trap_handler

_isr_handler:
/* Machine timer interrupt */
    la   t1, MTI_IRQ_VALUE
    bne  t1, t0, _skip_mti
    j    mtim_isr
    mret
_skip_mti:

/* Machine external interrupt */
    la    t1, MEI_IRQ_VALUE
    bne   t1, t0, _skip_mei
    j     mei_isr
    mret
_skip_mei:

_default_irq_handler:
    j     _default_irq_handler
