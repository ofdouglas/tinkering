#pragma once
/*
 * @file event_trace.h
 * @brief Event trace implementation
*/

#include <array>
#include <atomic>
#include <cstdint>
#include <limits>
#include <optional>

#include "data_structures/ring_buffer.h"
#include "util/span.h"
#include "logging/logging.h"

/* Event Trace System
 *  - Thread-safe event tracing system with arbitrary data payloads.
 *  - Each thread has it's own lock-free SPSC queue to the event tracer.
 *  - Threads log events by enqueuing them to their own SPSC queue.
 *  - The event tracer reads and processes events from all threads, logging
 *    them as desired.
 *  - The user defines the logging implementation and policy for each event,
 *    either individually or using a global policy. The default global policy
 *    is to log all events to the console with no rate-limiting.
 *  - Defining logging policy includes, per event type (not exhaustive):
 *    - Per-event rate-limiting for logging
 *    - Global rate-limiting for logging
 *    - Coalescing multiple events into a single log message (can be as simple
 *      as the max value, latest value, etc, or can be an arbitrary function).
 *    - Priority for logging
 *    - Filtering repeated messages (TBD)
 * 
 * Example Usage:
 *  // Thread 1:
 *    if (!doOperation(foo)) {
 *      recordEvent(kEventFooFailed); // Enqueues event
 *    }
 *    ...
 *  // Thread 2:
 *    if (!doOperation(bar)) {
 *      recordEvent(kEventBarFailed); // Enqueues event
 *    }
 *    ...
 *  // Thread 3 (Event Tracer):
 *    for (event : registered_events) {
 *      if (event.hasChanged()) {
 *        LOG() << event.name << " count: " << event.count;
 *      }
 *    }
*/

namespace logging {

using EventIdT = uint16_t;
using ThreadIdT = uint16_t;
constexpr EventIdT kInvalidEventId = std::numeric_limits<EventIdT>::max();

enum class EventDataFormat : uint8_t {
    kInvalid = 0,
    kNoData = 1,  // Log the event name only.
    kBool = 2,
    kCString = 3,
    kUint8 = 4,
    kInt8 = 5,
    kUint16 = 6,
    kInt16 = 7,
    kUint32 = 8,
    kInt32 = 9,
    kUint64 = 10,
    kInt64 = 11,
    kFloat = 12,
    kDouble = 13
};

struct EventHandle {
    const char* name;
    EventIdT    event_id;
};

struct Event {
    EventHandle             handle;
    EventDataFormat         data_format[2U];
    uint8_t                 sequence_number;    // Can be used to chain adjacent events together into one log message?
    std::array<uint8_t, 8U> data_buffer;
};

// A few example events:
//
// Event{kApplicationInitializingEventHandle, EventDataFormat::kNoData, 0, {}}  prints "Application Initializing"
// Event{kInvalidEnumValue, EventDataFormat::kUint32, 0, {fsm_.state_}}         prints "Invalid enum value: 123"
// Event{kCoreTemperatureEventHandle, EventDataFormat::kFloat, 0, {core_temperature}}  prints "Core Temperature: 123.45"
// Event{kRailVoltageEventHandle, EventDataFormat::kFloat, EventDataFormat::kFloat, {rail3v3_voltage, rail5v_voltage}}  prints "Rail Voltage: 3.3, 5.0"
// TODO: big problem with the last one: how is the float formatted? how many decimal places? Naively this may be "Rail Voltage: 3.29879897, 5.01234567"
//
// How code uses it:
//
// auto event = Event{kApplicationInitializingEventHandle, EventDataFormat::kNoData, 0, {}};
// events_queue_.push(event); // SPSC queue which is private between the thread and the event tracer. Each thread has one queue.
//



struct EventArg {
    std::array<uint8_t, 8U> data;
    EventDataFormat         data_format;
};

// Chained events: User should be able to pass arbitrary length (up to a small limit) and primitive type messages to the API.
struct ChainedEvent {
    EventHandle              handle;
    std::array<EventArg, 8U> args;
};

struct ChainedEventExampleData {
    float phase_currents[3U];
    float phase_voltages[3U];
};

ChainedEventExampleData chained_event_example_data;

// This is passed to the "transport layer" as:
auto chained_event = ChainedEvent{kMotorPhaseVoltsAmpsEventHandle, {
        EventArg{chained_event_example_data.phase_currents[0], EventDataFormat::kFloat}, 
        EventArg{chained_event_example_data.phase_currents[1], EventDataFormat::kFloat}, 
        EventArg{chained_event_example_data.phase_currents[2], EventDataFormat::kFloat},
        EventArg{chained_event_example_data.phase_voltages[0], EventDataFormat::kFloat},
        EventArg{chained_event_example_data.phase_voltages[1], EventDataFormat::kFloat},
        EventArg{chained_event_example_data.phase_voltages[2], EventDataFormat::kFloat},
    }};

// Chained events require you to provide a custom handler to the event tracer.


// Base class for all event channels.
// Both the thread and the event tracer need access to the event channel.
template <size_t kQueueDepth>
class EventChannel {
public:

