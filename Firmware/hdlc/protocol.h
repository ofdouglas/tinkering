#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>
#include <type_traits>

#include "can/can_frame.h"
#include "hdlc/hdlc.h"
#include "data_structures/span.h"

namespace Hdlc {
/**
  * Protocol Types: The first byte of the HDLC frame is the protocol type.
  */
enum class ProtocolType : uint8_t {
    ASCII_TEXT     = 0xF0, // Raw text stream
    VIRTUAL_CAN    = 0xE1, // Virtual CAN bus: {uint16_t id, uint8_t data[8], uint32_t crc} (fixed 8 byte payload size)
    MEM_TRANSFER   = 0xD2  // Memory transfer: {uint32_t address, uint16_t size, uint8_t data[64], uint32_t crc}
};

struct ProtocolDescriptor {
  ProtocolType protocolType;
  size_t minPayloadSize;
  size_t maxPayloadSize;
};

class ProtocolRxHandlerInterface {
public:
  virtual ~ProtocolRxHandlerInterface() = default;

  virtual ProtocolDescriptor protocolDescriptor() const = 0;

  virtual bool receive(Span<const uint8_t> payload) = 0;
};

class VirtualCanProtocolRxHandler : public ProtocolRxHandlerInterface {
public:
  static constexpr ProtocolDescriptor descriptor {ProtocolType::VIRTUAL_CAN, 8U, 8U};

  VirtualCanProtocolRxHandler() = default;
  ~VirtualCanProtocolRxHandler() override = default;

  ProtocolDescriptor protocolDescriptor() const override {
    return descriptor;
  }
};

class AsciiTextProtocolRxHandler : public ProtocolRxHandlerInterface {
public:
  static constexpr ProtocolDescriptor descriptor {ProtocolType::ASCII_TEXT, 0U, 64U};

  AsciiTextProtocolRxHandler() = default;
  ~AsciiTextProtocolRxHandler() override = default;

  ProtocolDescriptor protocolDescriptor() const override {
    return descriptor;
  }
};

class RxRouter {
public:
    RxRouter(Span<ProtocolRxHandlerInterface*> handlers) : handlers_(handlers) {}
    ~RxRouter() = default;

    bool route(Span<const uint8_t> frame) {
      if (frame.size() < 2U) {
        return false; // TODO: log error
      }
      const ProtocolType type = static_cast<ProtocolType>(frame[0U]);
      const Span<const uint8_t> payload = frame.subspan(1U, frame.size() - 1U);

      for (auto handler : handlers_) {
        if (handler->protocolDescriptor().protocolType != type) {
          continue;
        }
        if (payload.size() < handler->protocolDescriptor().minPayloadSize || 
            payload.size() > handler->protocolDescriptor().maxPayloadSize) {
          return false; // TODO: log error
        }
        return handler->receive(payload);
      }
      return false; // TODO: log error
    }

private:
  Span<ProtocolRxHandlerInterface*> handlers_;
};


} // namespace Hdlc
#endif // PROTOCOL_H