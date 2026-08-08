#pragma once

#include "bootloader/transport/transport_interface.h"
#include "hal/reset_interface.h"
#include "interfaces/memory_interface.h"

#include <array>
#include <variant>
#include <vector>

namespace bootloader::test {

class MockTransport : public TransportInterface {
public:
    bool sendCommand(const CommandVariant& command) noexcept override {
        if (std::holds_alternative<GeneralCommand>(command)) {
            const std::optional<CommandPacket> encoded = std::get<GeneralCommand>(command).encode();
            if (!encoded) {
                return false;
            }
            sent_commands_.push_back(*encoded);
            return send_result_;
        }
        if (std::holds_alternative<MemoryCommand>(command)) {
            const std::optional<CommandPacket> encoded = std::get<MemoryCommand>(command).encode();
            if (!encoded) {
                return false;
            }
            sent_commands_.push_back(*encoded);
            return send_result_;
        }
        return false;
    }

    void receive() noexcept override {}

    bool setCommandReceivedCallback(CommandReceivedCallback callback) noexcept override {
        command_callback_ = callback;
        return true;
    }

    bool setSegmentReceivedCallback(SegmentReceivedCallback callback) noexcept override {
        segment_callback_ = callback;
        return true;
    }

    void deliverCommand(const CommandPacket& command) noexcept {
        if (command_callback_) {
            command_callback_(command);
        }
    }

    void setSendResult(bool result) noexcept { send_result_ = result; }

    const std::vector<CommandPacket>& sentCommands() const noexcept { return sent_commands_; }

private:
    bool send_result_{true};
    CommandReceivedCallback command_callback_{};
    SegmentReceivedCallback segment_callback_{};
    std::vector<CommandPacket> sent_commands_{};
};

class MockReset : public hal::ResetInterface {
public:
    void reset() noexcept override { reset_count_++; }

    uint32_t resetCount() const noexcept { return reset_count_; }

private:
    uint32_t reset_count_{0U};
};

class MockMemory : public Memory::MemoryInterface {
public:
    bool read(uint32_t address, uint8_t* data, size_t size) override {
        (void)address;
        (void)data;
        (void)size;
        return false;
    }

    bool write(uint32_t address, const uint8_t* data, size_t size) override {
        (void)address;
        (void)data;
        (void)size;
        return write_result_;
    }

    bool erase(uint32_t address, size_t size) override {
        (void)address;
        (void)size;
        return false;
    }

    void setWriteResult(bool result) noexcept { write_result_ = result; }

private:
    bool write_result_{true};
};

} // namespace bootloader::test
