#pragma once

#include <stdint.h>

namespace Memory {

class MemoryInterface {
public:
    virtual ~MemoryInterface() = default;

    virtual bool read(uint32_t address, uint8_t* data, size_t size) = 0;
    virtual bool write(uint32_t address, const uint8_t* data, size_t size) = 0;
    virtual bool erase(uint32_t address, size_t size) = 0;
};    

struct Region {
    enum class Attributes : uint32_t {
        kUnknown  = 0,
        kReadable =    (1 << 0U),
        kWriteable =   (1 << 1U),
        kBootable =    (1 << 2U),
        kIsFlash =     (1 << 3U),
        kIsSram =      (1 << 4U),
    };
    
    uint32_t start_address;
    uint32_t size_bytes;
    uint32_t attributes;
    MemoryInterface* interface;
};

static inline bool isWithinRegion(uint32_t address, uint32_t size, const Region& region) noexcept {
    return (address >= region.start_address) && ((address + size) <= (region.start_address + region.size_bytes));
}

} // namespace Memory
