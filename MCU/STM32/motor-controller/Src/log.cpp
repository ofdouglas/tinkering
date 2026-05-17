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

const char *fileBasename(const char *path) {
    const char *base = path;
    for (const char *p = path; *p != '\0'; ++p) {
        if (*p == '/' || *p == '\\') {
            base = p + 1;
        }
    }
    return base;
}

const char *levelTag(logging::Level level) {
    switch (level) {
    case logging::Level::Info:
        return "INFO";
    case logging::Level::Warn:
        return "WARN";
    case logging::Level::Fatal:
        return "FATAL";
    }
    return "????";
}

bool uartReady() {
    return bsp.huart1.Instance != nullptr &&
           bsp.huart1.gState != HAL_UART_STATE_RESET;
}

int formatLog(char *buf, size_t buf_size, const char *file, uint32_t line, const char *level,
              const char *msg, size_t msg_len, uint32_t data) {
    return std::snprintf(buf, buf_size, "%s:%lu [%s] %.*s %lu\n", fileBasename(file),
                         static_cast<unsigned long>(line), level,
                         static_cast<int>(msg_len), msg, data);
}

void transmitFormatted(const char *buf, size_t buf_size, int n) {
    if (n <= 0 || !uartReady()) {
        return;
    }

    const int max_len = static_cast<int>(buf_size) - 1;
    const uint16_t len = static_cast<uint16_t>((n < max_len) ? n : max_len);
    (void)bsp.uartTransmit(reinterpret_cast<const uint8_t *>(buf), len, 10000U);
}

void emit(const char *file, uint32_t line, const char *level, StaticString message, uint32_t data) {
    char buf[128];
    const int n = formatLog(buf, sizeof(buf), file, line, level, message.data(), message.length(), data);
    transmitFormatted(buf, sizeof(buf), n);
}

} /* namespace */

namespace logging {

void write(const char *file, uint32_t line, Level level, StaticString message, uint32_t data) {
    emit(file, line, levelTag(level), message, data);
}

[[noreturn]] void fatal_at(const char *file, uint32_t line, StaticString message, uint32_t data) {
    __disable_irq();
    emit(file, line, "FATAL", message, data);

    while (true) {
        bsp.ledToggle();
        for (volatile uint32_t i = 0; i < 500000U; ++i) {
        }
    }
}

} /* namespace logging */

extern "C" void log_fatal_c(const char *file, uint32_t line, const char *msg, size_t msg_len) {
    logging::fatal_at(file, line, StaticString(msg, msg_len));
}
