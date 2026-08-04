
#include "bootloader/mcu/bootloader.h"

#include "bootloader/image_header.h"

#include "hal/clock.h"
#include "hal/delay.h"

#include "logging/log.hpp"

#include <limits.h>

namespace Bootloader {

Bootloader::Bootloader(Span<const Memory::Region> regions, TransportInterface& transport, hal::ResetInterface& reset) noexcept :
    regions_(regions),
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

    const ImageHeader_Application_v1* image_header =
        reinterpret_cast<const ImageHeader_Application_v1*>(job_context_.active_region->start_address);

    return isValidApplication(*image_header);
}

bool Bootloader::sendMessage(MessageType message_type, uint8_t job_id, uint32_t value32, uint8_t value8) noexcept {
    Message message{};
    message.message_type = static_cast<uint8_t>(message_type);
    message.job_id = job_id;
    message.value32 = value32;
    message.value8 = value8;
    return transport_.sendMessage(message);
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
    transport_.setMessageReceivedCallback([this](const Message& message) {
        handleMessage(message);
    });
    transport_.setSegmentReceivedCallback([this](const MemTransferSegmentView& segment) {
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
 *  Message Dispatch
 ******************************************************************************/

void Bootloader::acceptCommand(const Message& message) noexcept {
    sendMessage(MessageType::kCommandAccept, message.job_id, 0U, message.message_type);
}

void Bootloader::rejectCommand(const Message& message) noexcept {
    sendMessage(MessageType::kCommandReject, message.job_id, 0U, message.message_type);
}

void Bootloader::handleJobId(const Message& message) noexcept {
    if (job_context_.job_id != message.job_id) {
        job_context_.create(message.job_id);
        LOG_INFO() << "New job accepted: " << message.job_id;
        state_ = State::kJobSetup;
    }
}

void Bootloader::handleMessage(const Message& message) noexcept {
    last_rx_time_ = hal::PlatformClock::now();
    handleJobId(message);

    switch (static_cast<MessageType>(message.message_type)) {
        case MessageType::kBootloaderInfo1:
            handleBootloaderInfo1(message);
            break;
        case MessageType::kImageStatus1:
            handleImageStatus1(message);
            break;
        case MessageType::kBoardVersion1:
            handleBoardVersion1(message);
            break;
        case MessageType::kAppVersion1:
            handleAppVersion1(message);
            break;
        case MessageType::kSetStartAddress:
            handleSetStartAddress(message);
            break;
        case MessageType::kSetSizeBytes:
            handleSetSizeBytes(message);
            break;
        case MessageType::kDoErase:
            handleDoErase(message);
            break;
        case MessageType::kStartTransfer:
            handleStartTransfer(message);
            break;
        case MessageType::kMemTransferSegment:
            handleMemTransferSegment(message);
            break;
        case MessageType::kFinalizeTransfer:
            handleFinalizeTransfer(message);
            break;
        case MessageType::kReset:
            handleReset(message);
            break;
        case MessageType::kCommandAccept:
        case MessageType::kCommandReject:
        case MessageType::kSegmentAck:
        case MessageType::kSegmentNak:
        case MessageType::kTransferSuccess:
        case MessageType::kTransferFailed:
            LOG_ERROR() << "Unexpected message type: " << message.message_type;
            break;
        default:
            LOG_ERROR() << "Unknown message type: " << message.message_type;
            break;
    }
}

/******************************************************************************
 *  Information Messages
 ******************************************************************************/

// TODO: get bootloader info
void Bootloader::handleBootloaderInfo1(const Message& message) noexcept {
    const BootloaderInfo1 bootloader_info = {
        .magic = BootloaderInfo1::kMagic,
        .protocol_version = 1,
        .bootloader_major = 1,
        .bootloader_minor = 0,
    };

    const uint32_t bootloader_info_value = *reinterpret_cast<const uint32_t*>(&bootloader_info);
    sendMessage(MessageType::kBootloaderInfo1, message.job_id, bootloader_info_value);
}

// TODO: get image status
void Bootloader::handleImageStatus1(const Message& message) noexcept {
    const ImageStatus1 image_status = {
        .status_a = static_cast<uint8_t>(ImageStatus::kUnknown),
        .status_b = static_cast<uint8_t>(ImageStatus::kNotSupported),
    };

    const uint32_t image_status_value = *reinterpret_cast<const uint32_t*>(&image_status);
    sendMessage(MessageType::kImageStatus1, message.job_id, image_status_value);
}

// TODO: get board version
void Bootloader::handleBoardVersion1(const Message& message) noexcept {
    const BoardVersion1 board_version = {
        .board_type = 1, // TODO
        .board_version_major = 1,
        .board_version_minor = 0,
    };

    const uint32_t board_version_value = *reinterpret_cast<const uint32_t*>(&board_version);
    sendMessage(MessageType::kBoardVersion1, message.job_id, board_version_value);
}

// TODO: get app version
void Bootloader::handleAppVersion1(const Message& message) noexcept {
    const AppVersion1 app_version = {
        .app_version_major = 1,
        .app_version_minor = 0,
    };

    const uint32_t app_version_value = *reinterpret_cast<const uint32_t*>(&app_version);
    sendMessage(MessageType::kAppVersion1, message.job_id, app_version_value);
}


/******************************************************************************
 *  Job Setup Messages
 ******************************************************************************/

void Bootloader::handleSetStartAddress(const Message& message) noexcept {
    job_context_.start_address = message.value32;

    job_context_.active_region = regionFromAddress(job_context_.start_address);
    if (job_context_.active_region == nullptr) {
        rejectCommand(message);
        LOG_ERROR() << "No memory interface found for start address: " << job_context_.start_address;
        return;
    }

    acceptCommand(message);
}

void Bootloader::handleSetSizeBytes(const Message& message) noexcept {
    job_context_.size_bytes = message.value32;
    acceptCommand(message);
}

void Bootloader::handleDoErase(const Message& message) noexcept {
    LOG_ERROR() << "Do erase not implemented yet";
    rejectCommand(message);
}

// TODO: check transfer type
void Bootloader::handleStartTransfer(const Message& message) noexcept {
    if (!job_context_.isValid() || (state_ != State::kJobSetup)) {
        rejectCommand(message);
        return;
    }

    state_ = State::kTransfer;
    job_context_.segment_number = 0U;
    acceptCommand(message);
}

void Bootloader::handleMemTransferSegment(const Message& message) noexcept {
    (void)message;
    LOG_ERROR() << "Mem transfer segment received as message; expected segment frame";
}

void Bootloader::handleFinalizeTransfer(const Message& message) noexcept {
    if (state_ != State::kTransfer) {
        rejectCommand(message);
        LOG_ERROR() << "Not in transfer state";
        return;
    }

    acceptCommand(message);

    if (validateTransfer()) {
        sendMessage(MessageType::kTransferSuccess, job_context_.job_id);
        LOG_INFO() << "Transfer successful: "; // TODO: << job_context_;
    } else {
        sendMessage(MessageType::kTransferFailed, job_context_.job_id);
        LOG_ERROR() << "Transfer failed: "; // TODO: << job_context_;
    }

    state_ = State::kIdle;
    job_context_.create(Message::kInvalidJobId);
}

void Bootloader::handleReset(const Message& message) noexcept {
    acceptCommand(message);
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

// TODO: handle segment wrapping
void Bootloader::handleSegment(const MemTransferSegmentView& segment) noexcept {
    auto nakSegment = [this, &segment]() {
        sendMessage(MessageType::kSegmentNak, segment.job_id, segment.segment_number);
    };

    if (state_ != State::kTransfer) {
        nakSegment();
        LOG_ERROR() << "Not in transfer state";
        return;
    }

    if (segment.job_id != job_context_.job_id) {
        nakSegment();
        LOG_ERROR() << "Job ID mismatch: got " << segment.job_id << ", expected " << job_context_.job_id;
        return;
    }

    if ((job_context_.active_region == nullptr) || (job_context_.active_region->interface == nullptr)) {
        nakSegment();
        LOG_ERROR() << "No active region";
        return;
    }

    const uint32_t write_address = job_context_.start_address + static_cast<uint32_t>(segment.segment_number) *
        static_cast<uint32_t>(segment.data.size());
    if (!Memory::isWithinRegion(write_address, segment.data.size(), *job_context_.active_region)) {
        nakSegment();
        return;
    }

    if (job_context_.active_region->interface->write(write_address, segment.data.data(), segment.data.size())) {
        sendMessage(MessageType::kSegmentAck, segment.job_id, segment.segment_number);
        job_context_.segment_number = static_cast<uint16_t>(segment.segment_number + 1U);
        return;
    }

    nakSegment();
    LOG_ERROR() << "Failed to write segment to active region";
}

} // namespace Bootloader
