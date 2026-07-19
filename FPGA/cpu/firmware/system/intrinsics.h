#ifndef INTRINSICS_H
#define INTRINSICS_H

#include <stdint.h>

#define MSTATUS_MIE (1 << 3)
#define MIE_MEI (1 << 11)
#define MIE_MTI (1 << 7)

inline void global_irq_enable(void) {
    asm volatile (
        "csrs mstatus, 8"
        :
        :
        : "memory"
    );
}

inline void global_irq_disable(void) {
    asm volatile (
        "csrc mstatus, 8"
        :
        :
        : "memory"
    );
}

inline uint32_t csr_read_mstatus(void) {
    uint32_t mstatus;
    asm volatile (
        "csrr %0, mstatus"
        : "=r" (mstatus)
        :
        : "memory"
    );
    return mstatus;
}

inline void csr_write_mstatus(uint32_t mstatus) {
    asm volatile (
        "csrw mstatus, %0"
        :
        : "r" (mstatus)
        : "memory"
    );
}

inline uint32_t csr_read_mie(void) {
    uint32_t mie;
    asm volatile (
        "csrr %0, mie"
        : "=r" (mie)
        :
        : "memory"
    );
    return mie;
}

inline void csr_write_mie(uint32_t mie) {
    asm volatile (
        "csrw mie, %0"
        :
        : "r" (mie)
        : "memory"
    );
}

void ecall(void) {
    asm volatile (
        "ecall"
        :
        :
        : "memory"
    );
}

#endif