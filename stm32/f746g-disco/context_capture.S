  .syntax unified
  .cpu cortex-m7
  .fpu softvfp
  .thumb

.section .bss
# Syntax: .comm symbol_name, total_size_in_bytes, alignment_in_bytes
# .comm float_reg_buffer, 68, 4 /* unused for now */
.comm int_reg_buffer, 68, 4
.comm fault_reg_buffer, 20, 4

/* Use this for crash logging, in case the main stack is corrupted */
.comm emergency_stack, 512, 8

.equ SCB_BASE,  0xE000ED00
.equ SCB_SHCSR, 0x24
.equ SCB_CFSR,  0x28
.equ SCB_HFSR,  0x2C
.equ SCB_MMFAR, 0x34
.equ SCB_BFAR,  0x38

.section .text

/* Save the fault status registers into the buffer */
.macro save_fault_status
    ldr     r12, =SCB_BASE
    ldr     r3, =fault_reg_buffer
    ldr     r0, [r12, #SCB_SHCSR]
    ldr     r1, [r12, #SCB_CFSR]
    ldr     r2, [r12, #SCB_HFSR]
    stmia   r3!, {r0, r1, r2}
    ldr     r0, [r12, #SCB_MMFAR]
    ldr     r1, [r12, #SCB_BFAR]
    stmia   r3!, {r0, r1}
.endm

/* TODO: don't use SVC_Handler to enter the CrashHandler */
.global SVC_Handler
.thumb_func
SVC_Handler:
    nop

/* Capture the CPU context (integer only) into the buffer */
.global CrashHandler
.thumb_func
CrashHandler:
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

    /* call C++ log_crash(int_regs_ptr, fault_regs_ptr) */
    ldr     sp, =emergency_stack
    ldr     r1, =fault_reg_buffer
    sub     r0, r3, #64
    b       log_crash


.global HardFault_Handler
.thumb_func
HardFault_Handler:
    /* Save live registers, sacrificing r11 */
    ldr     r11, =int_reg_buffer
    stmia   r11!, {r0, r1, r2, r3, r4, r5, r6, r7}
    mov     r4, r8
    mov     r5, r9
    mov     r6, r10
    mov     r7, #0  /* dummy value for r11 */
    stmia   r11!, {r4, r5, r6, r7, r12}
    mov     r0, lr
    mov     r1, #0  /* dummy value for pc */
    mrs     r2, psr
    mov     r3, sp
    stmia   r11!, {r0, r1, r2, r3}

    save_fault_status

    ldr     sp, =emergency_stack
    ldr     r1, =fault_reg_buffer
    ldr     r0, =int_reg_buffer
    b       log_crash

.global BusFault_Handler
.thumb_func
BusFault_Handler:
/* TODO: A bus fault that occurs during a stack push when entering
   the bus fault handler will not escalate to hard fault. The exception
   stack frame will be corrupted, and should *not* be read via save_fault_status!
*/
    nop /* intentional fall-through */

.global MemManage_Handler
.thumb_func
MemManage_Handler:
    nop /* intentional fall-through */

.global UsageFault_Handler
.thumb_func
UsageFault_Handler:
/* escalate to hard fault (test-only) */
    mov     r0, #0
    sub     r0, r0, #1
    blx     r0

    save_fault_status
    b CrashHandler
