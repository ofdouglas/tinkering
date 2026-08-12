# Logging System Design

High-level design for the Firmware logging stack. The system splits **structured
events** (hot path) from **deferred messages** (flexible path), both drained by a
single low-priority logging task.

## Goals

- Safe to call from time-sensitive tasks (control loops, ISRs where appropriate).
- No dynamic allocation on the record path.
- Single source of truth for event semantics (id, name, argument layout, format).
- Covers the majority of firmware signals without per-event custom formatters.
- Escape hatch for arbitrary debug-style messages without polluting the event registry.

## Architecture Overview

```
  Thread A ──SPSC──┐
  Thread B ──SPSC──┼──► Logging task ──► format ──► LogSink (UART, etc.)
  Thread C ──SPSC──┘         ▲
                             │
                    Event registry (metadata)
```

Each producer thread owns one **lock-free SPSC queue** to the logging task. The
producer only serializes a small record and enqueues it. Formatting (`snprintf`,
string assembly) happens on the consumer side.

Two record types may share the same per-thread queue (tagged union) or use
separate queues drained by the same task. See [IPC](#ipc).

## Two-Tier Model

| | **Event** | **DeferredLogger** |
|---|---|---|
| **Purpose** | Structured, registry-defined signals | Free-form debug / one-off messages |
| **Producer cost** | Very low: fixed-size record | Higher: serialize each argument into a blob |
| **Consumer cost** | Registry lookup + known format | Parse blob + generic deserialization |
| **Typical shapes** | `"name"`, `"name: value"`, `"name: v1 v2"` | Arbitrary strings and mixed types |
| **Registry row** | Required | Not required (file/line/level in record) |
| **Hot-path use** | Default | Only when an Event is insufficient |

**Rule of thumb:** if the message is worth keeping long-term, make it an Event.
Use `DEFERRED_LOG_*()` for bring-up debug, irregular prose, or anything that does
not justify a registry entry.

### Immediate logging (`LOG_*`)

`logging.h` provides synchronous `LOG_INFO()` / `LOG_WARN()` / etc. via
`LocalLogger`. This is appropriate for host unit tests and early bring-up. On
MCU targets, application code should prefer Event or DeferredLogger and reserve
direct `LOG_*()` for the logging task itself or non-real-time contexts.

## Event System

### Wire record (transport)

Events on the queue carry **only data needed to identify and decode the
payload**. Registry metadata (name, format tags) lives in the registry, not on
the wire. **Call-site context** (`file`, `line`) is included on the wire because
it identifies which SW component / code path produced the event — essential for
shared events like `kInvalidEnumValue` that are raised from many places.

```cpp
struct EventRecord {
    EventIdT      id;        // 2 bytes
    uint8_t       arg_count; // 0, 1, 2, or 3
    uint8_t       reserved;
    const char*   file;      // __FILE__ string-literal pointer (4 bytes on 32-bit)
    uint32_t      line;      // __LINE__
    uint32_t      arg0;      // raw bits (uint32 or float)
    uint32_t      arg1;
    uint32_t      arg2;
};
```

Target size: ~24 bytes — still a small fixed copy, suitable for tight loops.

`file` is a pointer to a compile-time string literal (`__FILE__` or
`__FILE_NAME__`); nothing is copied on the record path. The logging task prints
the basename, consistent with `LocalLogger` / `writeMessage()` in `logging.h`.

v1 caps at **two arguments**, which covers the majority of cases:

- `"bootloader.cpp:142 [INFO] Bootloader initialized"`
- `"motor_ctrl.cpp:88 [INFO] Invalid enum value: 123"`
- `"power.cpp:201 [INFO] Rail voltages: 3.30, 5.01, 12.07"`

Events needing three or more formatted fields, or custom layout, should use
DeferredLogger or gain a v2 extension (larger payload / custom registry handler).

### Registry (semantics)

A single table (X-macro `.inc` file or generated from YAML) is the source of
truth for every event:

```cpp
struct EventDescriptor {
    EventIdT      id;
    const char*   name;
    ArgFormat     arg0_format;  // kNone, kUint32Dec, kUint32Hex, kFloat3, ...
    ArgFormat     arg1_format;
    // Future: rate-limit, coalesce policy, custom log handler
};
```

`ArgFormat` encodes **representation** (how to print), not C++ type:

- `kNone`, `kUint32Dec`, `kUint32Hex`, `kUint32Hex0Pad`, `kInt32Dec`, `kFloat2`, `kFloat3`, `kFloat4`, `kBool`, ...

The producer passes raw values; the logging task looks up the descriptor and
formats consistently (e.g. float3 always prints 3 significant figures for a
given event).

Event id describes **what** happened. `file` / `line` describe **where** it was
raised. The registry name alone is not enough when the same event id is shared
across components (e.g. `kInvalidEnumValue`, `kTimeout`, `kCrcMismatch`).

### Producer API (sketch)

Call-site context is injected by macros so application code does not pass
`__FILE__` / `__LINE__` manually:

```cpp
// Per-thread queue, created at task start
RECORD_EVENT(kBootloaderInitialized);
RECORD_EVENT(kInvalidEnumValue, state);
RECORD_EVENT(kRailVoltages, v3v3, v5v0 v12v0);
```

Macro expansion (sketch):

```cpp
#define RECORD_EVENT(id, ...) \
    recordEvent(__FILE__, __LINE__, EventId::id, ##__VA_ARGS__)
```

Underlying API:

```cpp
void recordEvent(const char* file, uint32_t line, EventId id);
void recordEvent(const char* file, uint32_t line, EventId id, uint32_t arg0);
void recordEvent(const char* file, uint32_t line, EventId id, uint32_t arg0, uint32_t arg1);
void recordEvent(const char* file, uint32_t line, EventId id, uint32_t arg0, uint32_t arg1, uint32_t arg2);
// float overloads store raw float bits in arg0/arg1/arg2
```

Typed overloads or template wrappers can `static_assert` argument counts against
the registry at compile time.

### Consumer output

The logging task formats each event roughly as:

```
<file_basename>:<line> [LEVEL] <registry_name>[: <formatted_args>]
```

Example:

```
motor_ctrl.cpp:88 [INFO] Invalid enum value: 7
```

This mirrors the existing `LOG_*()` prefix style and makes shared events
actionable without per-call-site registry entries.

Related files:

- `event_trace.h` — Event types, record layout, record API (WIP)
- `events.inc` — Registry source of truth (TBD)

## DeferredLogger

For messages that do not fit the Event model. The producer builds a **serialized
blob** of typed items; formatting to text happens in the logging task.

### Wire record (transport)

`deferred_logger.h` defines a length-prefixed serialization:

- `ItemHeader` (type + size) per argument
- Supported types: `uint32_t`, `int32_t`, `string` (v1; extensible)
- Variable-size buffer (default ~100 bytes, configurable)

```cpp
DEFERRED_LOG_INFO() << "Parsed header: version=" << version << " len=" << len;
```

On destruction, the `DeferredLogger` enqueues the serialized buffer (does **not**
format on the producer side).

### Producer vs consumer cost

| Step | Where |
|---|---|
| `<<` operators, `memcpy` into buffer | Producer (bounded, no `snprintf`) |
| Walk items, `LocalLogger` / `snprintf` | Logging task |

Fast enough for many real-time contexts, but measurably more expensive than an
`EventRecord`. Prefer Event when possible.

### Current status

`deferred_logger.h` implements serialization and deserialization. The destructor
currently calls `deserialize()` inline — this should be changed to enqueue to the
logging task's queue.

## Logging Task

One task (or bare-metal main-loop hook) drains all per-thread queues and writes
to `LogSink` (`logging.h`).

```cpp
void LoggingTask::run() {
    for (auto& queue : thread_queues_) {
        while (QueueItem item; queue.try_pop(item)) {
            switch (item.type) {
            case QueueItemType::kEvent:
                formatEvent(item.event);
                break;
            case QueueItemType::kDeferredLog:
                deserializeAndEmit(item.deferred);
                break;
            }
        }
    }
}
```

Round-robin across threads avoids starvation. Events may be prioritized over
deferred logs when both are pending.

## IPC

### Per-thread SPSC queue

- **Producer:** task or ISR (if enqueue is wait-free and ISR-safe).
- **Consumer:** logging task only.
- No locks on the record path.

Queue depth is configured **per thread** (hot tasks may need deeper queues).
Separate from event registry semantics.

### Queue item layout (option A — unified queue)

```cpp
enum class QueueItemType : uint8_t { kEvent, kDeferredLog };

struct QueueItem {
    QueueItemType type;
    union {
        EventRecord event;
        uint8_t     deferred[kMaxDeferredSize];
    };
};
```

### Overflow policy (TBD)

Pick one and expose counters:

- Drop newest
- Drop oldest
- Block (not recommended on producer)

A `dropped_records` counter (per queue or global) should be readable from the
logging task or a diagnostic event.

## Event Registry at Scale

For tens of events: X-macro `events.inc` generating enum, descriptors, and
`recordEvent()` lookup.

For hundreds: codegen from YAML/CSV with validation (duplicate ids, arg count
mismatches, orphan queue configs).

Registry rows should read as a single line of semantics:

```
EVENT(4, kStartingApplicationDownload, "Starting application download", kUint32Dec)
EVENT(42, kRailVoltages, "Rail voltages", kFloat2, kFloat2, kFloat3)
```

Custom formatters are the exception, referenced by name on the same row.

## ISR and Thread Safety

| API | ISR-safe? | Notes |
|---|---|---|
| `recordEvent()` / `RECORD_EVENT()` (Event) | Maybe | Only if SPSC enqueue is wait-free and queue is ISR-dedicated or shared safely |
| `DEFERRED_LOG_*()` | Unlikely | Serialization cost and string handling too heavy |
| `LOG_*()` | No | Formats synchronously; for logging task / host tests |

## File Map

| File | Role |
|---|---|
| `logging.h` | `LogSink`, `LocalLogger`, synchronous `LOG_*` macros |
| `log.cpp` | Platform sink implementation (MCU) |
| `mock_log.cpp` | Host test sink |
| `event_trace.h` | Event record types and API (**WIP**, being refactored to this design) |
| `deferred_logger.h` | Serialized deferred log API (**WIP**, needs queue integration) |
| `events.inc` | Event registry source of truth (**TBD**) |
| `design.md` | This document |

## Implementation Phases

1. **Event core** — `EventRecord`, registry table, per-thread SPSC, drain + format loop.
2. **DeferredLogger integration** — enqueue blob in destructor; deserialize in logging task.
3. **Unified logging task** — drain both item types; wire to `LogSink`.
4. **Registry tooling** — `events.inc` X-macros; codegen if the table grows large.
5. **Policy** — rate limiting, coalescing, drop counters (as needed).

## Non-Goals (v1)

- Dynamic allocation on the record path.
- `std::variant` / RTTI on MCU targets.
- Per-event polling channels (replaced by per-thread queues).
- Embedded format strings on the wire (registry owns format policy).
- Guaranteed delivery (best-effort with drop counters is acceptable for hobby use).

## Open Questions

- Unified vs separate queues for Event and DeferredLogger.
- Exact overflow / drop policy.
- Whether ISR producers share the task queue or use a dedicated ISR → logging bridge.
- Max deferred log buffer size per platform.
- Whether rare events need custom `log()` handlers in the registry or should always use DeferredLogger.
- `__FILE__` vs `__FILE_NAME__` (path length in flash) for the `file` pointer.
- Whether level (`Info` / `Warn` / `Error`) belongs on `EventRecord` or is implied by registry row.
