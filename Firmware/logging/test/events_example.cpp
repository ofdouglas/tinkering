#include "logging/event_trace.h"
#include "logging/logging.h"

#include <array>
#include <cstdio>

namespace logging {

enum class EventId : EventIdT {
    kUnknownEvent = 0,
    kBootloaderInitializing = 1,
    kBootloaderInitialized = 2,
    kBootloaderFailedToInitialize = 3,
    kStartingApplicationDownload = 4,
    kApplicationDownloaded = 5,
    kApplicationFailedToDownload = 6,
    kApplicationStarting = 7,
    kApplicationStarted = 8,
    kApplicationFailedToStart = 9,
    // EventIds can be sparse:
    //kDeprecatedEvent10 = 10,
    //kDeprecatedEvent11 = 11,
    //kDeprecatedEvent12 = 12,
    kEventIdNotRelevantInBootloader = 13,
    kSocketCreateFailed = 203,
    kSocketConnected = 204
    // etc
};

constexpr std::array<BasicEventChannel, 10U> kBootloaderEventChannels = {{
    BasicEventChannel(EventHandle{EventId::kUnknownEvent, "Unknown Event"}),
    BasicEventChannel(EventHandle{EventId::kBootloaderInitializing, "Bootloader initializing..."}),
    BasicEventChannel(EventHandle{EventId::kBootloaderInitialized, "Bootloader initialized"}),
    BasicEventChannel(EventHandle{EventId::kBootloaderFailedToInitialize, "Bootloader failed to initialize"}),

    BasicEventChannel(EventHandle{EventId::kStartingApplicationDownload, "kStartingApplicationDownload", 
            auto [](uint32_t size) -> void { LOG_INFO() << "Starting application download, size = " << size << " bytes"}}),
            
    BasicEventChannel(EventHandle{EventId::kApplicationDownloaded, "Application downloaded"}),
    BasicEventChannel(EventHandle{EventId::kApplicationFailedToDownload, "Application failed to download"}),
    BasicEventChannel(EventHandle{EventId::kApplicationStarting, "Application starting..."}),
    BasicEventChannel(EventHandle{EventId::kApplicationStarted, "Application started"}),
    BasicEventChannel(EventHandle{EventId::kApplicationFailedToStart, "Application failed to start"})
}};

bool logEvent(EventId eventId) {
    const EventIdT id = static_cast<EventIdT>(eventId);
    for (auto& channel : kBootloaderEventChannels) {
        if (channel.getHandle().event_id == id) {
            channel.recordEvent();
            return true;
        }
    }
    return false;
}

class StdoutLogSink : public LogSink {
public:
    bool write(util::Span<const uint8_t> message) override {
        return std::fwrite(message.data(), 1U, message.size(), stdout) == message.size();
    }
};

} // namespace logging

int main() {
    logging::StdoutLogSink sink;
    logging::setLogSink(&sink);

    logging::EventTracer<10> eventTracer(logging::kBootloaderEventChannels);
    eventTracer.processEvents();

    logging::logEvent(logging::EventId::kBootloaderInitializing);
    logging::logEvent(logging::EventId::kBootloaderInitialized);
    eventTracer.processEvents();

    logging::logEvent(logging::EventId::kStartingApplicationDownload);
    logging::logEvent(logging::EventId::kSocketConnected);
    eventTracer.processEvents();

    return 0;
}
