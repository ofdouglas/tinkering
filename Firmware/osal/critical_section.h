#pragma once
/*
 * @file  critical_section.h
 * @brief Selects the platform implementation of the RAII critical section.
 * 
 * @details Provides an RAII critical section wrapper, which accounts for nesting.
 * This disables *all* interrupts. TODO: add a variant supporting priority masking.
 * 
 * Example usage:
 * {
 *   osal::RaiiCriticalSection cs{};
 *   // code that must be executed atomically
 * }
 */

// TODO: add other architectures eventually
#if defined(OSAL_CRITICAL_SECTION_HOST)
    #include "critical_section_host.h"
#elif defined(__arm__)
    #include "critical_section_arm.h"
#else
    #error "No critical section implementation selected"
#endif
