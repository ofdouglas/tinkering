#pragma once

#include <functional>

#include "bootloader/protocol.h"

namespace bootloader {

class TransportInterface {
public:
    using CommandReceivedCallback = std::function<void(const CommandPacket& command)>;
    using SegmentReceivedCallback = std::function<void(const SegmentTransferPacketView& segment)>;

    virtual ~TransportInterface() = default;
    virtual bool sendCommand(const CommandVariant& command) noexcept = 0;
    virtual void receive() noexcept = 0;
    virtual bool setCommandReceivedCallback(CommandReceivedCallback callback) noexcept = 0;
    virtual bool setSegmentReceivedCallback(SegmentReceivedCallback callback) noexcept = 0;
};

} // namespace bootloader