    EventChannel() : count_(0), last_count_read_(0) noexcept {}
    ~EventChannel() = default;

    EventHandle getHandle() const { return handle_; }

    void recordEvent() {
        Event event = { handle_, EventDataFormat::kNoData, 0, {} };
        queue_.write(event);
        count_.fetch_add(1, std::memory_order_relaxed);
    }

    template<typename T>
    void recordEventData(EventDataFormat data_format, T data) {
        Event event = { handle_, data_format, 0, {} };

        if constexpr (std::is_same_v<T, bool>) {
            data_format = EventDataFormat::kBool;
            memcpy(event.data_buffer.data(), &data, sizeof(T));
        } else if constexpr (std::is_same_v<T, char>) {
            data_format = EventDataFormat::kCString;
            memcpy(event.data_buffer.data(), &data, sizeof(T));
        } else if constexpr (std::is_same_v<T, uint8_t>) {
            data_format = EventDataFormat::kUint8;
            memcpy(event.data_buffer.data(), &data, sizeof(T));
        } else if constexpr (std::is_same_v<T, int8_t>) {
            data_format = EventDataFormat::kInt8;
            memcpy(event.data_buffer.data(), &data, sizeof(T));
        } else if constexpr (std::is_same_v<T, uint16_t>) {
            data_format = EventDataFormat::kUint16;
            memcpy(event.data_buffer.data(), &data, sizeof(T));
        } else if constexpr (std::is_same_v<T, int16_t>) {
            data_format = EventDataFormat::kInt16;
            memcpy(event.data_buffer.data(), &data, sizeof(T));
        } else if constexpr (std::is_same_v<T, uint32_t>) {
            data_format = EventDataFormat::kUint32;
            memcpy(event.data_buffer.data(), &data, sizeof(T));
        } else if constexpr (std::is_same_v<T, int32_t>) {
            data_format = EventDataFormat::kInt32;
            memcpy(event.data_buffer.data(), &data, sizeof(T));
        } else if constexpr (std::is_same_v<T, uint64_t>) {
            data_format = EventDataFormat::kUint64;
            memcpy(event.data_buffer.data(), &data, sizeof(T));
        } else if constexpr (std::is_same_v<T, int64_t>) {
            data_format = EventDataFormat::kInt64;
            memcpy(event.data_buffer.data(), &data, sizeof(T));
        } else if constexpr (std::is_same_v<T, float>) {
            data_format = EventDataFormat::kFloat;
            memcpy(event.data_buffer.data(), &data, sizeof(T));
        } else if constexpr (std::is_same_v<T, double>) {
            data_format = EventDataFormat::kDouble;
            memcpy(event.data_buffer.data(), &data, sizeof(T));
        } else {
            LOG_ERROR() << "Invalid data type: " << typeid(T).name();
        }
        recordEventDataImpl(data_format, event.data_buffer);
    }
    
    Event readEvent() {
        return queue_.read();
    }

private:
    void recordEventDataImpl(EventDataFormat data_format, util::Span<const uint8_t> data) {
        Event event = { handle_, data_format, 0, {} };
        memcpy(event.data_buffer.data(), data.data(), data.size());
        queue_.write(event);
        count_.fetch_add(1, std::memory_order_relaxed);
    }

    RingBuffer<Event, kQueueDepth> queue_;
};

template <size_t kMaxNumClients>
class EventTracer {
public:
    EventTracer(util::Span<EventChannel> basic_event_channels) : basic_event_channels_(basic_event_channels) {}

    void processEvents() {
        while (!queue_.empty()) {
            Event event = queue_.read();
            logEvent(event);
        }
    }

private:
    // Helper function to log the event. Only handles 1 scalar data type. Does not handle sequence numbers / coalescing.
    void logEvent(const Event& event) const {
        switch (event.data_format) {
            case EventDataFormat::kNoData:
                LOG_INFO() << event.handle.name;
                break;
            case EventDataFormat::kBool:
                LOG_INFO() << event.handle.name << ": " << event.data_buffer[0];
                break;
            case EventDataFormat::kCString:
                LOG_INFO() << event.handle.name << ": " << event.data_buffer.data();
                break;
            case EventDataFormat::kUint8:
                LOG_INFO() << event.handle.name << ": " << event.data_buffer[0];
                break;
            case EventDataFormat::kInt8:
                LOG_INFO() << event.handle.name << ": " << event.data_buffer[0];
                break;
            case EventDataFormat::kUint16:
                LOG_INFO() << event.handle.name << ": " << event.data_buffer[0];
                break;
            case EventDataFormat::kInt16:
                LOG_INFO() << event.handle.name << ": " << event.data_buffer[0];
                break;
            case EventDataFormat::kUint32:
                LOG_INFO() << event.handle.name << ": " << event.data_buffer[0];
                break;
            case EventDataFormat::kInt32:
                LOG_INFO() << event.handle.name << ": " << event.data_buffer[0];
            default:
                LOG_INFO() << event.handle.name << ": " << "Invalid data format: " << static_cast<uint8_t>(event.data_format);
                break;
        }
    }

    util::Span<EventChannel> event_channels_;
};

} // namespace logging