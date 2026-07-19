.equ MSI_IRQ_VALUE, 3
.equ MTI_IRQ_VALUE, 7
.equ MEI_IRQ_VALUE, 11 

.section .text
.global _exception_handler


_exception_handler:
    sw    t0, -4(sp)
    sw    t1, -8(sp)

    /* Dispatch to trap or ISR handler,
       with interrupt bit cleared         */
    csrrs t0, mcause, x0
    lui   t1, 0x80000
    and   t1, t1, t0
    beq   t1, x0, _trap_handler
    andi  t0, t0, 15
    sll   t0, t0, 3
    la    t1, _isr_handler_table
    add   t1, t1, t0
    jalr  x0, 0(t1)

_trap_handler:
    /* TODO */
    addi x30, x0, 42
    j    _trap_handler

_isr_handler_table:
/*  IRQ_SRC_RESERVED0     = 5'd0 */
    j    _default_irq_handler
    j    _default_irq_handler
/* IRQ_SRC_SSI           = 5'd1,  // Supervisor-level Software Interrupt */
    j    _default_irq_handler
    j    _default_irq_handler
/* IRQ_SRC_RESERVED2     = 5'd2, */
    j    _default_irq_handler
    j    _default_irq_handler
/* IRQ_SRC_MSI           = 5'd3,  // Machine-level Software Interrupt */
    j    _default_irq_handler
    j    _default_irq_handler
/* IRQ_SRC_RESERVED4     = 5'd4, */
    j    _default_irq_handler
    j    _default_irq_handler
/* IRQ_SRC_STI           = 5'd5,  // Supervisor-level Timer Interrupt */
    j    _default_irq_handler
    j    _default_irq_handler
/* IRQ_SRC_RESERVED6     = 5'd6, */
    j    _default_irq_handler
    j    _default_irq_handler
/* IRQ_SRC_MTI           = 5'd7,  // Machine-level Timer Interrupt */
    j    mtim_isr
    j    _exception_return
/* IRQ_SRC_RESERVED8     = 5'd8, */
    j    _default_irq_handler
    j    _default_irq_handler
/* IRQ_SRC_SEI           = 5'd9,  // Supervisor-level External Interrupt */
    j    _default_irq_handler
    j    _default_irq_handler
/* IRQ_SRC_RESERVED10    = 5'd10, */
    j    _default_irq_handler
    j    _default_irq_handler
/* IRQ_SRC_MEI           = 5'd11, // Machine-level External Interrupt */
    j    mei_isr
    j    _exception_return
/* IRQ_SRC_RESERVED12    = 5'd12, */
    j    _default_irq_handler
    j    _default_irq_handler
/* IRQ_SRC_LCOFI         = 5'd13, */
    j    _default_irq_handler
    j    _default_irq_handler
/* IRQ_SRC_RESERVED14    = 5'd14, */
    j    _default_irq_handler
    j    _default_irq_handler
/* IRQ_SRC_RESERVED15    = 5'd15, */
    j    _default_irq_handler
    j    _default_irq_handler

_default_irq_handler:
    j     _default_irq_handler

_exception_return:
    lw    t0, -4(sp)
    lw    t1, -8(sp)
    mret
