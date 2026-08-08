
#include <cstdint>
#include <limits>

#include "bootloader/mcu/bootloader.h"
#include "bootloader/data_identifiers.h"
#include "bootloader/image_header.h"

#include "hal/clock.h"
#include "hal/delay.h"
#include "logging/logging.h"

namespace bootloader {

using data_id::AppVersion1;
using data_id::BoardVersion1;
using data_id::BootloaderInfo1;
using data_id::DataIdentifier;
using data_id::ImageStatus1;
using data_id::isValidDataIdentifier;

Bootloader::Bootloader(util::Span<const Memory::Region> regions, TransportInterface& transport,
                       hal::ResetInterface& reset) noexcept
    : regions_(regions),
      transport_(transport),
      reset_(reset) {}


// TODO: validate full application header
bool Bootloader::isValidApplication(const ImageHeader_Application_v1& image_header) const noexcept {
    // Header start
    if (image_header.header_start.magic != HeaderStart::kMagic) {
        return false;
    }
    if (image_header.header_start.image_type != static_cast<uint8_t>(ImageType::kApplication)) {
        return false;
    }
    if (image_header.header_start.header_version != 1U) {
        return false;
    }
    if (image_header.header_start.header_size_bytes != (sizeof(ImageHeader_Application_v1) - sizeof(HeaderStart))) {
        return false;
    }

    // Header CRC
    //    TODO

    // Board type and version
    //    TODO

    // Image CRC
    //    TODO

    return true;
}

bool Bootloader::validateTransfer() const noexcept {
    if (application_ == nullptr) {
        return false;
    }

    if (prepared_.active_region == nullptr) {
        return false;
    }

    const ImageHeader_Application_v1* image_header =
        reinterpret_cast<const ImageHeader_Application_v1*>(prepared_.active_region->start_address);

    return isValidApplication(*image_header);
}

bool Bootloader::sendGeneralCommand(CommandType command_type, uint16_t value16, uint32_t value32) noexcept {
    const GeneralCommand command{
        .command_type = command_type,
        .value16 = value16,
        .value32 = value32,
    };
    if (!command.isValid()) {
        LOG_ERROR() << "Failed to encode general command: " << static_cast<uint8_t>(command_type);
        return false;
    }

    return transport_.sendCommand(CommandVariant{command});
}

void Bootloader::sendCommandSuccess(CommandType original_command) noexcept {
    sendGeneralCommand(CommandType::kCommandSuccess, static_cast<uint16_t>(original_command), 0U);
}

void Bootloader::sendCommandFailed(CommandType original_command, ErrorCode error_code) noexcept {
    sendGeneralCommand(CommandType::kCommandFailed, static_cast<uint16_t>(original_command), static_cast<uint32_t>(error_code));
}

void Bootloader::sendSegmentAck(uint32_t segment_number) noexcept {
    sendGeneralCommand(CommandType::kSegmentAck, 0U, segment_number);
}

void Bootloader::sendSegmentNak(uint32_t segment_number) noexcept {
    sendGeneralCommand(CommandType::kSegmentNak, 0U, segment_number);
}

bool Bootloader::bootApplication() noexcept {
    if (application_ == nullptr) {
        LOG_ERROR() << "No bootable application found";
        return false;
    }

    const ImageHeader_Application_v1* image_header =
        reinterpret_cast<const ImageHeader_Application_v1*>(application_->start_address);

    void (*p_start_function)(void) = reinterpret_cast<void (*)(void)>(image_header->image_entry_address);
    p_start_function();

    // Should not return
    return false;
}

/******************************************************************************
 *  Public API
 ******************************************************************************/

