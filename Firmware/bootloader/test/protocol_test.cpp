#include "bootloader/protocol.h"
#include "bootloader/data_identifiers.h"

#include <gtest/gtest.h>

#include <array>
#include <cstring>

using bootloader::CommandPacket;
using bootloader::CommandType;
using bootloader::GeneralCommand;
using bootloader::MemoryCommand;
using bootloader::data_id::DataIdentifier;

TEST(ProtocolV2, GeneralCommandRoundTrip) {
    const GeneralCommand original{
        .command_type = CommandType::kReadDataIdentifier,
        .value16 = static_cast<uint16_t>(DataIdentifier::kBootloaderInfo1),
        .value32 = 0x12345678U,
    };

    const std::optional<CommandPacket> encoded = original.encode();
    ASSERT_TRUE(encoded.has_value());
    const std::optional<GeneralCommand> decoded = GeneralCommand::decode(*encoded);
    ASSERT_TRUE(decoded.has_value());
    EXPECT_EQ(decoded->command_type, original.command_type);
    EXPECT_EQ(decoded->value16, original.value16);
    EXPECT_EQ(decoded->value32, original.value32);
}

TEST(ProtocolV2, MemoryCommandRoundTrip) {
    const MemoryCommand original{
        .command_type = CommandType::kPrepareAppDownload,
        .size_words = {.value = 0x100U},
        .start_word = 0x020100U,
    };

    const std::optional<CommandPacket> encoded = original.encode();
    ASSERT_TRUE(encoded.has_value());
    const std::optional<MemoryCommand> decoded = MemoryCommand::decode(*encoded);
    ASSERT_TRUE(decoded.has_value());
    EXPECT_EQ(decoded->command_type, original.command_type);
    EXPECT_EQ(decoded->size_words.value, original.size_words.value);
    EXPECT_EQ(decoded->start_word, original.start_word);
}

TEST(ProtocolV2, SegmentTransferHeaderDecode) {
    const uint32_t wire =
        (static_cast<uint32_t>(CommandType::kSegmentTransfer) & 0x3FU) | (42U << 8U);

    const std::optional<bootloader::SegmentTransferCommandWord> decoded =
        bootloader::SegmentTransferPacketView::decodeCommandWord(wire);
    ASSERT_TRUE(decoded.has_value());
    EXPECT_EQ(static_cast<CommandType>(decoded->command_type), CommandType::kSegmentTransfer);
    EXPECT_EQ(decoded->segment_number.value, 42U);
}

TEST(ProtocolV2, CommandFailedWireLayout) {
    const GeneralCommand failed{
        .command_type = CommandType::kCommandFailed,
        .value16 = static_cast<uint16_t>(CommandType::kStartAppDownload),
        .value32 = static_cast<uint32_t>(bootloader::ErrorCode::kNotPrepared),
    };
    const std::optional<CommandPacket> encoded = failed.encode();
    ASSERT_TRUE(encoded.has_value());
    EXPECT_EQ(encoded->bytes[0U] & 0x3FU, static_cast<uint8_t>(CommandType::kCommandFailed));
}
