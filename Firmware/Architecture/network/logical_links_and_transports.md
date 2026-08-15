# Logical Links and Transports

## Status and authority

This document defines the **stable generic software contracts** for Logical Link Layers (LLLs), Hardware Links, and the currently supported transport vocabulary. It does not stabilize any particular wire encoding. The overall layering and scope are summarized in [Architecture Overview](architecture_overview.md).

Wire interoperability exists only under a separately named, immutable profile. The serial candidate is documented in [HDLC Logical Link Profile](hdlc_logical_link_profile.md) and is explicitly experimental. Link ownership, restart, and device-wide status are defined in [Link Entity Runtime and Status](link_entity_runtime_and_status.md).

The canonical descriptor, endpoint identity, virtual-circuit semantics, routing, and forwarding rules belong to [Core Protocol and Routing](core_protocol_and_routing.md). Application delivery policies belong to [Application Protocols and Services](application_protocols_and_services.md). This document uses those contracts rather than redefining them.

Normative terms such as **shall** and **must** apply to the stable generic contracts unless a section is marked Provisional, Experimental, Non-normative, or Open.

## 1. Layer boundaries

```text
Application / Service
        |
Endpoint / Service Router
        |
Transport
        |
Logical Link Layer
        |
Hardware Link
```

The boundaries are:

- A **transport** adds delivery semantics to one bounded application message.
- An **LLL** maps one canonical PDU value to and from one configured link profile.
- A **Hardware Link** moves profile-specific bytes, frames, datagrams, or equivalent transfer units. It has no endpoint or service semantics.
- A **Link Entity (LE)** owns and serializes the mutable runtime for one or more LLL and Hardware Link instances. An LE does not own an entire virtual circuit; see [Link Entity Runtime and Status](link_entity_runtime_and_status.md).

Physical signaling, transceivers, modulation, and electrical characteristics are outside the generic contracts.

## 2. Canonical bounded-PDU service

### 2.1 Canonical PDU value

At the transport/LLL boundary, one PDU is a discrete semantic value containing:

- the canonical descriptor and routing metadata defined by the core protocol;
- any transport-specific header or status;
- one bounded application payload.

The canonical PDU value is not necessarily a contiguous byte array, and it is not necessarily identical to a wire header. A link profile may encode canonical fields into link-native metadata, omit fields that static configuration reconstructs unambiguously, or serialize the complete value. Receive processing shall reconstruct the same canonical value before delivery upward.

An LLL shall not reinterpret application payload bytes. It shall not terminate end-to-end transport state.

### 2.2 Message boundaries and bounds

The LLL service is message-oriented:

- One accepted TX request represents exactly one complete PDU.
- One successful RX delivery represents exactly one complete, link-validated PDU.
- Partial PDUs shall not be delivered upward.
- The generic LLL does not segment an arbitrarily large PDU. A profile-specific adapter may fragment and reassemble internally only when that behavior is part of the named profile.
- Every configured instance shall expose finite TX and RX PDU bounds.
- Length and profile compatibility shall be checked before an implementation reads or writes outside supplied storage.

The effective application-payload limit depends on the service, transport overhead, canonical metadata encoding, and selected LLL profile. Static configuration tooling should reject combinations that cannot fit.

### 2.3 Receive delivery is not callback-dependent

The stable contract requires a bounded receive-delivery port, not a mandatory callback ABI. Valid implementations include:

- a consumer-owned `try_receive`/drain interface;
- a bounded ownership-transfer queue;
- direct publication into an explicitly configured endpoint storage object;
- an optional short callback adapter layered over one of those mechanisms.

No service or LLL profile may require arbitrary application code to run in the LLL execution context. If callbacks are offered, their execution context, work bound, reentrancy rule, and ownership result shall be explicit.

### 2.4 TX acceptance and completion

A TX submission shall return promptly with an explicit result. At minimum, results shall distinguish:

- accepted into bounded LLL-owned state;
- rejected because capacity is temporarily unavailable;
- rejected because the route/profile is invalid;
- rejected because the PDU or length is invalid;
- rejected because the link is disabled or faulted.

Acceptance means that ownership has transferred as documented and the request entered the local transport/link path. It does **not** mean peer delivery, successful physical transmission, or acknowledgment.

After acceptance, the implementation shall eventually do one of the following:

- complete transmission and release/report the accepted object;
- report cancellation caused by an explicit stop or restart;
- report a terminal local fault.

Completion notification may be polled, queued, or delivered through an optional bounded callback. The implementation shall define whether completion means queued to hardware, consumed by hardware, or physically completed. The generic contract does not equate any of those events with end-to-end delivery.

## 3. Ownership and borrowing

Every PDU buffer has one logical owner at a time.

Two API forms are permitted:

