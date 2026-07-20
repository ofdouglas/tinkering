.equ MSI_IRQ_VALUE, 3
.equ MTI_IRQ_VALUE, 7
.equ MEI_IRQ_VALUE, 11 

.section .text
.global _exception_handler

.extern mei_isr_body
.extern mtim_isr_body
.extern checkpoint

/* Save caller-saved registers for a nested C call. */
.macro SAVE_CALLER_REGS
    addi sp, sp, -64
    sw   ra, 60(sp)
    sw   t0, 56(sp)
    sw   t1, 52(sp)
    sw   t2, 48(sp)
    sw   t3, 44(sp)
    sw   t4, 40(sp)
    sw   t5, 36(sp)
    sw   t6, 32(sp)
    sw   a0, 28(sp)
    sw   a1, 24(sp)
    sw   a2, 20(sp)
    sw   a3, 16(sp)
    sw   a4, 12(sp)
    sw   a5, 8(sp)
    sw   a6, 4(sp)
    sw   a7, 0(sp)
.endm

.macro RESTORE_CALLER_REGS
    lw   ra, 60(sp)
    lw   t0, 56(sp)
    lw   t1, 52(sp)
    lw   t2, 48(sp)
    lw   t3, 44(sp)
    lw   t4, 40(sp)
    lw   t5, 36(sp)
    lw   t6, 32(sp)
    lw   a0, 28(sp)
    lw   a1, 24(sp)
    lw   a2, 20(sp)
    lw   a3, 16(sp)
    lw   a4, 12(sp)
    lw   a5, 8(sp)
    lw   a6, 4(sp)
    lw   a7, 0(sp)
    addi sp, sp, 64
.endm

_exception_handler:
    SAVE_CALLER_REGS
    li    a0, 13
    call  checkpoint

    /* Dispatch to trap or ISR handler. */
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
    jal   ra, mtim_isr
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
    jal   ra, mei_isr
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
    RESTORE_CALLER_REGS
    mret