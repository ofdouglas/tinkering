#pragma once

#include <stdint.h>
#include <stddef.h>

#include "util/span.h"

namespace Stream {

class StreamInterface {
public:
    virtual ~StreamInterface() = default;
    virtual size_t read(util::Span<uint8_t> data) = 0;
    virtual size_t write(util::Span<const uint8_t> data) = 0;
};

} // namespace Stream
