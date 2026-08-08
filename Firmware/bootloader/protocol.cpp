#include "bootloader/protocol.h"

#include <cstring>

namespace bootloader {

/******************************************************************************
 *  Command Utility Methods
 ******************************************************************************/
constexpr uint8_t kCommandTypeMask{0x3FU};

CommandType decodeCommandByte(uint8_t first_byte) noexcept {
    return static_cast<CommandType>(first_byte & kCommandTypeMask);
}

uint8_t encodeCommandByte(CommandType command_type) noexcept {
    return static_cast<uint8_t>(command_type) & kCommandTypeMask;
}

std::optional<CommandVariant> decodeCommand(const CommandPacket& command) noexcept {
    if (auto general_command = GeneralCommand::decode(command)) {
        return CommandVariant{*general_command};
    }
    if (auto memory_command = MemoryCommand::decode(command)) {
        return CommandVariant{*memory_command};
    }
    return std::nullopt;
}

/******************************************************************************
 *  General Command Methods
 ******************************************************************************/
 bool GeneralCommand::isValid() const noexcept {
    return isValidCommandType(command_type) && isGeneralCommand(command_type);
}

std::optional<CommandPacket> GeneralCommand::encode() const noexcept {
    if (!isValid()) {
        return std::nullopt;
    }

    CommandPacket encoded{};
    encoded.bytes[0U] = encodeCommandByte(command_type);
    encoded.bytes[1U] = 0U;
    std::memcpy(&encoded.bytes[2U], &value16, sizeof(value16));
    std::memcpy(&encoded.bytes[4U], &value32, sizeof(value32));
    return encoded;
}

std::optional<CommandPacket> GeneralCommand::createEncoded(CommandType command_type, uint16_t value16,
                                                           uint32_t value32) noexcept {
    GeneralCommand command{
        .command_type = command_type,
        .value16 = value16,
        .value32 = value32,
    };
    return command.encode();
}

std::optional<GeneralCommand> GeneralCommand::decode(const CommandPacket& command) noexcept {
    const CommandType command_type = decodeCommandByte(command.bytes[0U]);
    if (!isGeneralCommand(command_type)) {
        return std::nullopt;
    }

    GeneralCommand decoded{};
    decoded.command_type = command_type;
    std::memcpy(&decoded.value16, &command.bytes[2U], sizeof(decoded.value16));
    std::memcpy(&decoded.value32, &command.bytes[4U], sizeof(decoded.value32));
    return decoded;
}

/******************************************************************************
 *  Memory Command Methods
 ******************************************************************************/
 bool MemoryCommand::isValid() const noexcept {
    return isValidCommandType(command_type) && isMemoryCommand(command_type) &&
           util::Uint24_s::isValid(size_words.value);
}

std::optional<CommandPacket> MemoryCommand::encode() const noexcept {
    if (!isValid()) {
        return std::nullopt;
    }

    CommandPacket encoded{};
    encoded.bytes[0U] = encodeCommandByte(command_type);
    std::memcpy(&encoded.bytes[1U], &size_words.value, util::Uint24_s::kSizeBytes);
    std::memcpy(&encoded.bytes[4U], &start_word, sizeof(start_word));
    return encoded;
}

std::optional<CommandPacket> MemoryCommand::createEncoded(CommandType command_type, uint32_t size_words,
                                                            uint32_t start_word) noexcept {
    MemoryCommand command{
        .command_type = command_type,
        .start_word = start_word,
    };
    if (!command.size_words.deserialize(size_words)) {
        return std::nullopt;
    }
    return command.encode();
}

std::optional<MemoryCommand> MemoryCommand::decode(const CommandPacket& command) noexcept {
    const CommandType command_type = decodeCommandByte(command.bytes[0U]);
    if (!isMemoryCommand(command_type)) {
        return std::nullopt;
    }

    MemoryCommand decoded{};
    decoded.command_type = command_type;
    uint32_t size_wire{0U};
    std::memcpy(&size_wire, &command.bytes[1U], util::Uint24_s::kSizeBytes);
    if (!decoded.size_words.deserialize(size_wire)) {
        return std::nullopt;
    }
    std::memcpy(&decoded.start_word, &command.bytes[4U], sizeof(decoded.start_word));
    return decoded;
}

/******************************************************************************
 *  Segment Transfer Methods
 ******************************************************************************/
std::optional<SegmentTransferCommandWord> SegmentTransferPacketView::decodeCommandWord(
    uint32_t command_word) noexcept {
    const CommandType command_type = decodeCommandByte(static_cast<uint8_t>(command_word & 0xFFU));
    if (!isSegmentTransferCommand(command_type)) {
        return std::nullopt;
    }

    SegmentTransferCommandWord decoded{};
    decoded.command_type = static_cast<uint8_t>(command_type);
    if (!decoded.segment_number.deserialize(command_word >> 8U)) {
        return std::nullopt;
    }
    return decoded;
}

bool SegmentTransferCommandWord::isValid() const noexcept {
    return isSegmentTransferCommand(static_cast<CommandType>(command_type)) &&
           util::Uint24_s::isValid(segment_number.value);
}

} // namespace bootloader
