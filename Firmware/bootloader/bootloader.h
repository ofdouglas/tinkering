#ifndef BOOTLOADER_H
#define BOOTLOADER_H

#include <stdint.h>
#include <stddef.h>
#include <functional>

#include "protocol.h"

#include "hal/clock.h"
#include "data_structures/span.h"

#include "interfaces/reset_interface.h"
#include "interfaces/memory_interface.h"

namespace Bootloader {

/******************************************************************************
 *  Interfaces
 ******************************************************************************/
class TransportInterface {
public:
    using MessageReceivedCallback = std::function<void(const Bootloader::Message& message)>;
    using SegmentReceivedCallback = std::function<void(const Bootloader::MemTransferSegmentGeneric& segment)>;

    virtual ~TransportInterface() = default;
    virtual bool sendMessage(const Bootloader::Message& message) noexcept = 0;
    virtual void receive() noexcept = 0;
    virtual bool setMessageReceivedCallback(MessageReceivedCallback callback) noexcept = 0;
    virtual bool setSegmentReceivedCallback(SegmentReceivedCallback callback) noexcept = 0;
};


/******************************************************************************
 *  Bootloader
 ******************************************************************************/

// Inputs: 
//  - Transport layer for outbound Messages and Segments
//  - Logging driver?
//  - Reset driver
//  x Memory specifications
//  x Memory access drivers
//  - Bootloader info, Board Info
class Bootloader {
public:
  enum class State : uint8_t {
    kUnknown = 0,
    kFault,
    kIdle,
    kJobSetup,
    kTransfer,
  };

  Bootloader(Span<const Memory::Region> regions, TransportInterface& transport, hal::ResetInterface& reset) noexcept;

  bool initialize() noexcept;

  bool validateApplication() noexcept;

  bool tick() noexcept;

private:
    struct JobContext {
        void create(uint8_t new_job_id = Message::kInvalidJobId) noexcept {
            job_id = new_job_id;
            segment_number = 0;
            start_address = UINT32_MAX;
            size_bytes = UINT32_MAX;
            active_region = nullptr;
        }

        bool isValid() const noexcept {
            // Unset values
            if ((active_region == nullptr) || (active_region->interface == nullptr)) {
                return false;
            }
            if ((start_address == UINT32_MAX) || (size_bytes == UINT32_MAX)) {
                return false;
            }
            if (job_id == Message::kInvalidJobId) {
                return false;
            }
            // Memory region constraints
            if (!isWithinRegion(start_address, size_bytes, *active_region)) {
                return false;
            }
            return true;
        }   

        uint8_t job_id{0};
        uint16_t segment_number{0};
        uint32_t start_address{UINT32_MAX};
        uint32_t size_bytes{UINT32_MAX};
        Memory::MemoryInterface* active_region{nullptr};
    };

    Memory::MemoryInterface* memoryInterfaceFromAddress(uint32_t address) const noexcept;
    bool sendMessage(MessageType message_type, uint8_t job_id, uint32_t value32 = 0U, uint8_t value8 = 0U) noexcept;
    void acceptCommand(const Message& message) noexcept;
    void rejectCommand(const Message& message) noexcept;
    void handleMessage(const Message& message) noexcept;
    void handleSegment(const MemTransferSegmentGeneric& segment) noexcept;
    void handleJobId(const Message& message) noexcept;
    void handleBootloaderInfo1(const Message& message) noexcept;
    void handleImageStatus1(const Message& message) noexcept;
    void handleBoardVersion1(const Message& message) noexcept;
    void handleAppVersion1(const Message& message) noexcept;
    void handleSetStartAddress(const Message& message) noexcept;
    void handleSetSizeBytes(const Message& message) noexcept;
    void handleDoErase(const Message& message) noexcept;
    void handleStartTransfer(const Message& message) noexcept;
    void handleMemTransferSegment(const Message& message) noexcept;
    void handleFinalizeTransfer(const Message& message) noexcept;
    void handleReset(const Message& message) noexcept;

    const Span<const Memory::Region> regions_;
    TransportInterface& transport_;
    hal::ResetInterface& reset_;


    State       state_{State::kIdle};
    JobContext  job_context_{};

    hal::PlatformClock::TimePoint last_active_time_{hal::PlatformClock::kTimeZero};
};

} // namespace Bootloader


#endif // BOOTLOADER_H