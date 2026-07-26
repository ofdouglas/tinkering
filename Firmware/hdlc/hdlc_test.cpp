#include "hdlc/hdlc.h"
#include "hdlc/protocol.h"
#include "can/can_frame.h"

#include <array>
#include <cstring>
#include <gtest/gtest.h>

namespace {

size_t stuffedPayloadLength(Span<const uint8_t> payload) {
    size_t length = 0U;
    uint8_t out[2];
    for (size_t i = 0U; i < payload.size(); i++) {
        length += Hdlc::byteStuff(payload[i], Span<uint8_t>(out, 2U));
    }
    return length;
}

size_t encodedFrameLength(Span<const uint8_t> payload) {
    return 1U + stuffedPayloadLength(payload) + 1U;
}

void roundTrip(Span<const uint8_t> payload) {
    uint8_t frame[64];
    uint8_t decoded[64];
    const size_t expected_frame_len = encodedFrameLength(payload);
    ASSERT_LE(expected_frame_len, sizeof(frame));
    Span<uint8_t> frame_span(frame, sizeof(frame));
    ASSERT_TRUE(Hdlc::encodeFrame(payload, frame_span));
    EXPECT_EQ(frame[0], Hdlc::HdlcFlag::kFlag);
    EXPECT_EQ(frame[expected_frame_len - 1U], Hdlc::HdlcFlag::kFlag);
    const Span<const uint8_t> stuffed_with_close(
        frame + 1U, expected_frame_len - 1U);
    std::memset(decoded, 0, sizeof(decoded));
    Span<uint8_t> decoded_span(decoded, sizeof(decoded));
    ASSERT_TRUE(Hdlc::decodePayload(stuffed_with_close, decoded_span));
    EXPECT_EQ(0, std::memcmp(payload.data(), decoded, payload.size()));
}

class VirtualCanRxHandler : public Hdlc::VirtualCanProtocolRxHandler {
public:
    bool receive(Span<const uint8_t> payload) override {
        if (payload.size() != 8U) {
            return false;
        }
        std::memcpy(stored_.data(), payload.data(), payload.size());
        receive_called_ = true;
        return true;
    }
    bool receive_called() const { return receive_called_; }
    Span<const uint8_t> stored_payload() const {
        return Span<const uint8_t>(stored_.data(), stored_.size());
    }

private:
    std::array<uint8_t, 8U> stored_{};
    bool receive_called_{false};
};

} // namespace

TEST(HdlcByteStuff, PlainByte) {
    uint8_t out[2];
    const size_t len = Hdlc::byteStuff(0x10U, Span<uint8_t>(out, 2U));
    ASSERT_EQ(len, 1U);
    EXPECT_EQ(out[0], 0x10U);
}

TEST(HdlcByteStuff, EscapesFlag) {
    uint8_t out[2];
    const size_t len = Hdlc::byteStuff(Hdlc::HdlcFlag::kFlag, Span<uint8_t>(out, 2U));
    ASSERT_EQ(len, 2U);
    EXPECT_EQ(out[0], Hdlc::HdlcFlag::kEscape);
    EXPECT_EQ(out[1], 0x5EU);
}

TEST(HdlcByteStuff, EscapesEscape) {
    uint8_t out[2];
    const size_t len = Hdlc::byteStuff(Hdlc::HdlcFlag::kEscape, Span<uint8_t>(out, 2U));
    ASSERT_EQ(len, 2U);
    EXPECT_EQ(out[0], Hdlc::HdlcFlag::kEscape);
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
    static const uint8_t payload[] = {Hdlc::HdlcFlag::kFlag};
    uint8_t frame[2];
    EXPECT_FALSE(Hdlc::encodeFrame(payload, Span<uint8_t>(frame, sizeof(frame))));
}

TEST(HdlcReceiver, PartialInput) {
    static const uint8_t kPayload[] = {0x10U, 0x23U, 0x45U, 0x7EU, 0x00U};
    uint8_t frame[32];
    const Span<const uint8_t> payload_span(kPayload);
    const size_t frame_len = encodedFrameLength(payload_span);
    Span<uint8_t> frame_span(frame, sizeof(frame));
    ASSERT_TRUE(Hdlc::encodeFrame(payload_span, frame_span));
    Hdlc::Receiver<32> receiver;
    uint8_t decoded[32];
    ASSERT_TRUE(receiver.enqueue(Span<const uint8_t>(frame, 1U)));
    EXPECT_FALSE(receiver.process());
    ASSERT_TRUE(receiver.enqueue(Span<const uint8_t>(frame + 1U, frame_len - 1U)));
    ASSERT_TRUE(receiver.process());
    EXPECT_EQ(receiver.receivePayload(Span<uint8_t>(decoded, sizeof(decoded))),
              sizeof(kPayload));
    EXPECT_EQ(0, std::memcmp(kPayload, decoded, sizeof(kPayload)));
}

