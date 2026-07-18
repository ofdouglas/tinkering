.section .startup_text
.global _start

_start:
_init_stack:
    la  sp, _stack_top

_zero_bss:
    la   t0, _bss_start         /* t0 = .bss start  */
    la   t1, _bss_end           /* t1 = .bss end  */
    bge  t0, t1, _zero_done
_zero_loop:
    sw   x0, 0(t0)
    addi t0, t0, 4
    blt  t0, t1, _zero_loop
_zero_done:

_copy_init:
    la   x20, _copy_data         /* x20 = src[0]  */ 
    la   x21, _data_start        /* x21 = dest[0]  */
    la   x22, _data_end          /* x22 = dest[end]  */
    bge  x21, x22, _copy_done
_copy_loop:
    lw   x6, 0(x20)
    sw   x6, 0(x21)
    addi x20, x20, 4
    addi x21, x21, 4
    blt  x21, x22, _copy_loop
_copy_done:

_enable_irq:
    la  t0, _exception_handler
    csrrw  x0, mtvec, t0
    addi   t0, x0, 128
    csrrs  x0, mie, t0          /* Enable machine timer IRQ */ 
    csrrsi x0, mstatus, 8       /* Global interrupt enable  */ 

_jump_main:
    jal main
1:  j 1b


.section .data

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
    j _trap_handler

_isr_handler:
    /* MTI handler */
    j mtim_isr
 
    lw t1, -8(sp)
    lw t0, -4(sp)
    mret
