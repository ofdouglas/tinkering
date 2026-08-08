#include "hdlc/hdlc.h"
#include "hdlc/protocol.h"
#include "hdlc/protocol_handler.h"

#include <array>
#include <cstring>
#include <gtest/gtest.h>

namespace {

size_t stuffedPayloadLength(util::Span<const uint8_t> payload) {
    size_t length = 0U;
    uint8_t out[2];
    for (size_t i = 0U; i < payload.size(); i++) {
        length += hdlc::byteStuff(payload[i], util::Span<uint8_t>(out, 2U));
    }
    return length;
}

size_t encodedFrameLength(util::Span<const uint8_t> payload) {
    return 1U + stuffedPayloadLength(payload) + 1U;
}

void roundTrip(util::Span<const uint8_t> payload) {
    uint8_t frame[64];
    uint8_t decoded[64];
    const size_t expected_frame_len = encodedFrameLength(payload);
    ASSERT_LE(expected_frame_len, sizeof(frame));
    util::Span<uint8_t> frame_span(frame, sizeof(frame));
    ASSERT_TRUE(hdlc::encodeFrame(payload, frame_span));
    EXPECT_EQ(frame[0], hdlc::Flag::kFlag);
    EXPECT_EQ(frame[expected_frame_len - 1U], hdlc::Flag::kFlag);
    const util::Span<const uint8_t> stuffed_with_close(
        frame + 1U, expected_frame_len - 1U);
    std::memset(decoded, 0, sizeof(decoded));
    util::Span<uint8_t> decoded_span(decoded, sizeof(decoded));
    ASSERT_TRUE(hdlc::decodePayload(stuffed_with_close, decoded_span));
    EXPECT_EQ(0, std::memcmp(payload.data(), decoded, payload.size()));
}

class BootloaderCmdRxHandler : public hdlc::BootloaderMessageProtocolRxHandler {
public:
    bool receive(util::Span<const uint8_t> payload) override {
        if (payload.size() != 8U) {
            return false;
        }
        std::memcpy(stored_.data(), payload.data(), payload.size());
        receive_called_ = true;
        return true;
    }
    bool receive_called() const { return receive_called_; }
    util::Span<const uint8_t> stored_payload() const {
        return util::Span<const uint8_t>(stored_.data(), stored_.size());
    }

private:
    std::array<uint8_t, 8U> stored_{};
    bool receive_called_{false};
};

} // namespace

TEST(HdlcByteStuff, PlainByte) {
    uint8_t out[2];
    const size_t len = hdlc::byteStuff(0x10U, util::Span<uint8_t>(out, 2U));
    ASSERT_EQ(len, 1U);
    EXPECT_EQ(out[0], 0x10U);
}

TEST(HdlcByteStuff, EscapesFlag) {
    uint8_t out[2];
    const size_t len = hdlc::byteStuff(hdlc::Flag::kFlag, util::Span<uint8_t>(out, 2U));
    ASSERT_EQ(len, 2U);
    EXPECT_EQ(out[0], hdlc::Flag::kEscape);
    EXPECT_EQ(out[1], 0x5EU);
}

TEST(HdlcByteStuff, EscapesEscape) {
    uint8_t out[2];
    const size_t len = hdlc::byteStuff(hdlc::Flag::kEscape, util::Span<uint8_t>(out, 2U));
    ASSERT_EQ(len, 2U);
    EXPECT_EQ(out[0], hdlc::Flag::kEscape);
    EXPECT_EQ(out[1], 0x5DU);
}

TEST(HdlcRoundTrip, PlainPayload) {
    static const uint8_t kPlain[] = {0x01U, 0x02U, 0x03U};
    roundTrip(kPlain);
}

TEST(HdlcRoundTrip, StuffedPayload) {
    static const uint8_t kStuffed[] = {
        0x10U, 0x23U, 0x45U, 0x7EU, 0x00U, 0x7DU, 0x7DU, 0x10U, 0x08U};
    roundTrip(kStuffed);
}

TEST(HdlcEncode, RejectsUndersizedBuffer) {
    static const uint8_t payload[] = {hdlc::Flag::kFlag};
    uint8_t frame[2];
    EXPECT_FALSE(hdlc::encodeFrame(payload, util::Span<uint8_t>(frame, sizeof(frame))));
}

TEST(HdlcReceiver, PartialInput) {
    static const uint8_t kPayload[] = {0x10U, 0x23U, 0x45U, 0x7EU, 0x00U};
    uint8_t frame[32];
    const util::Span<const uint8_t> payload_span(kPayload);
    const size_t frame_len = encodedFrameLength(payload_span);
    util::Span<uint8_t> frame_span(frame, sizeof(frame));
    ASSERT_TRUE(hdlc::encodeFrame(payload_span, frame_span));
    hdlc::Receiver<32> receiver;
    uint8_t decoded[32];
    ASSERT_TRUE(receiver.enqueue(util::Span<const uint8_t>(frame, 1U)));
    EXPECT_FALSE(receiver.process());
    ASSERT_TRUE(receiver.enqueue(util::Span<const uint8_t>(frame + 1U, frame_len - 1U)));
    ASSERT_TRUE(receiver.process());
    EXPECT_EQ(receiver.receivePayload(util::Span<uint8_t>(decoded, sizeof(decoded))),
              sizeof(kPayload));
    EXPECT_EQ(0, std::memcmp(kPayload, decoded, sizeof(kPayload)));
}

TEST(HdlcRxRouter, DispatchesBootloaderCommand) {
    BootloaderCmdRxHandler cmd_handler;
    hdlc::ProtocolRxHandlerInterface* handlers[] = {&cmd_handler};
    hdlc::RxRouter router{util::Span<hdlc::ProtocolRxHandlerInterface*>(handlers)};
    static const uint8_t kCmdPayload[] = {
        0x01U, 0x02U, 0x03U, 0x04U, 0x05U, 0x06U, 0x07U, 0x08U};
    uint8_t routed_frame[1U + sizeof(kCmdPayload)];
    routed_frame[0U] = static_cast<uint8_t>(hdlc::ServiceType::kBootloaderCommand);
    std::memcpy(routed_frame + 1U, kCmdPayload, sizeof(kCmdPayload));
    ASSERT_TRUE(router.route(routed_frame));
    EXPECT_TRUE(cmd_handler.receive_called());
    EXPECT_EQ(0, std::memcmp(
        cmd_handler.stored_payload().data(), kCmdPayload, sizeof(kCmdPayload)));
}

TEST(HdlcRxRouter, RejectsInvalidFrames) {
    BootloaderCmdRxHandler cmd_handler;
    hdlc::ProtocolRxHandlerInterface* handlers[] = {&cmd_handler};
    hdlc::RxRouter router{util::Span<hdlc::ProtocolRxHandlerInterface*>(handlers)};
    static const uint8_t kShortFrame[] = {
        static_cast<uint8_t>(hdlc::ServiceType::kBootloaderCommand), 0xAAU};
    EXPECT_FALSE(router.route(kShortFrame));
    static const uint8_t kUnknownType[] = {
        static_cast<uint8_t>(hdlc::ServiceType::kBootloaderSegment), 0x00U};
    EXPECT_FALSE(router.route(kUnknownType));
}
