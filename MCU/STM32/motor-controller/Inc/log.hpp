/**
  ******************************************************************************
  * @file    log.hpp
  * @brief   C++ logging API
  ******************************************************************************
  */

#pragma once

#include "static_string.h"

#include <cstdint>

namespace logging {

enum class Level : uint8_t {
    Info,
    Warn,
    Fatal,
};

void write(const char *file, uint32_t line, Level level, StaticString message, uint32_t data = 0U);
[[noreturn]] void fatal_at(const char *file, uint32_t line, StaticString message, uint32_t data = 0U);

inline void info(StaticString message, uint32_t data = 0U) {
    write(__FILE__, __LINE__, Level::Info, message, data);
}

inline void warn(StaticString message, uint32_t data = 0U) {
    write(__FILE__, __LINE__, Level::Warn, message, data);
}

[[noreturn]] inline void fatal(StaticString message, uint32_t data = 0U) {
    fatal_at(__FILE__, __LINE__, message, data);
}

} /* namespace logging */
