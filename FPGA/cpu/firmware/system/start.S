.section .startup_text
.global _start

_start:
/* Initialize stack and exception entry */
    /* la sp, _stack_top relaxes to auipc+addi(0) and lands at RAM base, not stack top */
    lui  sp, %hi(_stack_top)
    addi sp, sp, %lo(_stack_top)
    la  t0, _exception_handler
    csrrw  x0, mtvec, t0

/* Zero the .bss section */
_zero_bss:
    la   t0, _bss_start         /* t0 = .bss start  */
    la   t1, _bss_end           /* t1 = .bss end  */
    bge  t0, t1, _zero_done
_zero_loop:
    sw   x0, 0(t0)
    addi t0, t0, 4
    blt  t0, t1, _zero_loop
_zero_done:

/* Initialize the .data sections */
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

/* Call C++ global constructors (.init_array). */
_init_ctors:
    la   s0, _init_array_start
    la   s1, _init_array_end
    bge  s0, s1, _init_ctors_done
_init_ctors_loop:
    lw   t0, 0(s0)
    jalr ra, t0, 0
    addi s0, s0, 4
    blt  s0, s1, _init_ctors_loop
_init_ctors_done:

_paint_stack:
    la   s2, 0x77777777
    la   s3, _stack_bottom
    la   s4, _stack_top
    bge  s3, s4, _jump_main
_paint_stack_loop:
    sw   s2, 0(s3)
    addi s3, s3, 4
    blt  s3, s4, _paint_stack_loop

_jump_main:
    jal main
1:  j 1b