- **Borrowed view:** immutable for a documented call or lease scope. The borrower shall not retain it after that scope or mutate the backing storage.
- **Owned handle:** exclusive ownership is transferred explicitly. The receiver may retain or enqueue the handle and shall eventually release or transfer it.

Requirements:

- Ownership transfer shall be mechanically visible in the API.
- A rejected TX submission leaves ownership with the caller.
- An accepted owning TX submission transfers ownership to the receiving layer.
- A borrowed TX submission may be accepted only if the implementation copies the required bytes before return or documents a bounded lease extending through a later completion event.
- RX storage retained by application code shall remain valid until released, including across task/core transfer and LE restart. The storage allocator therefore needs a lifetime broader than restartable LE state.
- Buffer reclamation shall occur through the allocator's defined owner or synchronization mechanism; arbitrary contexts shall not mutate a shared free list.
- Mutable access after publication is prohibited unless a separate, explicit protocol transfers ownership back.

Zero-copy operation is desirable but not required. Correct ownership and bounded behavior take precedence.

## 4. Stable Logical Link contract

For each configured instance, an LLL shall:

1. accept or reject complete canonical TX PDU values without unbounded waiting;
2. encode accepted values according to one configured named profile;
3. consume Hardware Link RX units incrementally;
4. validate profile framing, lengths, and link-level integrity before parsing untrusted PDU fields;
5. reconstruct canonical PDU values and deliver only complete valid values upward;
6. expose bounded progress, drop, parse, integrity, oversize, congestion, and lifecycle diagnostics;
7. support explicit start, stop, reset, and fault handling through its owning LE;
8. preserve message ordering only to the extent promised by the selected profile and Hardware Link;
9. keep application/service dispatch outside Hardware Link drivers;
10. avoid hidden dynamic allocation requirements.

An LLL may exploit link-native addressing or priority metadata, but the resulting canonical value shall obey the core protocol. A transparent forwarding path is compatible only when every hop can preserve or unambiguously reconstruct that value.

## 5. Stable Hardware Link contract

A Hardware Link supplies the smallest useful transfer service below an LLL. Its profile-specific RX/TX unit may be a byte span, frame, datagram, FIFO entry, DMA descriptor, or shared-memory record.

Each instance shall provide:

- bounded RX acquisition or draining;
- bounded TX acceptance with explicit temporary rejection;
- explicit transfer-unit lengths and capacity;
- lifecycle control or integration with equivalent driver lifecycle control;
- fault and progress observations;
- a way to wake or notify the owning execution context when useful;
- ownership and completion semantics for every submitted storage object.

A Hardware Link shall not:

- parse endpoint IDs, transport headers, or application payloads;
- dispatch services;
- claim end-to-end delivery;
- silently retain caller-owned storage beyond its documented lease;
- invoke unbounded application work from an ISR.

### 5.1 Capability description

Each configured Hardware Link and LLL shall expose an immutable capability description sufficient for static composition. Relevant capabilities include:

- RX and TX transfer-unit kind;
- maximum RX and TX unit size;
- maximum decoded/reassembled PDU size;
- required alignment and contiguous-storage constraints;
- scatter/gather and zero-copy support;
- full-duplex, half-duplex, or simplex behavior;
- ordering and possible duplication behavior;
- link-provided error detection, if any;
- whether TX completion and timestamps are available;
- whether DMA, ISR production, cross-core access, or cache maintenance is involved;
- bounded queue depth or other acceptance limits.

Capabilities describe the local implementation. They are not a substitute for a wire profile and do not imply runtime negotiation.

## 6. Named immutable profiles and compatibility

Every interoperable LLL configuration shall select a named, versioned profile. A stabilized profile is immutable. Any incompatible change requires a new name/version.

A profile shall specify all interoperability-relevant behavior, including:

- exact field and transfer-unit layout;
- byte and bit order;
- framing, escaping, padding, and reserved-value rules;
- PDU length interpretation and all minimum/maximum bounds;
- integrity algorithm, field order, coverage, and validation order;
- fragmentation/reassembly and timeout behavior, if present;
- mapping between canonical fields, static configuration, and link-native metadata;
- malformed-input and resynchronization behavior;
- required diagnostics;
- representative valid, boundary, and invalid conformance vectors.

Peer compatibility is a static deployment property. Peers shall use the same named profile and compatible bounds/configuration. Profiles are not auto-detected from traffic, and the base architecture does not require profile negotiation.

Static validation should reject:

- profile-name/version mismatch;
- incompatible PDU bounds;
- canonical fields that cannot be preserved or reconstructed;
- incompatible aliases or static mappings;
- service/transport messages that exceed the effective capacity;
- unsupported Hardware Link capability requirements.

## 7. Execution and concurrency

### 7.1 One serialized mutable LLL context

