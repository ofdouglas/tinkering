# Link Entity Runtime and Status

## Status and authority

**Status:** Provisional runtime design for prototype validation; normative only for prototype experiments; no runtime API or status wire service is stable.

**Authority:** Within that provisional scope, this document is authoritative for Link Entity ownership, execution, recovery, state lifetime, status semantics, bounded publication behavior, and validation expectations.

This document is the concise runtime companion to [Logical Links and Transports](logical_links_and_transports.md). It defines ownership, execution, recovery, and status responsibilities without defining a wire-status schema. The device and network context is summarized in [Architecture Overview](architecture_overview.md).

Virtual-circuit identity, routing, and forwarding belong to [Core Protocol and Routing](core_protocol_and_routing.md). Device status is an application service using those facilities; it does not alter link or virtual-circuit semantics.

Normative terms such as **shall** and **must** state prototype semantic requirements. They do not stabilize enum names, function signatures, object layouts, endpoint allocations, schemas, wire encodings, or any other public API.

## 1. Link Entity ownership

A device contains **1..N Link Entities (LEs)**. Each LE owns **1..N local runtime link instances**, where one runtime link consists of the relevant Logical Link and Hardware Link/driver state.

An LE owns:

- serialization of mutable LLL and link-driver state;
- parser/reassembly, TX scheduling, timers, and lifecycle state for its links;
- bounded ingress, egress, completion, and control ports;
- live status publication for state it can still observe;
- restart and cancellation of its transient runtime state.

An LE may run in a super-loop, task, process/executor, or dedicated core. Each LLL still has one serialized mutable execution context as required by [Logical Links and Transports](logical_links_and_transports.md).

> **An LE owns runtime links; it does not own a whole virtual circuit.**

A virtual circuit may use one link, cross several links/LEs, or pass through other devices. Its identity and static route remain core configuration. Restarting an LE changes availability of its local link instances; it does not redefine, renumber, or take ownership of the virtual circuit.

A small device should normally use one LE unless scheduling, multicore placement, fault containment, or independent restart justifies more. Cross-LE forwarding uses explicit bounded ownership transfer and may copy when safe zero-copy transfer is unavailable.

## 2. Lifecycle contract

Links and LEs have independent lifecycle state.

A link shall support operations equivalent to:

```text
disabled -> starting -> up
    ^          |         |
    |          v         v
    +------ stopping <- degraded/faulted
```

An LE shall support operations equivalent to:

```text
stopped -> starting -> running -> stopping -> stopped
                     |
                     v
                  faulted -> restarting -> starting
```

Exact enum names are implementation-defined, but the contract shall provide:

- explicit start, stop, reset/reinitialize, enable, and disable requests;
- idempotent handling or explicit rejection of redundant requests;
- bounded request acceptance and completion reporting;
- a defined result for TX/RX work interrupted by stop or restart;
- rejection of new work while the relevant link/LE cannot accept it;
- a monotonically changing runtime generation or equivalent way to distinguish observations from before and after restart;
- ordinary handling of temporary link unavailability by routing and higher layers.

Normal lifecycle control also supports power management, maintenance, and configured reinitialization. It is not only a fault path.

Stopping or restarting shall not wait indefinitely for a failed driver or peer. The implementation shall define bounded quiesce and forced-cancellation behavior.

## 3. Incremental recovery

Recovery escalates from the smallest affected scope:

1. clear/restart the individual Hardware Link driver;
2. disable and re-enable the affected runtime link;
3. reconstruct or reset the owning LE's transient runtime;
4. restart a larger communications subsystem, if the platform has one;
5. reset the MCU/device only after lower-level recovery fails or system policy requires it.

The escalation policy shall define attempt limits, deadlines/backoff, and the conditions that move to the next level. Repeated automatic recovery shall not become an unbounded busy loop or diagnostic storm.

Uncertain transient work should be discarded rather than guessed back into a valid state. End-to-end transports or application services are responsible for recovering conversations that require continuity.

Recovery of one link should not stop healthy sibling links unless shared hardware, clocks, memory, or driver dependencies make isolation impossible. Such dependencies shall be represented in configuration and tests.

## 4. External supervision

An LE cannot be trusted to recover itself from every failure. A supervisor outside the monitored LE shall be able to observe at least:

- missing execution heartbeat or deadline;
- persistent lack of RX/TX progress where progress is expected;
- driver fault or repeated restart;
- invariant violation;
- sustained queue/buffer exhaustion;
- explicit LE fault transition.

