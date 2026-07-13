
.section .text
.global _start

_start:
_init_stack:
    lui  sp, %hi(_stack_top)
    addi sp, sp, %lo(_stack_top)

_zero_bss:
    lui  t0, %hi(_bss_start)         /* t0 = .bss start  */
    addi t0, t0, %lo(_bss_start)    
    lui  t1, %hi(_bss_end)           /* t1 = .bss end  */
    addi t1, t1, %lo(_bss_end)
    bge  t0, t1, _zero_done
_zero_loop:
    sw   x0, 0(t0)
    addi t0, t0, 4
    blt  t0, t1, _zero_loop
_zero_done:

_copy_init:
    lui  x20, %hi(_copy_data)         /* x20 = src[0]  */ 
    addi x20, x20, %lo(_copy_data)    
    lui  x21, %hi(_data_start)        /* x21 = dest[0]  */
    addi x21, x21, %lo(_data_start)
    lui  x22, %hi(_data_end)          /* x22 = dest[end]  */
    addi x22, x22, %lo(_data_end)
    bge  x21, x22, _copy_done
_copy_loop:
    lw   x6, 0(x20)
    sw   x6, 0(x21)
    addi x20, x20, 4
    addi x21, x21, 4
    blt  x21, x22, _copy_loop
_copy_done:

    jal main
1:  j 1b

.global _trap_handler

_trap_handler:
    beq x0, x0, _trap_handler