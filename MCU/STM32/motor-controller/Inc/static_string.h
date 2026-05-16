#pragma once

#include <stddef.h>

struct StaticString {
  const char *const data;
  const size_t length;
};

#define STATIC_STRING(s) \
  StaticString { (s), sizeof(s) - 1U }
