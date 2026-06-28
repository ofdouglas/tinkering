.section .text
.global _start
_start:
    lui sp, %hi(_stack_top)
    addi sp, sp, %lo(_stack_top)
    jal main
1:  j 1b