The supervisor applies configured recovery escalation and records the reason/result outside disposable LE state.

The supervising path should depend on less infrastructure than the LE it supervises. A heartbeat updated by the failed task but consumed only by that same task is not external supervision.

Supervision is policy-driven: lack of traffic alone is not necessarily a fault on an idle link. Expectations, timeouts, and restart authority remain explicit configuration.

## 5. Independent debug path

Where hardware permits, provide a low-complexity debug path independent of the normal network runtime. It should avoid dependencies on:

- LLL parsing and routing;
- normal service dispatch;
- the primary packet-buffer pool;
- aggregate status publication;
- recoverable LE task state.

The path may be a dedicated debug interface, debugger channel, retained crash record, or another platform-specific mechanism. It is intended for boot progress, fatal fault evidence, and recovery observation, not routine high-volume traffic.

Debug access is privileged. Independence from the network stack does not exempt it from product security, production-disable, physical-access, or rate-limiting policy.

## 6. Transient, long-lived, and application-owned state

### 6.1 Disposable transient state

An LE restart may discard:

- partial frames and decoder state;
- incomplete profile-specific reassembly;
- pending timers and transient scheduler state;
- in-progress low-level transfers that cannot be recovered safely;
- queued work whose documented cancellation result is reported;
- live counters explicitly defined to reset with the runtime generation.

### 6.2 State that outlives an LE restart

The following shall live outside disposable LE runtime state:

- immutable/static configuration and named profile selection;
- persistent diagnostic counters required across LE restart;
- latched fault and restart records;
- supervisor state and recovery attempt history;
- resources intentionally shared by multiple LEs;
- storage backing buffers already transferred to application ownership.

Here, **persistent** means persistent across LE reconstruction. It does not necessarily mean nonvolatile across MCU reset or power loss. Each field shall have an explicit reset boundary.

### 6.3 Application-owned buffer survival

An RX buffer transferred to application ownership shall remain valid until the application releases or transfers it, even if the originating link or LE restarts.

Therefore:

- application-owned buffers cannot be allocated from storage destroyed with the LE runtime;
- restart shall not reclaim them by assuming all old-generation handles are free;
- release after restart shall remain safe;
- stale handles shall be detectable if compact slots can be reused;
- a restart may temporarily reduce available capacity while applications retain buffers, and that condition shall be bounded and observable.

An implementation may avoid this complexity by copying into application-owned storage before transfer, but it shall not invalidate an accepted ownership contract.

## 7. Live and latched status

Status has two distinct lifetimes.

**Live status** describes the current runtime generation:

- LE and link lifecycle state;
- enabled/configured/operational flags;
- heartbeat and last progress time;
- current queue/pool use and congestion state;
- current Hardware Link observations;
- counters explicitly scoped to the current generation.

**Latched status** preserves diagnostic evidence:

- last fault code, source, and timestamp;
- first/most severe fault where useful;
- LE and link restart counts;
- last recovery reason, action, and result;
- total bounded error/drop counters intended to survive LE restart;
- suppressed or rate-limited diagnostic count.

Live status may change rapidly and may become unavailable while an LE is faulted. Latched status remains readable from storage outside the failed runtime until its defined clear/reset boundary.

Snapshots shall include enough generation/sequence information to prevent consumers from combining unrelated fields from different updates.

## 8. Device-wide aggregate status service

Each device exposes **one aggregate Link Entity Status service**, not one independent network service per LE. It reports every local LE and each LE-owned runtime link.

The aggregate service:

- reads coherent live snapshots where available;
- reads latched records that survive failed-runtime reconstruction;
- identifies LE/link instances using stable configuration identifiers;
- distinguishes configured-disabled, starting, healthy, degraded, faulted, and unavailable observations;
- includes status age and runtime generation where needed;
- does not infer that a silent failed LE is healthy merely because its live snapshot stopped changing.

The service schema and endpoint allocation remain open. The application/service conventions are defined in [Application Protocols and Services](application_protocols_and_services.md).

## 9. Publication over healthy paths

Aggregate status shall be publishable over every currently healthy, configured path authorized to carry it. A failed link is not required to report its own failure.

For example, a healthy link owned by one LE may publish the latched fault and unavailable live state of another LE or link. Publication configuration remains static and subject to normal routing, capacity, priority, and security policy.

Requirements:

