#pragma once

#include <stdint.h>
#include <variant>
 
using UintVariant = std::variant<uint8_t, uint16_t, uint32_t, uint64_t>;
 
// TODO: add bounded integer types (eg. Uint24_s)
 
 