 bool Bootloader::initialize() noexcept {
    transport_.setCommandReceivedCallback([this](const CommandPacket& command) {
        handleCommand(command);
    });
    transport_.setSegmentReceivedCallback([this](const SegmentTransferPacketView& segment) {
        handleSegment(segment);
    });

    return true;
}

bool Bootloader::validateApplication() noexcept {
    for (const auto& region : regions_) {
        if ((region.attributes & static_cast<uint32_t>(Memory::Region::Attributes::kBootable)) != 0U) {
            const ImageHeader_Application_v1* image_header =
                reinterpret_cast<const ImageHeader_Application_v1*>(region.start_address);
            if (isValidApplication(*image_header)) {
                application_ = const_cast<Memory::Region*>(&region);
                return true;
            }
            LOG_ERROR() << "Invalid application header in region: " << region.start_address;
        }
    }

    LOG_ERROR() << "No bootable application found in any region";
    return false;
}

Bootloader::State Bootloader::tick() noexcept {
    const hal::PlatformClock::TimePoint now = hal::PlatformClock::now();

    transport_.receive();

    if (state_ == State::kIdle) {
        if (now > (last_rx_time_ + std::chrono::seconds(10U))) {
            if (!bootApplication()) {
                LOG_ERROR() << "Failed to boot application";
                state_ = State::kFault;
            }
        }
    }

    return state_;
};


/******************************************************************************
 *  Command Dispatch
 ******************************************************************************/

void Bootloader::handleCommand(const CommandPacket& packet) noexcept {
    last_rx_time_ = hal::PlatformClock::now();

    const std::optional<CommandVariant> decoded = decodeCommand(packet);
    if (!decoded.has_value()) {
        LOG_ERROR() << "Invalid command";
        return;
    }

    if (std::holds_alternative<GeneralCommand>(decoded.value())) {
        handleGeneralCommand(std::get<GeneralCommand>(decoded.value()));
    } else if (std::holds_alternative<MemoryCommand>(decoded.value())) {
        handleMemoryCommand(std::get<MemoryCommand>(decoded.value()));
    }
}

void Bootloader::handleGeneralCommand(const GeneralCommand& command) noexcept {
    switch (command.command_type) {
        case CommandType::kReadDataIdentifier:
            handleReadDataIdentifier(command.value16);
            break;
        case CommandType::kWriteDataIdentifier:
            handleWriteDataIdentifier(command.value16, command.value32);
            break;
        case CommandType::kReset:
            handleReset();
            break;
        case CommandType::kBootApplication:
            if (bootApplication()) {
                // TODO: if we want to send a response, we need to set a "boot on next tick() flag"
                // currently, this code will not be reached if the app image is valid
                sendCommandSuccess(CommandType::kBootApplication);
            } else {
                sendCommandFailed(CommandType::kBootApplication, ErrorCode::kInvalidImageFormat);
            }
            break;
        case CommandType::kCommandSuccess:
        case CommandType::kCommandPending:
        case CommandType::kCommandFailed:
        case CommandType::kSegmentAck:
        case CommandType::kSegmentNak:
            LOG_ERROR() << "Unexpected command type from host: " << static_cast<uint8_t>(command.command_type);
            break;
        case CommandType::kSegmentTransfer:
            LOG_ERROR() << "Segment transfer received as command frame; expected segment frame";
            break;
        default:
            LOG_ERROR() << "Unknown command type: " << static_cast<uint8_t>(command.command_type);
            break;
    }
}

void Bootloader::handleMemoryCommand(const MemoryCommand& command) noexcept {
    switch (command.command_type) {
        case CommandType::kPrepareErase:
        case CommandType::kPrepareAppDownload:
            handlePrepareMemory(command);
            break;
        case CommandType::kStartErase:
            handleStartErase(command);
            break;
        case CommandType::kStartAppDownload:
            handleStartAppDownload(command);
            break;
        case CommandType::kEndAppDownload:
            handleEndAppDownload();
            break;
        default:
            sendCommandFailed(command.command_type, ErrorCode::kUnknown);
            break;
    }
}


/******************************************************************************
 *  Data Identifiers
 ******************************************************************************/

// TODO: get bootloader info
void Bootloader::handleReadDataIdentifier(uint16_t data_identifier) noexcept {
    const auto did = static_cast<DataIdentifier>(data_identifier);
    if (!isValidDataIdentifier(did)) {
        sendCommandFailed(CommandType::kReadDataIdentifier, ErrorCode::kInvalidDataIdentifier);
        return;
    }

    uint32_t value32{0U};
    switch (did) {
        case DataIdentifier::kBootloaderInfo1: {
            const BootloaderInfo1 bootloader_info = {
                .magic = BootloaderInfo1::kMagic,
                .protocol_version = kProtocolVersion,
                .bootloader_major = 1,
                .bootloader_minor = 0,
            };
            value32 = *reinterpret_cast<const uint32_t*>(&bootloader_info);
            break;
        }
        // TODO: get image status
        case DataIdentifier::kImageStatus1: {
            const ImageStatus1 image_status = {
                .status_a = static_cast<uint8_t>(ImageStatus::kUnknown),
                .status_b = static_cast<uint8_t>(ImageStatus::kNotSupported),
            };
            value32 = *reinterpret_cast<const uint32_t*>(&image_status);
            break;
        }
        // TODO: get board version
        case DataIdentifier::kBoardVersion1: {
            const BoardVersion1 board_version = {
                .board_type = 1, // TODO
                .board_version_major = 1,
                .board_version_minor = 0,
            };
            value32 = *reinterpret_cast<const uint32_t*>(&board_version);
            break;
        }
        // TODO: get app version
        case DataIdentifier::kAppVersion1: {
            const AppVersion1 app_version = {
                .app_version_major = 1,
                .app_version_minor = 0,
            };
            value32 = *reinterpret_cast<const uint32_t*>(&app_version);
            break;
        }
        default:
            sendCommandFailed(CommandType::kReadDataIdentifier, ErrorCode::kInvalidDataIdentifier);
            return;
    }

    sendGeneralCommand(CommandType::kReadDataIdentifier, data_identifier, value32);
}

void Bootloader::handleWriteDataIdentifier(uint16_t data_identifier, uint32_t value) noexcept {
    (void)value;
    const auto did = static_cast<DataIdentifier>(data_identifier);
    if (!isValidDataIdentifier(did)) {
        sendCommandFailed(CommandType::kWriteDataIdentifier, ErrorCode::kInvalidDataIdentifier);
        return;
    }

    // All current identifiers are read-only on the device.
    sendCommandFailed(CommandType::kWriteDataIdentifier, ErrorCode::kWriteOnlyDataIdentifier);
}

/******************************************************************************
 *  Memory Commands
 ******************************************************************************/

// TODO: handle start addresses that aren't at the start of a region
void Bootloader::handlePrepareMemory(const MemoryCommand& command) noexcept {
    const uint32_t start_address = command.start_word * 4U;
    const uint32_t size_bytes = command.size_words.value * 4U;

    prepared_.clear();
    prepared_.command_type = command.command_type;
    prepared_.start_word = command.start_word;
    prepared_.size_words = command.size_words.value;
    prepared_.active_region = regionFromAddress(start_address);

    if (prepared_.active_region == nullptr) {
        sendCommandFailed(command.command_type, ErrorCode::kInvalidMemAddress);
        prepared_.clear();
        LOG_ERROR() << "No memory interface found for start address: " << start_address;
        return;
    }

    if (!Memory::isWithinRegion(start_address, size_bytes, *prepared_.active_region)) {
        sendCommandFailed(command.command_type, ErrorCode::kInvalidMemSize);
        prepared_.clear();
        return;
    }

    state_ = State::kPrepared;
    sendCommandSuccess(command.command_type);
}

void Bootloader::handleStartErase(const MemoryCommand& command) noexcept {
    if ((state_ != State::kPrepared) || !prepared_.isValid() || !prepared_.matches(command)) {
        sendCommandFailed(command.command_type, ErrorCode::kNotPrepared);
        return;
    }

    if (prepared_.command_type != CommandType::kPrepareErase) {
        sendCommandFailed(command.command_type, ErrorCode::kNotPrepared);
        return;
    }

    LOG_ERROR() << "Do erase not implemented yet";
    sendCommandFailed(command.command_type, ErrorCode::kUnknown);
}

void Bootloader::handleStartAppDownload(const MemoryCommand& command) noexcept {
    if ((state_ != State::kPrepared) || !prepared_.isValid() || !prepared_.matches(command)) {
        sendCommandFailed(command.command_type, ErrorCode::kNotPrepared);
        return;
    }

    if (prepared_.command_type != CommandType::kPrepareAppDownload) {
        sendCommandFailed(command.command_type, ErrorCode::kNotPrepared);
        return;
    }

    // TODO: check transfer type
    state_ = State::kTransfer;
    sendCommandSuccess(command.command_type);
}

void Bootloader::handleEndAppDownload() noexcept {
    if (state_ != State::kTransfer) {
        sendCommandFailed(CommandType::kEndAppDownload, ErrorCode::kNotPrepared);
        LOG_ERROR() << "Not in transfer state";
        return;
    }
    
    sendGeneralCommand(CommandType::kCommandPending, static_cast<uint16_t>(CommandType::kEndAppDownload), 0U);
    if (validateTransfer()) {
        sendCommandSuccess(CommandType::kEndAppDownload);
        LOG_INFO() << "Transfer successful: "; // TODO: << prepared_;
    } else {
        // TODO: handle other image verification failure types
        sendCommandFailed(CommandType::kEndAppDownload, ErrorCode::kInvalidImageCrc);
        LOG_ERROR() << "Transfer failed: "; // TODO: << prepared_;
    }

    state_ = State::kIdle;
    prepared_.clear();
}

void Bootloader::handleReset() noexcept {
    sendCommandSuccess(CommandType::kReset);
    hal::delayFor<hal::PlatformClock>(std::chrono::milliseconds(15U));
    reset_.reset();
}


/******************************************************************************
 *  Memory Interface
 ******************************************************************************/

// TODO: handle start addresses that aren't at the start of a region
const Memory::Region* Bootloader::regionFromAddress(uint32_t address) const noexcept {
    for (const auto& region : regions_) {
        if (Memory::isWithinRegion(address, 1U, region)) {
            return &region;
        }
    }
    return nullptr;
}

std::optional<ErrorCode> Bootloader::isReadyForSegment() const noexcept {
    if (state_ != State::kTransfer) {
        return ErrorCode::kNotInTransferState;
    }
    if (prepared_.active_region == nullptr) {
        return ErrorCode::kNoActiveRegion;
    }
    return std::nullopt;
}

// TODO: Consider changing the memory interface to word-oriented
bool Bootloader::writeSegment(const SegmentTransferCommandWord& command_word, util::Span<const uint32_t> memory_words) noexcept {
    const uint32_t segment_bytes = static_cast<uint32_t>(memory_words.size() * 4U);
    const uint32_t start_address = prepared_.start_word * 4U;

    const uint32_t write_address = start_address + (command_word.segment_number.value * segment_bytes);
    if (!Memory::isWithinRegion(write_address, segment_bytes, *prepared_.active_region)) {
        return false;
    }

    const uint8_t* byte_data = reinterpret_cast<const uint8_t*>(memory_words.data());
    return prepared_.active_region->interface->write(write_address, byte_data, segment_bytes);
}

// TODO: handle segment wrapping
void Bootloader::handleSegment(const SegmentTransferPacketView& segment) noexcept {
    last_rx_time_ = hal::PlatformClock::now();

    const std::optional<ErrorCode> ready_for_segment = isReadyForSegment();
    const std::optional<SegmentTransferCommandWord> command_word = SegmentTransferPacketView::decodeCommandWord(segment.command_word);

    if (!command_word.has_value()) {
        const util::Uint24_s segment_number{.value = segment.command_word & util::Uint24_s::kMask};
        sendSegmentNak(segment_number.value);
        LOG_ERROR() << "Invalid segment command word";
        return;
    }

    if (ready_for_segment.has_value()) {
        sendSegmentNak(command_word.value().segment_number.value);
        LOG_ERROR() << "Not ready for segment: " << static_cast<uint32_t>(ready_for_segment.value());
        return;
    }

    if (!writeSegment(command_word.value(), segment.memory_words)) {
        sendSegmentNak(command_word.value().segment_number.value);
        LOG_ERROR() << "Failed to write segment";
        return;
    }

    sendSegmentAck(command_word.value().segment_number.value);
}

} // namespace bootloader
