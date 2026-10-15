#pragma once
/*
 * @file  critical_section_host.h
 * @brief No-op critical section implementation for single-threaded host tests.
 */

namespace osal {

/**
 * @brief Provides the critical-section API without synchronization for host tests.
 *
 * This implementation is only suitable for single-threaded tests. Host builds must
 * select it explicitly by defining OSAL_CRITICAL_SECTION_HOST.
 */
class RaiiCriticalSection {
public:
    RaiiCriticalSection() noexcept {}
    ~RaiiCriticalSection() noexcept {}

    RaiiCriticalSection(const RaiiCriticalSection&) = delete;
    RaiiCriticalSection& operator=(const RaiiCriticalSection&) = delete;
    RaiiCriticalSection(RaiiCriticalSection&&) = delete;
    RaiiCriticalSection& operator=(RaiiCriticalSection&&) = delete;
};

} // namespace osal
