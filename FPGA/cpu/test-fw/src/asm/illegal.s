.section .text

.global _start
_start:
    /* ================================================================
     * mtvec setup
     * ================================================================ */
    1:
    auipc x10, %pcrel_hi(_exception_entry)
    addi  x10, x10, %pcrel_lo(1b)
    csrrw x11, mtvec, x10            /* verify x11 = 0x00000000 (old mtvec) */
    csrrs x12, mtvec, x0             /* verify x12 = handler address */

    /* ================================================================
     * Illegal instruction traps (raw encodings; RV32I build only)
     *
     * Handler bumps mepc by 4 so mret resumes at the fall-through addi.
     * ================================================================ */

    /* Invalid target of a not-taken branch must not trap. */
    addi  x21, x0,  0x771             /* verify x21 = not-taken pre-marker */
    bne   x0,  x0,  not_taken_invalid_target
    addi  x22, x0,  0x772             /* verify x22 = not-taken fall-through marker */
    jal   x0,  not_taken_invalid_done
not_taken_invalid_target:
    .word 0x0000007f
    addi  x21, x0,  -1
not_taken_invalid_done:
    addi  x23, x0,  0x773             /* verify x23 = not-taken done marker */

    /* WFI (not implemented) */
    addi  x4,  x0,  0x111             /* verify x4  = pre-WFI marker */
    .word 0x10500073                   /* wfi */
    addi  x5,  x0,  0x222             /* verify x5  = post-WFI marker */

    /* EBREAK (not implemented) */
    addi  x6,  x0,  0x333             /* verify x6  = pre-EBREAK marker */
    .word 0x00100073                   /* ebreak */
    addi  x7,  x0,  0x444             /* verify x7  = post-EBREAK marker */

    /* Primary opcode 0x7F (custom-0 / reserved on RV32I) */
    addi  x8,  x0,  0x555             /* verify x8  = pre-unknown marker */
    .word 0x0000007f
    addi  x9,  x0,  0x666             /* verify x9  = post-unknown marker */

    /* Opcodes from other extensions (encoded via .word, still RV32I build) */
    addi  x16, x0,  0x701             /* verify x16 = pre-AMO marker */
    .word 0x0000002f                   /* amo opcode (A extension) */
    addi  x17, x0,  0x702             /* verify x17 = post-AMO marker */

    addi  x18, x0,  0x703             /* verify x18 = pre-FP marker */
    .word 0x00000053                   /* op-fp opcode (F extension) */
    addi  x19, x0,  0x704             /* verify x19 = post-FP marker */

    nop
    nop
    nop
    nop
2:  j     2b

.global _exception_entry
_exception_entry:
    lui   x15, %hi(0x00011e2a)
    addi  x15, x15, %lo(0x00011e2a)  /* verify x15 = illegal-handler marker */
    csrrs x2,  mepc, x0               /* verify x2  = last illegal PC */
    addi  x3,  x2,  4
    csrrw x0,  mepc, x3
    csrrs x14, mcause, x0             /* verify x14 = 0x00000002 (illegal inst) */
    csrrs x20, mstatus, x0
    mret