Each LLL instance has exactly one logical execution context that mutates parser state, reassembly state, TX scheduling state, timers, and its owned queues/pools. It may run in a bare-metal loop, a dedicated task, or another explicitly serialized executor.

Other tasks or cores shall communicate through bounded submission ports, ownership-transfer queues, immutable snapshots, or explicitly synchronized control requests. They shall not concurrently call arbitrary state-mutating LLL operations.

Read-only status may be exposed concurrently only through a coherent snapshot or other documented synchronization mechanism.

### 7.2 Interrupt context

ISR work should be limited to:

- acknowledging hardware;
- capturing a bounded RX unit or completion record;
- publishing it to the owning context;
- waking that context.

Parsing complete PDUs, endpoint dispatch, arbitrary callbacks, blocking, and general allocator/free-list manipulation shall not occur in an ISR unless an implementation defines and verifies a stricter ISR-safe profile. Such an exception does not weaken the generic contract for other implementations.

ISR-to-task publication requires the platform's documented atomic and memory-ordering operations. `volatile` alone is not a synchronization primitive.

### 7.3 Cross-core execution

Cross-core transfer shall use an explicit publication boundary. Before publishing an owned handle, the producer completes all payload writes and performs any required cache clean/release operation. The consumer performs the matching acquire/invalidate operation before reading.

The queue or mailbox implementation shall define:

- producer and consumer identities;
- ordering and atomic-width assumptions;
- cache/coherency requirements;
- full/empty behavior;
- reset behavior while records are in flight.

An implementation may copy when zero-copy ownership cannot be made safe.

## 8. `kLocalHost` semantic equivalence

`VirtualCircuit::kLocalHost` selects the reserved local-only communication
domain. `UpstreamHost::kLocalHost` is the required companion marker for that
domain; it does not select the domain by itself. Implementations shall reject
either local symbol without the other, and shall never emit either on an
external link or accept either from external ingress.

Same-core and cross-core `kLocalHost` paths shall provide the same externally visible protocol semantics:

- the same canonical PDU value and endpoint identity;
- the same service directionality and configured peer binding;
- the same transport interpretation;
- the same semantic validation and configured local routing policy;
- explicit bounded acceptance and backpressure;
- the same distinction between accepted, rejected, delivered, and consumed.

The implementation mechanism may differ. A same-core path may use direct serialized dispatch; a cross-core path may use shared memory, a FIFO, a queue, or a copy. Optimization shall not make same-core delivery reentrant when the equivalent cross-core path is deferred, unless that execution difference is an explicit endpoint policy.

`kLocalHost` guarantees off-host containment only. The marker does not authenticate callers or isolate tasks, processes, or cores. Authorization requires a platform principal and enforcement boundary, such as MPU/MMU permissions or authenticated IPC. Without such a boundary, all code able to submit to the local path is trusted. Bounds, descriptor, endpoint/service-direction, peer-binding, payload/schema, and routing validation still apply, but those checks shall not be represented as caller authorization. Local corruption protection may be omitted only as a named local profile decision.

## 9. Snapshot and queue semantics are separate

Storage policy is above the generic LLL contract:

- A **snapshot** retains the latest complete value. A newer publication may replace an older unread value. Readers observe a coherent value but are not promised every update.
- A **queue/event stream** retains discrete accepted events in order up to a bounded capacity. Overflow has an explicit reject, drop, or replacement policy.

Neither may masquerade as the other. In particular:

- a snapshot replacement is not a queue delivery;
- queue exhaustion shall not silently become latest-value replacement;
- snapshots and live status require coherent publication;
- event, command, and reliable-protocol state normally require non-replacing storage.

Callbacks are only notification mechanisms and do not define storage or ownership semantics.

## 10. Congestion and bounded backpressure

Temporary TX rejection is a normal runtime condition. No caller may assume submission succeeds, busy-spin indefinitely, or advance protocol state as though a rejected PDU entered the link.

Every transmitting service/transport shall define bounded behavior, such as:

- retain and replace one pending latest value;
- retain exact protocol state and retry later;
- use a bounded FIFO with a defined overflow result;
- drop or aggregate best-effort diagnostics while incrementing a counter.

An LLL should expose bounded capacity/congestion observations such as available slots, occupancy, high-water mark, rejected count, dropped count, and replaced count. Notification of newly available capacity may be polled, scheduled, queued, or callback-based; it is not required to be a callback.

Local TX rejection is distinct from a transport timeout: rejection means the PDU never entered the transport/link path, while a timeout applies after transport acceptance.

## 11. Transport profiles

### 11.1 Unreliable Datagram — stable and completed

Unreliable Datagram is the only completed transport contract in the initial architecture.

It provides:

