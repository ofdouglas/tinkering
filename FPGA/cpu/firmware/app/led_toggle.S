.section .text
.global _start

/* Tiny SRAM_TEXT payload: turn LED0 on, then off, then spin. */
_start:
    lui  t0, 0x80000
    addi t1, x0, 1
    sw   t1, 0(t0)
    sw   x0, 0(t0)
1:
    jal  x0, 1b
