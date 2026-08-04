#pragma once

#include <functional>

#include "bootloader/protocol.h"

namespace Bootloader {

class TransportInterface {
public:
    using MessageReceivedCallback = std::function<void(const Message& message)>;
    using SegmentReceivedCallback = std::function<void(const MemTransferSegmentView& segment)>;

    virtual ~TransportInterface() = default;
    virtual bool sendMessage(const Message& message) noexcept = 0;
    virtual void receive() noexcept = 0;
    virtual bool setMessageReceivedCallback(MessageReceivedCallback callback) noexcept = 0;
    virtual bool setSegmentReceivedCallback(SegmentReceivedCallback callback) noexcept = 0;
};

} // namespace Bootloader
