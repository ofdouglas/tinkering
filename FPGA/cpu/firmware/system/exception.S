.equ MSI_IRQ_VALUE, 3
.equ MTI_IRQ_VALUE, 7
.equ MEI_IRQ_VALUE, 11 

.section .text
.global _vector_table

.extern mtim_isr
.extern uart0_isr

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

_mei_isr:
    j    _mei_isr

_trap_handler:
    /* TODO */
    addi x30, x0, 42
    j    _trap_handler

_vector_table:
/*  Trap handler                 */
    j    _trap_handler

/*  RISC-V Machine-level Interrupts */
/* IRQ_SRC_SSI           = 5'd1,  // Supervisor-level Software Interrupt */
    j    _default_irq_handler
/* IRQ_SRC_RESERVED2     = 5'd2, */
    j    _default_irq_handler
/* IRQ_SRC_MSI           = 5'd3,  // Machine-level Software Interrupt */
    j    _default_irq_handler
/* IRQ_SRC_RESERVED4     = 5'd4, */
    j    _default_irq_handler
/* IRQ_SRC_STI           = 5'd5,  // Supervisor-level Timer Interrupt */
    j    _default_irq_handler
/* IRQ_SRC_RESERVED6     = 5'd6, */
    j    _default_irq_handler
/* IRQ_SRC_MTI           = 5'd7,  // Machine-level Timer Interrupt */
    j     mtim_isr
/* IRQ_SRC_RESERVED8     = 5'd8, */
    j    _default_irq_handler
/* IRQ_SRC_SEI           = 5'd9,  // Supervisor-level External Interrupt */
    j    _default_irq_handler
/* IRQ_SRC_RESERVED10    = 5'd10, */
    j    _default_irq_handler
/* IRQ_SRC_MEI           = 5'd11, // Machine-level External Interrupt */
    j     _mei_isr
/* IRQ_SRC_RESERVED12    = 5'd12, */
    j    _default_irq_handler
/* IRQ_SRC_LCOFI         = 5'd13, */
    j    _default_irq_handler
/* IRQ_SRC_RESERVED14    = 5'd14, */
    j    _default_irq_handler
/* IRQ_SRC_RESERVED15    = 5'd15, */
    j    _default_irq_handler

/* SoC External Interrupts         */
/* EXT_IRQ_0             =  5'd16, */
    j    uart0_isr
/* EXT_IRQ_1             =  5'd17, */
    j    _default_irq_handler
/* EXT_IRQ_2             =  5'd18, */
    j    _default_irq_handler
/* EXT_IRQ_3             =  5'd19, */
    j    _default_irq_handler
/* EXT_IRQ_4             =  5'd20, */
    j    _default_irq_handler
/* EXT_IRQ_5             =  5'd21, */
    j    _default_irq_handler
/* EXT_IRQ_6             =  5'd22, */
    j    _default_irq_handler
/* EXT_IRQ_7             =  5'd23, */
    j    _default_irq_handler
/* EXT_IRQ_8             =  5'd24, */
    j    _default_irq_handler
/* EXT_IRQ_9             =  5'd25, */
    j    _default_irq_handler
/* EXT_IRQ_10            =  5'd26, */
    j    _default_irq_handler
/* EXT_IRQ_11            =  5'd27, */
    j    _default_irq_handler
/* EXT_IRQ_12            =  5'd28, */
    j    _default_irq_handler
/* EXT_IRQ_13            =  5'd29, */
    j    _default_irq_handler
/* EXT_IRQ_14            =  5'd30, */
    j    _default_irq_handler

_default_irq_handler:
    j     _default_irq_handler
