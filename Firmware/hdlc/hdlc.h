#ifndef HDLC_H
#define HDLC_H

#include <cstdint>
#include <cstddef>
#include <array>
#include <cstring>

#include "data_structures/ring_buffer.h"
#include "data_structures/span.h"

namespace Hdlc {

    struct HdlcFlag {
        static constexpr uint8_t kFlag     = 0x7E;
        static constexpr uint8_t kEscape   = 0x7D;
        static constexpr uint8_t kXorValue = 0x20;
    };

    size_t byteStuff(uint8_t byte, Span<uint8_t> out);
    bool encodeFrame(Span<const uint8_t> payload, Span<uint8_t> frame);
    bool decodePayload(Span<const uint8_t> input, Span<uint8_t> output);

    class ReceiverInterface {
    public:
        virtual ~ReceiverInterface() = default;
        virtual void reset() = 0;
        virtual bool enqueue(Span<const uint8_t> data) = 0;
        virtual bool process() = 0;
        virtual size_t receivePayload(Span<uint8_t> output) = 0;
    };

    template <size_t PayloadBufferSize, size_t InputRingCapacity = PayloadBufferSize * 2U + 8U>
    class Receiver : public ReceiverInterface {
    private:
        enum class State : uint8_t {
            IDLE,
            FRAME_DELIMETER,
            DATA,
            ESCAPE
        };

    public:
        Receiver() = default;

        void reset() override {
            state_ = State::IDLE;
            payload_index_ = 0U;
            input_buffer_.clear();
        }

        bool enqueue(Span<const uint8_t> data) override {
            for (size_t i = 0U; i < data.size(); i++) {
                if (!input_buffer_.enqueue(data[i])) {
                    return false;
                }
            }
            return true;
        }

        bool process(Span<const uint8_t> data) {
            if (!enqueue(data)) {
                return false;
            }
            return process();
        }

        bool process() override {
            while (!input_buffer_.isEmpty()) {
                uint8_t data{0U};
                if (!input_buffer_.dequeue(data)) {
                    return false;
                }
                switch (state_) {
                    case State::IDLE:
                        if (data == HdlcFlag::kFlag) {
                            state_ = State::FRAME_DELIMETER;
                        }
                        break;

                    case State::FRAME_DELIMETER:
                        if (data == HdlcFlag::kFlag) {
                            payload_index_ = 0U;
                        } else if (data == HdlcFlag::kEscape) {
                            state_ = State::ESCAPE;
                        } else if (payload_index_ >= PayloadBufferSize) {
                            reset();
                            return false;
                        } else {
                            payload_buffer_[payload_index_++] = data;
                            state_ = State::DATA;
                        }
                        break;

                    case State::ESCAPE:
                        if (payload_index_ >= PayloadBufferSize) {
                            reset();
                            return false;
                        }
                        payload_buffer_[payload_index_++] = static_cast<uint8_t>(data ^ HdlcFlag::kXorValue);
                        state_ = State::DATA;
                        break;

                    case State::DATA:
                        if (data == HdlcFlag::kFlag) {
                            state_ = State::FRAME_DELIMETER;
                            return true;
                        } else if (data == HdlcFlag::kEscape) {
                            state_ = State::ESCAPE;
                        } else if (payload_index_ >= PayloadBufferSize) {
                            reset();
                            return false;
                        } else {
                            payload_buffer_[payload_index_++] = data;
                        }
                        break;

                    default:
                        reset();
                        return false;
                }
            }
            return false;
        }

        size_t receivePayload(Span<uint8_t> output) override {
            if (state_ != State::FRAME_DELIMETER) {
                return 0U;
            }
            if ((payload_index_ == 0U) || (payload_index_ > output.size())) {
                return 0U;
            }
            std::memcpy(output.data(), payload_buffer_.data(), payload_index_);
            const size_t result = payload_index_;
            payload_index_ = 0U;
            state_ = State::IDLE;
            return result;
        }

    private:
        RingBuffer<uint8_t, InputRingCapacity> input_buffer_;
        std::array<uint8_t, PayloadBufferSize> payload_buffer_;
        size_t payload_index_{0U};
        State state_{State::IDLE};
    };

} // namespace Hdlc
#endif // HDLC_H