- status publication shall not depend exclusively on the failed path;
- no path is used unless configured and healthy enough to accept the PDU;
- failure to publish remotely does not erase local live/latched truth;
- each status-transmission outcome shall update its applicable local bounded counters, but shall not by itself trigger or schedule status publication;
- publication shall be driven only by the configured cadence or a valid explicit status request;
- each destination shall have at most one coalesced pending summary; a newer due summary updates that pending summary rather than creating another queued summary;
- a malformed status request or response, or any status publish failure, shall be recorded locally and shall never trigger a remote error, immediate status response, or another status publication;
- repeated faults are aggregated and rate-limited;
- diagnostic responses do not recursively generate diagnostic responses;
- status traffic remains bounded and shall not destabilize healthy control traffic.

A low-rate cadence-driven summary plus explicitly requested bounded detail is the required prototype pattern. The exact cadence, priority, destinations, and detail pagination are service-schema decisions.

## 10. Bounded diagnostics

Diagnostic storage and processing shall have fixed bounds. Implementations shall define:

- counter widths and saturating/wrapping behavior;
- maximum retained fault records;
- record replacement policy;
- maximum detail-response size and pagination/cursor behavior;
- publication rate and burst limits;
- suppression and dropped-diagnostic counters;
- clear authorization and reset boundary.

Per-byte, per-frame, or repeated identical fault logging shall not consume unbounded memory, CPU, or bandwidth. Local counters are the source of truth; remote diagnostics are a bounded projection.

## 11. Prototype restart and fault tests

### 11.1 Core prototype required tests

Every prototype shall validate:

1. Individual link stop/start, driver-fault detection, and successful link-only recovery.
2. Failed link recovery escalating through bounded quiesce and local LE reconstruction.
3. Healthy sibling links continuing through another link's recovery when the prototype configuration contains independently recoverable siblings.
4. A hung LE being detected by an external supervisor.
5. Partial RX, partial TX, pending timers, and queued work at each local restart boundary, including an explicit cancellation/completion result for every affected accepted TX.
6. Application-owned RX buffers surviving LE reconstruction and being released afterward, including bounded buffer/pool exhaustion caused by retained old-generation buffers.
7. Bounded ingress, egress, completion, and control queues reaching capacity and racing with local restart without corruption, ownership loss, or unbounded work.
8. Live snapshot generation changing without torn mixed-generation status.
9. Latched fault/restart evidence surviving LE reconstruction and obeying its defined clear/reset boundary.
10. Faulted-path status using another healthy configured path when one exists, and aggregate-service behavior when one or several LEs are silent.
11. Cadence- and request-driven status publication, one coalesced pending summary per destination, local transmission-outcome counters, and absence of diagnostic feedback publication.
12. Repeated fault injection with bounded records, rates, and suppression counts.
13. Configured-disabled links not being misreported or repeatedly recovered as faults.

### 11.2 Conditional platform tests

The following tests are required only when the prototype platform supports or selects the corresponding capability:

1. Independent debug-path operation while the normal LLL, router, pool, or status publisher is unavailable.
2. Restart races with DMA ownership, completion, cancellation, descriptor ordering, and required cache maintenance.
3. Recovery escalation from LE restart to MCU/device reset under its configured attempt limits and policy.
4. Cross-core queue and snapshot restart behavior on a non-coherent multicore platform, including required clean/invalidate and release/acquire operations.

A prototype that claims one of these capabilities shall run its conditional tests; an unselected or unsupported capability shall be recorded as not applicable rather than treated as missing core evidence.

## 12. Open schema and API issues

The following remain unresolved:

- exact LE and link lifecycle enums and transition/result APIs;
- ownership and timeout rules for asynchronous lifecycle commands;
- heartbeat/progress definition and external-supervisor interface;
- dependency groups for links that cannot restart independently;
- accepted-TX cancellation and completion result vocabulary;
- runtime generation representation and wrap behavior;
- persistent counter/record storage location and reset boundaries;
- coherent snapshot mechanism across tasks/cores;
- aggregate service endpoint ID, schema version, message sizes, and pagination;
- stable LE/link identifier allocation and representation;
- timestamps, clock domain, validity, and behavior before time synchronization;
- common versus profile-specific status fields;
- severity, fault-code registry, and extensibility rules;
- authorization for clear, restart, disable, and detailed-debug operations;
- exact healthy-path selection and duplicate publication policy;
- behavior when the aggregate service's own execution context fails;
- restart interaction with cross-core queues, non-coherent caches, and DMA;
- application-owned handle generation and allocator reclamation after restart.

These issues block stabilization of the runtime API or status wire service, but they do not change the ownership and recovery principles above. System-level fault-injection and promotion evidence belongs in [Prototype and Validation](prototype_and_validation.md).