- one bounded discrete message per PDU;
- no transport header;
- no retransmission or acknowledgment;
- no connection state;
- no delivery, uniqueness, or ordering guarantee beyond what lower layers happen to provide.

Successful local acceptance does not imply remote receipt. Applications that need freshness, duplicate handling, transactions, or retry shall define those semantics themselves or select a future standardized transport.

### 11.2 Sequenced/E2E-Protected Datagram — Provisional

Sequenced/E2E-Protected Datagram is a candidate future transport. Candidate metadata includes an alive/sequence counter and an end-to-end check value. Exact fields, coverage, state, reset behavior, acceptance window, diagnostics, and profile ID are unresolved.

No implementation shall claim interoperability from this document. A future profile must define all serialization and conformance vectors before stabilization.

E2E integrity is distinct from LLL integrity:

- LLL integrity detects corruption in one link/profile transfer or reassembly.
- E2E integrity protects selected producer-to-consumer semantics across transparent forwarding and re-encapsulation.

Neither mechanism provides cryptographic authentication.

### 11.3 Reliable Message Transport — wholly TBD

Reliable transport is intentionally undefined. This document defines no reliable header, sequence space, epoch, acknowledgment format, retry algorithm, flow control, timing, connection identity, restart behavior, or wire profile.

Older draft field sketches are not a contract and shall not be implemented as though they were one. Reliable transport shall be designed from concrete service requirements and published as a separate named profile with state-machine and conformance tests.

Until then, services may implement application-specific request/retry behavior over Unreliable Datagram, but shall not label it the standardized Reliable Message Transport.

## 12. Integrity and security distinctions

The following claims are separate:

- **Hardware error detection** concerns one physical/link transfer.
- **LLL integrity** concerns one encoded or reassembled LLL PDU.
- **E2E integrity** concerns accidental corruption or sequencing across the configured service path.
- **Cryptographic authentication** establishes an authorized origin and integrity against an attacker.
- **Confidentiality** prevents unauthorized reading.
- **Replay protection** rejects previously valid authenticated messages under defined freshness rules.

A CRC provides error detection, not authentication, confidentiality, authorization, or cryptographic replay protection. The core stack makes no default cryptographic-security claim. Secure carriers or explicitly designed secure services may be composed at an appropriate trust boundary.

## 13. Non-normative implementation guidance: SPSC pools

One suitable implementation uses statically allocated packet storage, one owner per global pool/free list, and bounded single-producer/single-consumer queues for cross-context ownership transfer.

This can provide deterministic memory use and zero-copy transfer, but it is **not** part of the LLL API or wire contract. Fixed ring SPSC queues, linked pooled SPSC queues, copied mailboxes, or other verified bounded mechanisms are all permitted.

If pooled linked SPSC queues are used:

- each queue has exactly one producer and one consumer;
- the pool/free list has one owner;
- handles have one owner at a time;
- publication is the synchronization boundary;
- pool and queue exhaustion have explicit behavior.

The initial/reference implementation should favor the simplest mechanism that can be verified on the target.

## 14. Open memory-model and coherency requirements

The generic architecture is not implementable safely across every target until the following platform contracts are made explicit:

- minimum atomic widths, alignment, and lock-free assumptions;
- release/acquire operations and required interrupt/core barriers;
- cache coherence assumptions for shared-memory queues and snapshots;
- DMA clean/invalidate ownership transitions and descriptor ordering;
- whether shared packet arenas may be cached;
- allocator lifetime and reclamation across LE restart;
- handle generation/ABA protection when compact indices are reused;
- behavior when a producer or consumer resets with records in flight;
- permitted borrowing across asynchronous DMA and execution-context boundaries;
- MPU/MMU permissions and isolation for shared storage;
- required behavior on non-coherent multicore systems;
- whether diagnostic counters are atomic, serialized, or snapshot-copied.

These are explicit gaps, not implementation details that may be assumed away. Each platform adapter shall document and test its choices. Cross-core and DMA-backed profiles cannot be promoted to stable implementation status until their applicable items are resolved.

## 15. Generic conformance expectations

An implementation of these contracts should be tested for:

- every TX acceptance and rejection result;
- ownership retention on rejection and release after acceptance;
- RX truncation, oversize, malformed, and integrity failures;
- queue/pool exhaustion without unbounded work or corruption;
- parser progress under arbitrary chunking;
- ISR-to-task and cross-core publication ordering;
- restart with Hardware Link and LLL work in flight;
- survival and eventual release of application-owned RX buffers across restart;
- same-core and cross-core `kLocalHost` semantic equivalence;
- snapshot replacement versus queue overflow behavior;
- static rejection of incompatible profile, capacity, and service combinations.

Wire-profile conformance is additional to these generic tests. System-level link, transport, congestion, restart, and archetype coverage belongs in [Prototype and Validation](prototype_and_validation.md).
