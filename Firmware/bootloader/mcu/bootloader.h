#pragma once

#include <stdint.h>
#include <stddef.h>
#include <functional>
#include <optional>

#include "bootloader/protocol.h"
#include "bootloader/transport/transport_interface.h"

#include "hal/clock.h"
#include "hal/reset_interface.h"
#include "util/span.h"

#include "interfaces/memory_interface.h"
#include "bootloader/image_header.h"

namespace bootloader {

// TODO: Bootloader info, Board Info
class Bootloader {
public:
  enum class State : uint8_t {
    kUnknown = 0,
    kFault,
    kIdle,
    kPrepared,
    kTransfer,
  };

  Bootloader(util::Span<const Memory::Region> regions, TransportInterface& transport, hal::ResetInterface& reset) noexcept;

  bool initialize() noexcept;

  bool validateApplication() noexcept;

  State tick() noexcept;

private:
    struct PreparedMemoryContext {
        void clear() noexcept {
            command_type = CommandType::kUnknown;
            start_word = UINT32_MAX;
            size_words = UINT32_MAX;
            active_region = nullptr;
        }

        bool matches(const MemoryCommand& command) const noexcept {
            return (command_type == command.command_type) && (start_word == command.start_word) &&
                (size_words == command.size_words.value);
        }

        bool isValid() const noexcept {
            if (active_region == nullptr) {
                return false;
            }
            if (active_region->interface == nullptr) {
                return false;
            }
            if ((start_word == UINT32_MAX) || (size_words == UINT32_MAX)) {
                return false;
            }
            // TODO: validate command type fully
            if (command_type == CommandType::kUnknown) {
                return false;
            }
            const uint32_t start_address = start_word * 4U;
            const uint32_t size_bytes = size_words * 4U;
            if (!Memory::isWithinRegion(start_address, size_bytes, *active_region)) {
                return false;
            }
            return true;
        }

        CommandType command_type{CommandType::kUnknown};
        uint32_t start_word{UINT32_MAX};
        uint32_t size_words{UINT32_MAX};
        const Memory::Region* active_region{nullptr};
    };

    const Memory::Region* regionFromAddress(uint32_t address) const noexcept;
    bool isValidApplication(const ImageHeader_Application_v1& image_header) const noexcept;
    bool validateTransfer() const noexcept;
    std::optional<ErrorCode> isReadyForSegment() const noexcept;
    bool writeSegment(const SegmentTransferCommandWord& command_word, util::Span<const uint32_t> memory_words) noexcept;

    bool sendGeneralCommand(CommandType command_type, uint16_t value16 = 0U, uint32_t value32 = 0U) noexcept;
    void sendCommandSuccess(CommandType original_command) noexcept;
    void sendCommandFailed(CommandType original_command, ErrorCode error_code) noexcept;
    void sendSegmentAck(uint32_t segment_number) noexcept;
    void sendSegmentNak(uint32_t segment_number) noexcept;

    void handleCommand(const CommandPacket& command) noexcept;
    void handleGeneralCommand(const GeneralCommand& command) noexcept;
    void handleMemoryCommand(const MemoryCommand& command) noexcept;
    void handleSegment(const SegmentTransferPacketView& segment) noexcept;

    void handleReadDataIdentifier(uint16_t data_identifier) noexcept;
    void handleWriteDataIdentifier(uint16_t data_identifier, uint32_t value) noexcept;
    void handlePrepareMemory(const MemoryCommand& command) noexcept;
    void handleStartErase(const MemoryCommand& command) noexcept;
    void handleStartAppDownload(const MemoryCommand& command) noexcept;
    void handleEndAppDownload() noexcept;
    void handleReset() noexcept;
    bool bootApplication() noexcept;

    const util::Span<const Memory::Region> regions_;
    TransportInterface& transport_;
    hal::ResetInterface& reset_;

    State       state_{State::kIdle};
    PreparedMemoryContext prepared_{};
    Memory::Region* application_{nullptr};

    hal::PlatformClock::TimePoint last_tx_time_{hal::PlatformClock::kTimeZero};
    hal::PlatformClock::TimePoint last_rx_time_{hal::PlatformClock::kTimeZero};
};

} // namespace bootloader
