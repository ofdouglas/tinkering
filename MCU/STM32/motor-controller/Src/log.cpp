/**
  ******************************************************************************
  * @file    log.cpp
  * @brief   UART logging implementation
  ******************************************************************************
  */

#include "log.hpp"
#include "log_c.h"
#include "bsp.h"

#include <cstdio>

namespace {

const char* fileBasename(const char* path) {
    const char* base = path;
    for (const char* p = path; *p != '\0'; ++p) {
        if (*p == '/' || *p == '\\') {
            base = p + 1;
        }
    }
    return base;
}

const char* levelTag(logging::Level level) {
    switch (level) {
    case logging::Level::Info:
        return "INFO";
    case logging::Level::Warn:
        return "WARN";
    case logging::Level::Error:
        return "ERROR";
    case logging::Level::Fatal:
        return "FATAL";
    }
    return "????";
}

int formatLog(char* buf, size_t buf_size, const char* file, uint32_t line, const char* level,
              const char* msg, size_t msg_len) {
    return std::snprintf(buf, buf_size, "%s:%lu [%s] %.*s\n", fileBasename(file),
                         static_cast<unsigned long>(line), level,
                         static_cast<int>(msg_len), msg);
}

void transmitFormatted(const char* buf, size_t buf_size, int n) {
    if (n <= 0 || !bsp.uartReady()) {
        return;
    }

    const int max_len = static_cast<int>(buf_size) - 1;
    const uint16_t len = static_cast<uint16_t>((n < max_len) ? n : max_len);
    (void)bsp.uartTransmit(reinterpret_cast<const uint8_t*>(buf), len, 10000U);
}

} // namespace

namespace logging {

void write(const char* file, uint32_t line, Level level, StaticString<> message) {
    char buf[128];
    const int n = formatLog(buf, sizeof(buf), file, line, levelTag(level), message.data(), message.length());
    transmitFormatted(buf, sizeof(buf), n);
}

[[noreturn]] void fatal_at(const char* file, uint32_t line, StaticString<> message) {
    __disable_irq();
    write(file, line, Level::Fatal, message);

    while (true) {
        bsp.ledToggle();
        for (volatile uint32_t i = 0; i < 500000U; ++i) {
        }
    }
}

} // namespace logging

extern "C" void log_fatal_c(const char* file, uint32_t line, const char* msg, size_t msg_len) {
    logging::fatal_at(file, line, StaticString<>(msg, msg_len));
}
