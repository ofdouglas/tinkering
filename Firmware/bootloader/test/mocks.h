#pragma once

#include "bootloader/transport/transport_interface.h"
#include "hal/reset_interface.h"
#include "interfaces/memory_interface.h"

#include <vector>

namespace Bootloader::test {

class MockTransport : public TransportInterface {
public:
    bool sendMessage(const Message& message) noexcept override {
        sent_messages_.push_back(message);
        return send_result_;
    }

    void receive() noexcept override {}

    bool setMessageReceivedCallback(MessageReceivedCallback callback) noexcept override {
        message_callback_ = callback;
        return true;
    }

    bool setSegmentReceivedCallback(SegmentReceivedCallback callback) noexcept override {
        segment_callback_ = callback;
        return true;
    }

    void setSendResult(bool result) noexcept { send_result_ = result; }

    const std::vector<Message>& sentMessages() const noexcept { return sent_messages_; }

private:
    bool send_result_{true};
    MessageReceivedCallback message_callback_{};
    SegmentReceivedCallback segment_callback_{};
    std::vector<Message> sent_messages_{};
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

} // namespace Bootloader::test
