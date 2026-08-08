/**
  ******************************************************************************
  * @file    log.cpp
  * @brief   UART logging implementation
  ******************************************************************************
  */

#include "logging.h"
#include "log_c.h"
#include "bsp/base_bsp.h"

#include <cstdio>

namespace logging {

LogSink* logSink_{nullptr};

bool setLogSink(LogSink* logSink) {
    logSink_ = logSink;
    return true;
}

bool writeToLogSink(util::Span<const uint8_t> message) {
    if (logSink_ == nullptr) {
        return false;
    }
    return logSink_->write(message);
}

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

bool transmitFormatted(const char* buf, size_t buf_size, int n) {
    if (n <= 0 || logSink_ == nullptr) {
        return false;
    }

    const int max_len = static_cast<int>(buf_size) - 1;
    const uint16_t len = static_cast<uint16_t>((n < max_len) ? n : max_len);

    // TODO: log write errors (increment error count, drain later)
    util::Span<const uint8_t> message(reinterpret_cast<const uint8_t*>(buf), len);
    return writeToLogSink(message);
}

bool writeMessage(const char* file, uint32_t line, Level level, util::StaticString<> message) {
    char buf[128];
    const int n = formatLog(buf, sizeof(buf), file, line, levelTag(level), message.data(), message.length());
    return transmitFormatted(buf, sizeof(buf), n);
}

[[noreturn]] void fatal_at(const char* file, uint32_t line, util::StaticString<> message) {
    // TODO
    // __disable_irq();
    writeMessage(file, line, Level::Fatal, message);

    // TODO: call terminate_handler from here (no app-specific code here)
    bool led_on = true;
    while (true) {
        // BaseBsp::setDebugLed(led_on);
        // led_on = !led_on;
        for (volatile uint32_t i = 0; i < 500000U; ++i) {
        }
    }
}

} // namespace logging

extern "C" void log_fatal_c(const char* file, uint32_t line, const char* msg, size_t msg_len) {
    logging::fatal_at(file, line, util::StaticString<>(msg, msg_len));
}
