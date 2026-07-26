#ifndef MEM_TRANSFER_H
#define MEM_TRANSFER_H

#include <stdint.h>
#include <stddef.h>

#include "data_structures/span.h"

namespace Hdlc {

struct MemTransferPacket {
    uint16_t sequence_number;
    uint16_t data_size;
    uint32_t address;
    uint8_t  data[64];
};

static_assert(sizeof(MemTransferPacket) == 72U, "MemTransferPacket size is incorrect");


} // namespace Hdlc

#endif // MEM_TRANSFER_H