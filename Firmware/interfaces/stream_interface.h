#pragma once

#include <stdint.h>
#include <stddef.h>

#include "data_structures/span.h"

namespace Stream {

class StreamInterface {
public:
    virtual ~StreamInterface() = default;
    virtual size_t read(Span<uint8_t> data) = 0;
    virtual size_t write(Span<const uint8_t> data) = 0;
};

} // namespace Stream
