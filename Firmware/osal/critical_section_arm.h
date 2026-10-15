#pragma once
/*
 * @file  critical_section_arm.h
 * @brief ARM implementation of an interrupt-masking RAII critical section.
 */

#include <cstdint>

#include "cmsis_compiler.h"

namespace osal {

/**
 * @brief Disables interrupts for the object's lifetime and restores PRIMASK on destruction.
 *
 * Nested instances are supported because each instance restores the interrupt-mask state
 * that was active when it was constructed.
 */
class RaiiCriticalSection {
public:
    RaiiCriticalSection() noexcept
        : primask_{__get_PRIMASK()} {
        __disable_irq();
    }

    ~RaiiCriticalSection() noexcept {
        __set_PRIMASK(primask_);
    }

    RaiiCriticalSection(const RaiiCriticalSection&) = delete;
    RaiiCriticalSection& operator=(const RaiiCriticalSection&) = delete;
    RaiiCriticalSection(RaiiCriticalSection&&) = delete;
    RaiiCriticalSection& operator=(RaiiCriticalSection&&) = delete;

private:
    const std::uint32_t primask_;
};

} // namespace osal