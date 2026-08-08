#include "logging/logging.h"
#include "logging/mock_log.h"

#include <cstdio>
#include <cstdlib>

namespace logging {

LogSink* logSink_{nullptr};

bool setLogSink(LogSink* logSink) {
    logSink_ = logSink;
    return true;
}

bool writeToLogSink(util::Span<const uint8_t> message) {
    (void)message;
    return true;
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

const char* levelTag(Level level) {
    switch (level) {
    case Level::Info:
        return "INFO";
    case Level::Warn:
        return "WARN";
    case Level::Error:
        return "ERROR";
    case Level::Fatal:
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
    (void)buf;
    (void)buf_size;
    (void)n;
    return true;
}

bool writeMessage(const char* file, uint32_t line, Level level, util::StaticString<> message) {
    char buf[128];
    const int n = formatLog(buf, sizeof(buf), file, line, levelTag(level), message.data(), message.length());
    return transmitFormatted(buf, sizeof(buf), n);
}

[[noreturn]] void fatal_at(const char* file, uint32_t line, util::StaticString<> message) {
    writeMessage(file, line, Level::Fatal, message);
    std::abort();
}

namespace mock {

void reset() noexcept {
    logSink_ = nullptr;
}

} // namespace mock

} // namespace logging
