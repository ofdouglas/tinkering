/**
  ******************************************************************************
  * @file    log.hpp
  * @brief   C++ logging API
  ******************************************************************************
  */

#pragma once

#include "static_string.h"

#include <cstdint>
#include <cstdio>
#include <limits>
#include <type_traits>

namespace logging {

enum class Level : uint8_t {
    Info,
    Warn,
    Error,
    Fatal,
};

void write(const char* file, uint32_t line, Level level, StaticString<> message);
[[noreturn]] void fatal_at(const char* file, uint32_t line, StaticString<> message);

class LocalLogger {
public:
    LocalLogger(const char* file, uint32_t line, Level level)
        : file_(file), line_(line), level_(level) {}

    ~LocalLogger() {
        if (level_ == Level::Fatal) {
            fatal_at(file_, line_, buffer_);
        } else {
            write(file_, line_, level_, buffer_);
        }
    }

    LocalLogger(const LocalLogger&) = delete;
    LocalLogger& operator=(const LocalLogger&) = delete;
    LocalLogger(LocalLogger&&) = delete;
    LocalLogger& operator=(LocalLogger&&) = delete;

    LocalLogger& operator<<(const char* str) {
        buffer_.append(str);
        return *this;
    }

    template <size_t M>
    LocalLogger& operator<<(const StaticString<M>& str) {
        buffer_.append(str.data(), str.length());
        return *this;
    }

    LocalLogger& operator<<(int32_t value) {
        return appendValue("%ld", value);
    }

    LocalLogger& operator<<(uint32_t value) {
        return appendValue("%lu", value);
    }

    LocalLogger& operator<<(void* value) {
        return appendValue("0x%lX", value);
    }

    // Not supported with current newlib configuration
    // LocalLogger& operator<<(float value) {
    //     return appendValue("%f", value);
    // }

    LocalLogger& operator<<(bool value) {
        buffer_.append(value ? "true" : "false");
        return *this;
    }

    LocalLogger& operator<<(char c) {
        buffer_.append(c);
        return *this;
    }

private:
    template <typename T>
    LocalLogger& appendValue(const char* format, T value) {
        char number[20U]{};
        (void)std::snprintf(number, sizeof(number), format, value);
        buffer_.append(number);
        return *this;
    }

    const char* file_{nullptr};
    const uint32_t line_{0U};
    Level level_{Level::Info};
    StaticString<> buffer_{};
};

} /* namespace logging */

#define LOG_INFO() (::logging::LocalLogger(__FILE__, __LINE__, ::logging::Level::Info))
#define LOG_WARN() (::logging::LocalLogger(__FILE__, __LINE__, ::logging::Level::Warn))
#define LOG_ERROR() (::logging::LocalLogger(__FILE__, __LINE__, ::logging::Level::Error))
#define LOG_FATAL() (::logging::LocalLogger(__FILE__, __LINE__, ::logging::Level::Fatal))

#define LOG_INFO_MSG(msg) (::logging::write(__FILE__, __LINE__, ::logging::Level::Info, (msg)))
#define LOG_WARN_MSG(msg) (::logging::write(__FILE__, __LINE__, ::logging::Level::Warn, (msg)))
#define LOG_ERROR_MSG(msg) (::logging::write(__FILE__, __LINE__, ::logging::Level::Error, (msg)))
#define LOG_FATAL_MSG(msg) (::logging::fatal_at(__FILE__, __LINE__, (msg)))
