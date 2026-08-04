#include "protocol.h"

namespace Bootloader {

bool isValidMessageType(MessageType message_type) noexcept {
    static_assert(MessageType::kUnknown == 0U, "MessageType::kUnknown must be 0");

    return (message_type != MessageType::kUnknown) && (message_type < MessageType::kNumMessageTypes);
}

PYBIND11_MODULE(protocol, m) {
    m.def("isValidMessageType", &isValidMessageType, "Check if a MessageType is valid");
}

} // namespace Bootloader