#pragma once
/*
 * @file  protocol_handler.h
 * @brief HDLC protocol handler definitions.
 *
 * NOTE: this is a work in progress, may be reworked or removed.
 */

#include "hdlc/protocol.h"
#include "util/span.h"

namespace hdlc {

struct ProtocolDescriptor {
    ServiceType service_type;
    size_t min_payload_size;
    size_t max_payload_size;
};

class ProtocolRxHandlerInterface {
public:
    virtual ~ProtocolRxHandlerInterface() = default;

    virtual ProtocolDescriptor protocolDescriptor() const = 0;

    virtual bool receive(util::Span<const uint8_t> payload) = 0;
};

class AsciiTextProtocolRxHandler : public ProtocolRxHandlerInterface {
public:
    static constexpr ProtocolDescriptor descriptor{
        static_cast<ServiceType>(0x12U), 0U, 64U}; // unassigned common ID (placeholder)

    AsciiTextProtocolRxHandler() = default;
    ~AsciiTextProtocolRxHandler() override = default;

    ProtocolDescriptor protocolDescriptor() const override { return descriptor; }

    bool receive(util::Span<const uint8_t> payload) override {
        (void)payload;
        return true;
    }
};

class BootloaderMessageProtocolRxHandler : public ProtocolRxHandlerInterface {
public:
    static constexpr ProtocolDescriptor descriptor{
        ServiceType::kBootloaderCommand, 8U, 8U};

    BootloaderMessageProtocolRxHandler() = default;
    ~BootloaderMessageProtocolRxHandler() override = default;

    ProtocolDescriptor protocolDescriptor() const override { return descriptor; }

    virtual bool receive(util::Span<const uint8_t> payload) override {
        (void)payload;
        return true;
    }
};

class BootloaderSegmentProtocolRxHandler : public ProtocolRxHandlerInterface {
public:
    static constexpr ProtocolDescriptor descriptor{
        ServiceType::kBootloaderSegment, 64U, 64U};

    BootloaderSegmentProtocolRxHandler() = default;
    ~BootloaderSegmentProtocolRxHandler() override = default;

    ProtocolDescriptor protocolDescriptor() const override { return descriptor; }

    bool receive(util::Span<const uint8_t> payload) override {
        (void)payload;
        return true;
    }
};

class RxRouter {
public:
    explicit RxRouter(util::Span<ProtocolRxHandlerInterface*> handlers)
        : handlers_(handlers) {}
    ~RxRouter() = default;

    bool route(util::Span<const uint8_t> frame) {
        if (frame.size() < 2U) {
            return false; // TODO: log error
        }
        const ServiceType type = static_cast<ServiceType>(frame[0U]);
        const util::Span<const uint8_t> payload =
            frame.subspan(1U, frame.size() - 1U);

        for (auto* handler : handlers_) {
            if (handler->protocolDescriptor().service_type != type) {
                continue;
            }
            if (payload.size() < handler->protocolDescriptor().min_payload_size ||
                payload.size() > handler->protocolDescriptor().max_payload_size) {
                return false; // TODO: log error
            }
            return handler->receive(payload);
        }
        return false; // TODO: log error
    }

private:
    util::Span<ProtocolRxHandlerInterface*> handlers_;
};

} // namespace hdlc
