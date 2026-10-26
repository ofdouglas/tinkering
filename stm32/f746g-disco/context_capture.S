  .syntax unified
  .cpu cortex-m7
  .fpu softvfp
  .thumb

.section .bss
# Syntax: .comm symbol_name, total_size_in_bytes, alignment_in_bytes
# .comm float_reg_buffer, 68, 4 /* unused for now */
.comm int_reg_buffer, 68, 4

.section .text
/* Capture the CPU context (integer only) into the buffer 
 *
 * TODO:
 * - add support for floating point registers
 * - handle stack alignment padding
 * - capture fault status
*/

// .global context_capture
// context_capture:
.global SVC_Handler
.thumb_func
SVC_Handler:
    /* r12 = exception stack pointer */
    /* r12 = (EXC_RETURN & 0x04) ? PSP : MSP */
    tst     lr, #4
    ite     eq
    mrseq   r12, msp
    mrsne   r12, psp

    /* r3 = buffer pointer */
    ldr     r3, =int_reg_buffer

    /* copy stacked r0-r3 into buffer*/
    ldmia   r12!, {r0, r1, r2}
    stmia   r3!, {r0, r1, r2}
    ldmia   r12!, {r0}
    stmia   r3!, {r0}

    /* copy r4-r11 from register file into buffer*/
    stmia   r3!, {r4, r5, r6, r7}
    mov     r4, r8
    mov     r5, r9
    mov     r6, r10
    mov     r7, r11
    stmia   r3!, {r4, r5, r6, r7}

    /* copy stacked r12 into buffer */
    ldmia   r12!, {r0}
    stmia   r3!, {r0}

    /* copy stacked lr, pc, psr into buffer */
    ldmia   r12!, {r0, r1, r2}
    stmia   r3!, {r0, r1, r2}

    /* save the original stack pointer into buffer
       xPSR[9] is set if the frame was 4-byte aligned, and has a padding word */
    tst     r2, #512
    it      ne
    orrne   r12, #4
    str     r12, [r3]

    /* call C++ crash logger */
    /* ldr     sp, =_estack   TODO: safe stack */
    sub     r0, r3, #64
    b       log_crash


.global HardFault_Handler
.thumb_func
HardFault_Handler:
    b HardFault_Handler

.global BusFault_Handler
.thumb_func
BusFault_Handler:
    b BusFault_Handler

.global MemManage_Handler
.thumb_func
MemManage_Handler:
    b MemManage_Handler

.global UsageFault_Handler
.thumb_func
UsageFault_Handler:
    b UsageFault_Handler