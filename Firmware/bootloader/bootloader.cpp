#include "protocol.h"
#include "bootloader.h"

#include "hal/clock.h"
#include "hal/delay.h"

#include "logging/logging.h"

#include <limits.h>

namespace Bootloader {

Bootloader::Bootloader(Span<const Memory::Region> regions, TransportInterface& transport, hal::ResetInterface& reset) noexcept :
    regions_(regions),
    transport_(transport),
    reset_(reset) {}





/******************************************************************************
 *  Public API
 ******************************************************************************/

 bool Bootloader::initialize() noexcept {
    transport_.setMessageReceivedCallback([this](const Message& message) {
        handleMessage(message);
    });
    transport_.setSegmentReceivedCallback([this](const MemTransferSegmentGeneric& segment) {
        handleSegment(segment);
    });

    return true;
}

bool Bootloader::validateApplication() noexcept {
    for (const auto& region : regions_) {
        if (region.attributes & Memory::Region::Attributes::kBootable) {
            return false;
        }
    }

    // TODO: validate application
    return false;
}

bool Bootloader::tick() noexcept {
    // Run the transport layer and wait for callbacks
    transport_.receive();

    // TODO: handle current state

    return true;
};


/******************************************************************************
 *  Message Dispatch
 ******************************************************************************/

void Bootloader::acceptCommand(const Message& message) noexcept {
    sendMessage(MessageType::kCommandAccept, message.job_id, 0, message.message_type);
};

void Bootloader::rejectCommand(const Message& message) noexcept {
    sendMessage(MessageType::kCommandReject, message.job_id, 0, message.message_type);
};

void Bootloader::handleJobId(const Message& message) noexcept {
    if (job_context_.job_id != message.job_id) {
        job_context_.create(message.job_id);
        LOG_INFO() << "New job accepted: " << message.job_id;
        state_ = State::kJobSetup;
    }
}

void Bootloader::handleMessage(const Message& message) noexcept {
    handleJobId(message);

    switch (message.message_type) {
        // Discrete jobs
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

        // Tracked jobs
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

        case MessageType::kCommandAccept: // Intentional fall-through
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
    LOG_INFO() << "Bootloader info: " << bootloader_info;
}

// TODO: get image status
void Bootloader::handleImageStatus1(const Message& message) noexcept {
    const ImageStatus1 image_status = {
        .status_a = ImageStatus::kUnknown,
        .status_b = ImageStatus::kNotSupported,
    };

    const uint32_t image_status_value = *reinterpret_cast<const uint32_t*>(&image_status);
    sendMessage(MessageType::kImageStatus1, message.job_id, image_status_value);
    LOG_INFO() << "Image status: " << image_status;
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
    LOG_INFO() << "Board version: " << board_version;
}

// TODO: get app version
void Bootloader::handleAppVersion1(const Message& message) noexcept {
    const AppVersion1 app_version = {
        .app_version_major = 1,
        .app_version_minor = 0,
    };

    const uint32_t app_version_value = *reinterpret_cast<const uint32_t*>(&app_version);
    sendMessage(MessageType::kAppVersion1, message.job_id, app_version_value);
    LOG_INFO() << "App version: " << app_version;
}

/******************************************************************************
 *  Job Setup Messages
 ******************************************************************************/

void Bootloader::handleSetStartAddress(const Message& message) noexcept {
    job_context_.start_address = message.value32;

    // TODO: handle start addresses that aren't at the start of a region
    Memory::MemoryInterface* memory_interface = memoryInterfaceFromAddress(job_context_.start_address);
    if (memory_interface == nullptr) {
        rejectCommand(message);
        LOG_ERROR() << "No memory interface found for start address: " << job_context_.start_address;
        return;
    }

    job_context_.active_region = memory_interface;
    acceptCommand(message);
    LOG_INFO() << "Set start address: " << job_context_.start_address;
}

void Bootloader::handleSetSizeBytes(const Message& message) noexcept {
    job_context_.size_bytes = message.value32;

    acceptCommand(message);
    LOG_INFO() << "Set size bytes: " << job_context_.size_bytes;
}

void Bootloader::handleDoErase(const Message& message) noexcept {
    LOG_ERROR() << "Do erase not implemented yet";
    rejectCommand(message);
}

// TODO: check transfer type
void Bootloader::handleStartTransfer(const Message& message) noexcept {
    if (!job_context_.isValid() || (state_ != State::kJobSetup)) {
        rejectCommand(message);
        LOG_ERROR() << "Invalid job setup "; // TODO: << job_context_, state_;
        return;
    }

    state_ = State::kTransfer;
    acceptCommand(message);
    LOG_INFO() << "Start transfer: " << job_context_.start_address << " - " << job_context_.size_bytes;
}

// TODO: check transfer type
void Bootloader::handleFinalizeTransfer(const Message& message) noexcept {
    if (state_ != State::kTransfer) {
        rejectCommand(message);
        LOG_ERROR() << "Not in transfer state";
        return;
    }

    acceptCommand(message);
    LOG_INFO() << "Finalize transfer: " << job_context_.start_address << " - " << job_context_.size_bytes;

    if (validateTransfer()) {
        sendMessage(MessageType::kTransferSuccess, job_context_.job_id);
        LOG_INFO() << "Transfer successful: "; // TODO: << job_context_;
    } else {
        sendMessage(MessageType::kTransferFailed, job_context_.job_id);
        LOG_ERROR() << "Transfer failed: "; // TODO: << job_context_;
    }

    state_ = State::kIdle;
    job_context_.create(JobContext::kInvalidJobId);
}

void Bootloader::handleReset(const Message& message) noexcept {
    acceptCommand(message);
    LOG_INFO() << "Reset";
    hal::delayFor<hal::PlatformClock>(std::chrono::Milliseconds(15));
    reset_.reset();
}

/******************************************************************************
 *  Memory Interface
 ******************************************************************************/

// TODO: handle start addresses that aren't at the start of a region
Memory::MemoryInterface* Bootloader::memoryInterfaceFromAddress(uint32_t address) const noexcept {
    for (const auto& region : regions_) {
        if (address == region.start_address) {
            return region.interface;
        }
    }
    return nullptr;
}

void Bootloader::handleSegment(const MemTransferSegmentGeneric& segment) noexcept {
    auto nakSegment = [this]() {
        sendMessage(MessageType::kSegmentNak, job_id_, segment_number_);
    };
    
    if (state_ != State::kTransfer) {
        nakSegment();
        LOG_ERROR() << "Not in transfer state";
        return;
    }

    if (segment.job_id != job_id_) {
        nakSegment();
        LOG_ERROR() << "Job ID mismatch: got " << segment.job_id << ", expected " << job_id_;
        return;
    }
    
    if (active_region_ == nullptr) {
        nakSegment();
        LOG_ERROR() << "No active region";
        return;
    }

    if (active_region_->write(segment.data.data(), segment.data.size())) {
        sendMessage(MessageType::kSegmentAck, job_id_, segment_number_);
        segment_number_++;
        return;
    }

    nakSegment();
    LOG_ERROR() << "Failed to write segment to active region";
}

} // namespace Bootloader