TEST(HdlcCanFrame, RoundTrip) {
    using CanId = Can::CanId<Can::Rv32SocCanId>;
    Can::CanFrame<Can::Rv32SocCanId> frame(
        CanId(Can::Rv32SocCanId::kBootloaderCommand));
    static const uint8_t kCanData[] = {
        0x01U, 0x02U, 0x03U, 0x04U, 0x05U, 0x06U, 0x07U, 0x08U};
    ASSERT_TRUE(frame.setData(kCanData));
    frame.setCrc(0xDEADBEEFU);
    uint8_t payload[Can::CanFrame<Can::Rv32SocCanId>::kHdlcPayloadSize];
    const uint8_t protocol = static_cast<uint8_t>(Hdlc::ProtocolType::VIRTUAL_CAN);
    Span<uint8_t> payload_span(payload, sizeof(payload));
    const size_t payload_len = frame.storeHdlcPayload(protocol, payload_span);
    ASSERT_EQ(payload_len, Can::CanFrame<Can::Rv32SocCanId>::kHdlcPayloadSize);
    uint8_t hdlc_frame[64];
    Span<uint8_t> hdlc_frame_span(hdlc_frame, sizeof(hdlc_frame));
    ASSERT_TRUE(Hdlc::encodeFrame(Span<const uint8_t>(payload, payload_len), hdlc_frame_span));
    const size_t hdlc_len = encodedFrameLength(Span<const uint8_t>(payload, payload_len));
    uint8_t stuffed_payload[64];
    std::memset(stuffed_payload, 0, sizeof(stuffed_payload));
    Span<uint8_t> stuffed_span(stuffed_payload, sizeof(stuffed_payload));
    ASSERT_TRUE(Hdlc::decodePayload(
        Span<const uint8_t>(hdlc_frame + 1U, hdlc_len - 1U), stuffed_span));
    Can::CanFrame<Can::Rv32SocCanId> decoded;
    ASSERT_TRUE(decoded.loadHdlcPayload(
        protocol, Span<const uint8_t>(stuffed_payload, payload_len)));
    EXPECT_EQ(decoded.can_id().raw_id(), frame.can_id().raw_id());
    EXPECT_EQ(decoded.data_len(), frame.data_len());
    EXPECT_EQ(decoded.crc(), frame.crc());
    EXPECT_EQ(0, std::memcmp(decoded.data().data(), frame.data().data(), frame.data_len()));
    Hdlc::Receiver<64> receiver;
    ASSERT_TRUE(receiver.enqueue(Span<const uint8_t>(hdlc_frame, hdlc_len)));
    ASSERT_TRUE(receiver.process());
    uint8_t received_payload[64];
    const size_t received_len = receiver.receivePayload(
        Span<uint8_t>(received_payload, sizeof(received_payload)));
    ASSERT_EQ(received_len, payload_len);
    Can::CanFrame<Can::Rv32SocCanId> received;
    ASSERT_TRUE(received.loadHdlcPayload(
        protocol, Span<const uint8_t>(received_payload, payload_len)));
    EXPECT_EQ(received.can_id().enum_id(), Can::Rv32SocCanId::kBootloaderCommand);
    EXPECT_EQ(0, std::memcmp(received.data().data(), kCanData, sizeof(kCanData)));
}

TEST(HdlcRxRouter, DispatchesVirtualCan) {
    VirtualCanRxHandler can_handler;
    Hdlc::ProtocolRxHandlerInterface* handlers[] = {&can_handler};
    Hdlc::RxRouter router{Span<Hdlc::ProtocolRxHandlerInterface*>(handlers)};
    static const uint8_t kCanPayload[] = {
        0x01U, 0x02U, 0x03U, 0x04U, 0x05U, 0x06U, 0x07U, 0x08U};
    uint8_t routed_frame[1U + sizeof(kCanPayload)];
    routed_frame[0U] = static_cast<uint8_t>(Hdlc::ProtocolType::VIRTUAL_CAN);
    std::memcpy(routed_frame + 1U, kCanPayload, sizeof(kCanPayload));
    ASSERT_TRUE(router.route(routed_frame));
    EXPECT_TRUE(can_handler.receive_called());
    EXPECT_EQ(0, std::memcmp(
        can_handler.stored_payload().data(), kCanPayload, sizeof(kCanPayload)));
}

TEST(HdlcRxRouter, RejectsInvalidFrames) {
    VirtualCanRxHandler can_handler;
    Hdlc::ProtocolRxHandlerInterface* handlers[] = {&can_handler};
    Hdlc::RxRouter router{Span<Hdlc::ProtocolRxHandlerInterface*>(handlers)};
    static const uint8_t kShortFrame[] = {
        static_cast<uint8_t>(Hdlc::ProtocolType::VIRTUAL_CAN), 0xAAU};
    EXPECT_FALSE(router.route(kShortFrame));
    static const uint8_t kUnknownType[] = {
        static_cast<uint8_t>(Hdlc::ProtocolType::MEM_TRANSFER), 0x00U};
    EXPECT_FALSE(router.route(kUnknownType));
}
