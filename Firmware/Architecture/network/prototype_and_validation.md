# Prototype and Validation

**Status:** Actionable prototype plan  
**Maturity:** Disposable validation implementation; no stable API or wire-compatibility promise  
**Security posture:** Trusted local/private development environment only

This prototype proves the smallest coherent embedded message fabric before production services or additional transports are frozen. It implements [application protocol and service](application_protocols_and_services.md) semantics over the addressing model in [core protocol and routing](core_protocol_and_routing.md) and the contracts in [logical links and transports](logical_links_and_transports.md). The [README](README.md) and [architecture overview](architecture_overview.md) define authority and scope.

The four application protocols below are disposable test instruments. Their message names, APIs, endpoint IDs, and serialized layouts are provisional. The prototype must record semantic requirements and generate repeatable vectors, but it must not present an expedient encoding as a final ecosystem profile.

## 1. Initial implementation boundary

The first end-to-end slice contains:

- canonical in-memory endpoint/message metadata;
- literal endpoint identity using the 2-bit namespace and IDs `0x0100..0xFFFF`;
- explicit upstream/downstream service roles;
- Unreliable Datagram only;
- Service Router registration, validation, dispatch, and bounded TX results;
- latest-value snapshot and bounded event-queue delivery;
- single-writer and multi-caller TX paths;
- static VC, endpoint, peer, listener, forwarding, and replication configuration;
- loopback/in-process and shared-memory local links for
  `VirtualCircuit::kLocalHost` with its `UpstreamHost::kLocalHost` companion
  marker;
- one external reference link profile/technology, with as many configured link instances/segments as the fixtures require;
- packet/message capture, counters, fault injection, and link/restart diagnostics.

The default external profile is a process-isolated virtual datagram link with multiple harness-controlled ports. Separate processes and configured virtual ports provide external-link boundaries, independent capture, and multiple link instances/segments without exposing an unauthenticated prototype carrier to a physical or routed network. “One external link” means one external link profile/technology, not one link object, socket, port, bus, or segment.

Non-loopback UDP is not the default. Gate 0 may approve it only as an exception on an isolated, non-routed test network with exact local-interface and peer allowlists, a host firewall, and NAT and IP forwarding disabled. Its profile, capture, and report must prominently state that the carrier is unauthenticated and must identify the approved interfaces and peers.

Reliable transport is deferred. Reliable, CAN, and HDLC completion are not initial dependencies or acceptance prerequisites. HDLC and CAN remain experimental and are added only after their parsers, integrity rules, bounds, and negative vectors are ready enough not to obscure validation of the common architecture. The [CAN PDU adapter](can_pdu_adapter_spec.md) is not an initial dependency, and reports retain these expected gaps rather than implying that the corresponding production paths were validated.

## 2. Disposable validation protocols

All four protocols use a provisional canonical schema and begin semantically with `protocol_version` and `message_type`. The deployment assigns nonfinal literal endpoint IDs; no alias is visible to service code. Each protocol declares bounded sizes, rates, storage, and TX-rejection behavior before implementation.

Producer attribution is expressed as the configured `ProducerKey` plus recorded provenance. It is not inferred from route metadata or represented by a generic network-source label. Gate 1 must define the prototype's working `ProducerKey` metadata, derivation, and provenance rules so tests are reproducible, but this does not reserve or freeze a source field in a future wire profile.

### Protocol A — latest-state publication

Purpose: prove one-producer/many-listener publication, newest-value storage, freshness, and congestion coalescing.

Semantic messages:

- `StateSnapshot`: current bounded state, validity, producer sequence or epoch, and a defined time/age basis.

Behavior:

- one upstream publisher is authoritative;
- configured same-link and local listeners retain only the newest valid snapshot;
- missed intermediate values are acceptable and observable through sequence gaps;
- stale state becomes invalid after a configured age;
- one pending TX value is retained; a newer value replaces it under congestion;
- producer restart and sequence discontinuity are distinguishable from ordinary loss.

No acknowledgment or reliable delivery is implied.

### Protocol B — command, feedback, and asynchronous event

Purpose: prove directed roles, request-scoped reply context, autonomous downstream transmission, and mixed storage policies.

Semantic messages:

- `Command`: bounded upstream intent with command correlation and freshness/precondition data;
- `CommandFeedback`: immediate acceptance or service-level rejection;
- `CurrentFeedback`: latest achieved state;
- `CompletionEvent`: bounded asynchronous downstream completion/failure event.

Behavior:

- immediate feedback uses the request-scoped reply context;
- periodic/current feedback uses a statically configured downstream-to-upstream path;
- completion events use static or explicitly learned peer binding, never an expired request context;
- request-scoped responses and autonomous streams/events use separate endpoint registrations; the prototype does not combine their binding modes in one registration;
- the downstream service remains in the downstream role while sending feedback or events to an endpoint in the upstream role; traffic direction does not reverse either endpoint's configured role;
- duplicate and stale commands have explicit service behavior even though the transport does not retry;
- current feedback uses a latest-value slot; completion events use a bounded FIFO;
- local TX rejection does not advance command acceptance or event state.

The prototype does not claim exactly-once execution.

### Protocol C — minimal RPC

Purpose: prove bounded request/response dispatch, correlation, service errors, and timeout behavior without hiding reliability inside the application.

Semantic messages:

- `Request`: operation discriminator, correlation value, and bounded arguments;
- `SuccessResponse`: matching correlation and bounded result;
- `ErrorResponse`: matching correlation and compact service reason.

Behavior:

- one caller targets one configured service instance;
- the callee replies using request-scoped context;
- caller state has a fixed maximum number of outstanding requests;
- unknown operation, malformed arguments, busy state, and unsupported version/type are explicit;
- timeout ends the local attempt; the Unreliable Datagram prototype performs no automatic retry;
- any optional caller retry is a test policy and requires the chosen operation to be idempotent or carry an application transaction key.

### Protocol D — DRIP-style fault/event reporting

Purpose: prove bounded diagnostics that remain observable without creating an error storm.

Semantic messages:

- `HealthSummary`: periodic counts, highest severity, last code, lost/suppressed counts, and restart state;
- `DetailRequest`: bounded range/cursor request;
- `DetailResponse`: bounded record batch plus continuation state;
- optional `FaultEvent`: sequenced best-effort event for low-latency observation.

Behavior:

- local counters and a bounded record ring are authoritative;
- summary publication continues at a configured modest rate;
- detail is pulled explicitly, pauses on congestion, and resumes from bounded cursor state;
- repeated faults may aggregate; suppression and record loss remain visible;
- diagnostic failures and malformed requests never generate recursive remote errors;
- diagnostic traffic cannot block or outrank the state/command traffic used by the fixture.

## 3. Deployment fixtures

Each fixture is generated from the executable static topology/configuration schema established at Gate 0 and has a capture point at every link boundary. Before an acceptance run, its manifest fixes the enabled capabilities and bindings, queue depths, overflow policy, TX-result vocabulary, multi-caller ordering, clock/test values, metric registry, capture schema, counter semantics, expected counter deltas, fault seeds/tolerances, and resource budgets required by Gates 3 and 4.

### F1 — same-core `VirtualCircuit::kLocalHost`

Run all four protocol pairs in one process or one MCU core. Messages traverse
the router and loopback LLL rather than calling service implementations
directly. This proves API consistency, dispatch, storage policies, and
deterministic host tests. `VirtualCircuit::kLocalHost` selects the local domain
and carries the required `UpstreamHost::kLocalHost` companion marker; the core
protocol owns their representation and encoding details.

### F2 — cross-core `VirtualCircuit::kLocalHost`

Place producer/client and consumer/server on different cores or core-simulating
execution contexts connected by bounded shared memory. The paired local VC and
host marker never reach an external LLL. Exercise coherent and, where
supported, explicit cache-maintenance configurations.

### F3 — several Main hosts on `kMainCompute`

Use at least three Main instances. Exercise directed A/B/C/D service instances between selected Mains, passive observation where configured, and independent `UpstreamHost` identities. `kMainCompute` uses the normal endpoint/service model; it is not a separate inter-Main protocol.

### F4 — same-bus listeners

Connect one Main and several ECU fixtures to one externally carried logical bus. An ECU publication retains its configured `ProducerKey` and producer-attribution provenance while the Main and other configured ECUs listen because the link is broadcast-capable. Listeners do not become addressed peers, send acknowledgments, or acquire reply context.

This fixture must demonstrate that the complete downstream target is `VCN + Index` and that `UpstreamHost` identifies a Main rather than serving as a general ECU address.

### F5 — configured multi-bus state replication

Place members of one state-sharing group on at least two buses. A statically configured replicator consumes Protocol A state and originates bounded copies on the configured destination VCs. Its manifest declares the eligible Protocol A message types, source and destination `ProducerKey`s, authority owner, provenance transformation, and conflict policy. Validate loop-free fan-out, producer attribution, capacity accounting, conflict handling, and restart behavior. Negative fixtures reject replication of Protocol B commands, Protocol C RPC, Protocol C/D errors, and any undeclared message type, and reject a competing producer for a single-authority destination.

This is an explicit application/deployment pattern. It does not provide arbitrary MCU-X-to-MCU-Y unicast, infer routes from endpoint IDs, or create general ECU routing across buses.

### F6 — same-VC forwarding across segments

Connect two configured link instances/segments of the same external link profile and install only the declared same-VC forwarder between them. For every forwarded datagram, the canonical descriptor, payload bytes, `ProducerKey`/origin context, peer context, and transport state must remain unchanged end to end. Only hop-local LLL framing or envelope data may change. Captures on both segment boundaries prove byte/field equality and show that the forwarder performs no application decode/re-encode, producer rewrite, peer promotion, transport transition, or implicit route creation.

## 4. Prototype layering and components

Implement components in separable layers:

```text
Disposable A/B/C/D protocol semantics and provisional schemas
    |
Typed codecs and validation
    |
Endpoint policies: callback, snapshot, event FIFO, TX funnel
    |
Service Router: identity, role, peer context, dispatch, static VC policy
    |
Unreliable Datagram
    |
Loopback / shared-memory / external datagram LLLs
    |
Platform driver, scheduler, clock, and memory adapters
```

Cross-cutting components are:

- generated or declarative static configuration;
- fixed-capacity packet/message storage;
- capture with canonical metadata and raw-link records;
- metrics, persistent restart/fault counters, and DRIP reporting;
- Link Entity aggregate status and bounded status delivery;
- deterministic fault-injection hooks;
- host reference codecs and conformance runner.

The common layers must not depend on UDP socket concepts, CAN arbitration fields, or shared-memory pointers. Link adapters reconstruct the same canonical metadata before router delivery.

## 5. Implementation phases

### Phase 0 — freeze the experiment contract

- Resolve Gates 0 and 1 before implementation beyond isolated comparison experiments. Resolve Gate 2 before enabling its binding features, Gate 3 before concurrent fixture runs, and Gate 4 before acceptance runs.
- Produce an executable static topology/configuration schema that defines nodes, link profiles and instances/segments, VC attachments and routes, endpoint registrations and roles, peer and listener bindings, forwarders, replicators, queues and bounds, capability/fingerprint declarations, `ProducerKey`s, and a registry of stable validation reason IDs.
- Provide valid minimal and full fixture manifests plus invalid fixtures for conflicting roles or producers, illegal bindings, missing routes/attachments, incompatible capabilities/fingerprints, out-of-range bounds, and unauthorized forwarding/replication.
- Assign temporary literal endpoint IDs, working `ProducerKey`s, and experimental profile labels in generated fixture configuration.
- Write exact experimental semantic schemas, bounds, malformed-input rules, and resource budgets for A–D.
- Create initial positive and negative golden vectors before endpoint behavior is implemented.
- Define capture records, metric names, counter semantics, and deterministic expected deltas needed by every later phase.
- Publish a responsibility/ownership map for codec, endpoint, Service Router, transport, LLL, and Link Entity boundaries. For each boundary it identifies the accepted representation, validation owner and checks, buffer/state ownership transfer and lifetime, and the component that emits the stable rejection/error reason and increments/captures it.

### Phase 1 — same-core vertical slice

- Implement fixed storage, Unreliable Datagram pass-through, loopback LLL, router dispatch, and capture.
- Implement Protocol A end to end, then B/C/D.
- Prove request-scoped replies, static autonomous TX, snapshots, event queues, and rejection results.
- Run deterministic malformed-input and congestion tests.

### Phase 2 — cross-core local path

- Add the bounded shared-memory LLL and platform memory/coherency adapter.
- Repeat the A–D suite across execution contexts.
- Measure copies, atomics, synchronization, wakeups, latency, and memory cost.
- Restart either side while the other remains active.

### Phase 3 — external link and topology

- Add the selected process-isolated virtual datagram/multi-port LLL, or the explicitly approved Gate-0 exception.
- Run F3 and F4, including several Mains and passive same-bus listeners.
- Add the explicit Protocol A replicator and run F5.
- Add the declared same-VC forwarder across two link instances/segments and run F6.
- Verify captures can correlate canonical messages across each configured boundary.

### Phase 4 — overload, faults, and recovery

- Execute all negative, congestion, restart, and fault-injection suites.
- Validate persistent counters and DRIP visibility after LLL/router restart.
- Run normal, startup, degraded, diagnostic-burst, and one-node-error-storm load scenarios.
- Produce the resource and capacity report.

### Phase 5 — optional link readiness

Add HDLC or CAN only if the owning experimental specification has exact framing/integrity behavior, bounded parser/reassembly state, malformed-input rules, and golden vectors for the subset being tested. Failures found here feed the owning specification; they do not alter A–D semantics.

## 6. API contracts to prove

The prototype must prove these semantic operations without freezing their spelling or C++ types:

```text
registerEndpoint(identity, role, transport, binding, rx_policy, limits)
submit(message, configured_destination) -> bounded TxResult
dispatch(decoded_message, ingress_metadata, opaque_reply_context)
reply(opaque_reply_context, message) -> bounded TxResult
readLatest() -> coherent value plus validity metadata
tryPopEvent() -> event or empty
notifyTxCapacity()
restartLinkOrEntity(reason)
captureAndCount(outcome)
```

Questions the implementation must answer:

- Which checks occur at registration, submit, LLL receive, transport receive, and dispatch?
- When does ownership of a TX buffer transfer, and when may it be reused?
- How is request context retained for deferred replies, invalidated, and rejected after expiry?
- How do separate registrations keep request-scoped responses distinct from autonomous streams/events?
- How do static bindings, and any enabled eligible learned bindings, authorize autonomous downstream messages without treating route metadata as authentication?
- What ordering is guaranteed among multiple local callers?
- Which operations are safe from an ISR, task, host thread, or another core?
- How are capacity notification, shutdown, and restart races bounded?

An ergonomic implementation is evidence for later API design, not authority to standardize the first shape.

## 7. Validation suites

### Functional and topology tests

- A–D happy paths on F1 and F2, then on the external fixture.
- Every enabled peer-binding mode among receive-only, transmit-only, static, request-scoped, and learned; a fixture does not claim a disabled or unsupported mode.
- Receive-only registrations reject every TX and reply attempt with the declared `TxResult`, while transmit-only registrations never acquire, cache, or expose an ingress peer; dedicated captures and counters prove both behaviors and their exact expected deltas.
- Learned binding, when enabled, is tested only for a statically single-peer registration or a named authenticated-origin path, and only against a preconfigured peer allowlist. Route metadata is never accepted as authentication.
- Protocol B preserves its configured roles: the downstream endpoint remains downstream while feedback, current-state streams, and completion events target the configured upstream role.
- Several Main identities on `kMainCompute`.
- Same-bus observation without peer promotion or acknowledgment.
- Explicit multi-bus replication with no fallback/general route.
- Same-VC forwarding through F6 with invariant descriptor, payload, `ProducerKey`/origin context, peer context, and transport state.
- Service instances using the same protocol under different literal endpoint IDs.

### Negative protocol and routing tests

- truncated, oversized, and otherwise malformed payloads;
- unsupported `protocol_version` and unknown `message_type`;
- invalid enum/range, nonconforming reserved bits, and wrong profile length;
- wrong namespace, endpoint, transport, role/direction, VC, Index, or `UpstreamHost`;
- either local-host symbol without its required companion, either symbol on
  external ingress/egress, and any attempted external route for
  `VirtualCircuit::kLocalHost`;
- alias ID delivered above an LLL, which must be rejected as an implementation/configuration defect;
- unknown endpoint, missing route, incompatible profile, and unexpected ingress;
- forged, expired, reused, or wrong-request reply context;
- for every enabled learned mode, non-allowlisted installation, a non-single-peer unnamed/unauthenticated origin path, route-metadata-only authorization, and bounded-table exhaustion;
- attempted replication of commands, RPC, errors, undeclared Protocol A types, or state from a competing producer;
- listener attempts to reply or affect delivery state.

Parsers are fuzzed within deterministic memory/time bounds. Rejection is counted and captured without recursive diagnostics.

### Loss and fault tests

- drop, duplicate, reorder, delay, and burst loss at each datagram boundary;
- producer, consumer, router, LLL, and link restart at every protocol state;
- partial shared-memory record and interrupted TX ownership transition;
- stale snapshot, sequence wrap/discontinuity, clock jump, and producer epoch change;
- external link unavailable, reconnect, and route disabled during traffic;
- replicator input/output failure and restart without loops or stale replay;
- malformed/babbling source and diagnostic collector unavailable.

### Link Entity aggregate-status tests

- Every aggregate sample identifies the Link Entity generation and status age so a consumer can distinguish current state from stale state after restart.
- A silent or failed Link Entity becomes degraded/failed within its configured observation bound while healthy sibling Link Entities continue carrying their configured traffic.
- Queue exhaustion and recovery produce the declared state, exact counters, and latched evidence; transient evidence remains observable until the declared acknowledgement/reset policy clears it.
- Status is delivered through a healthy configured service path and does not depend on the failed link or entity being reported.
- An independent debug path, DMA-specific behavior, MCU reset testing, and any platform feature not present in the selected target are conditional capabilities. Their tests run only when enabled in the fixture manifest; omission remains an explicit expected gap and does not fail a platform that declares the feature unsupported.

### Congestion and concurrency tests

- reject every submission, reject in bursts, and oscillate capacity;
- verify Protocol A replacement always converges on the newest accepted value;
- fill B/D event rings under each declared overflow policy;
- pause and resume C/D response generation without busy-spin;
- contend all supported multi-caller contexts and verify ordering/ownership rules;
- saturate diagnostic traffic while preserving the configured progress/priority of A/B;
- restart while queues are full and verify disposal versus persistent-state boundaries.

## 8. Metrics and capture

Before acceptance, the Gate-3/Gate-4 fixture manifest freezes the metric registry and capture schema. Each counter declares width, reset and persistence boundary, saturation behavior, and its precise increment point. Every deterministic test names the expected initial values and exact deltas; probabilistic/fault-timing tests additionally name their seed, clock inputs, tolerance, and permitted outcomes.

Collect per endpoint, route, transport, and link:

- RX, TX, dispatch, success, service-error, malformed, and unexpected-ingress counts;
- accepted, rejected, dropped, replaced/coalesced, suppressed, and retried-submission counts;
- snapshot sequence gaps, stale transitions, and producer restarts;
- event queue occupancy, high-water mark, and overflow;
- request latency, dispatch time, TX queue delay, and scheduling jitter where clocks permit;
- route/replication outcomes and per-output failures;
- link/entity state, restart count/reason/result, and time without progress;
- capture loss and diagnostic-report loss.

Capture records include canonical endpoint/routing metadata, configured `ProducerKey` and provenance, peer context, transport state/profile identity, timestamps with clock-domain/quality, stable outcome/reason ID, and raw-link data where needed for adapter conformance. Capture itself is bounded and reports its own drops according to the same declared counter rules.

## 9. Resource accounting

For each fixture and service instance, report:

- static and peak RAM by router, routes, packet pool, snapshot, event queue, TX funnel, capture, and diagnostics;
- stack use and any initialization-only allocation;
- flash/code size by common layer, link adapter, codec, and protocol;
- encoded bytes, packet/frame overhead, expected/maximum rate, burst depth, and resulting link load;
- serialization/deserialization time, routing time, end-to-end latency distribution, and throughput;
- copies, atomic operations, lock/critical-section time, cache maintenance, and scheduler wakeups;
- CPU load at idle, normal, worst configured, and fault/diagnostic scenarios.

Queue depths and performance limits are deterministic fixture inputs, not numbers invented by this document. The prototype passes only against budgets and fault tolerances recorded before the corresponding acceptance or stress run. Steady-state MCU paths use no dynamic allocation.

## 10. Provisional profiles and golden vectors

Every implemented combination has a clearly marked **prototype profile** containing:

- semantic schema revision and protocol version;
- exact A–D message schemas, field bounds, temporary literal EIDs, descriptor packing, byte order, numeric assignments, PDU-length rule, and malformed-input behavior used by the experiment;
- exact virtual-datagram port/segment mapping and transport/LLL configuration;
- experimental profile identity and compatibility-fingerprint inputs;
- message and storage bounds;
- alias, integrity, and reserved-field policy;
- working `ProducerKey` metadata/derivation and provenance rules, explicitly without claiming a frozen wire source field.

Working labels must contain an experimental/prototype marker and must not use `v1` or imply publication. A changed interpretation creates a new working label so captures remain reproducible.

Golden-vector sets contain:

- valid minimum, typical, maximum, boundary, and each-message-type examples;
- byte-exact encoded PDUs plus expected decoded values, canonical metadata, `ProducerKey`/provenance, peer context, and transport state;
- malformed/truncated/oversized cases and expected rejection stage/reason;
- reserved-bit, unknown-version/type, alias-expansion, and routing mismatch cases;
- restart/sequence/correlation transitions that require multiple messages.

At least two independently exercised codecs—initially the target C++ path and a host reference implementation—must agree on positive and negative vectors. FPGA logic may consume the same vectors later. Vectors become candidates for immutable profiles only after the owning consolidated specification resolves its open wire decisions.

## 11. Acceptance criteria

The initial prototype is accepted when:

- A–D pass their happy, negative, loss, congestion, and restart suites on F1 and F2 and over the selected external link;
- F3 proves several Main identities and normal services over `kMainCompute`;
- F4 proves same-bus listeners without peer promotion, acknowledgment, or loss of configured `ProducerKey`/producer attribution;
- F5 proves only declared Protocol A replication, provenance/conflict policy, rejection of ineligible types and competing producers, and rejection of an attempted general cross-bus ECU route;
- F6 proves same-VC forwarding across two link instances/segments while only hop-local framing changes;
- Protocol B retains downstream/upstream role assignments for request-scoped feedback and autonomous streams/events;
- every enabled binding mode has bounded, tested authorization and lifetime behavior, including observable receive-only and transmit-only criteria; learned binding passes only under the Gate-2 eligibility and allowlist rules;
- aliases are absent above LLL boundaries and all service IDs are canonical literals;
- each TX-capable endpoint has a tested rejection policy and no path busy-spins or grows storage without bound;
- snapshots are coherent, event overflow is exact and observable, and multi-caller ownership/order matches its contract;
- restarts discard uncertain transient state while preserving configured persistent diagnostic evidence;
- nonrecursive diagnostics remain bounded during a babbling-node/error-storm test;
- captures and metrics explain every injected loss, rejection, overflow, and restart without themselves destabilizing traffic;
- Link Entity aggregate status proves generation/age handling, silent/failed detection, healthy sibling continuity, queue-exhaustion behavior, latched evidence, and delivery through a healthy configured path;
- deterministic oracle data records the fixed queue/clock/test values, counter starting points and exact deltas, captures, reason IDs, fault seeds/tolerances, and resource-budget outcomes for every acceptance run;
- both reference codecs pass the same golden vectors;
- measured RAM, flash, CPU, latency, and bandwidth meet the predeclared fixture budgets;
- no steady-state MCU execution requires dynamic allocation;
- unresolved decisions, conditional capability omissions, expected Reliable/CAN/HDLC gaps, and experimental profile labels remain visible in generated reports and documentation.

Passing validates architectural feasibility only. It does not establish safety integrity, security, or production readiness.

## 12. Explicit exclusions

The initial prototype excludes:

- Reliable transport, reliable RPC, reliable events, and exactly-once claims;
- final E2E protection, cryptographic authentication, confidentiality, or replay protection;
- production Internet/remote access or a general secure gateway;
- dynamic discovery/configuration, runtime route creation, a broker, or general internetwork routing;
- arbitrary ECU-to-ECU unicast across buses;
- transparent conversion between incompatible application schemas;
- bulk object transfer, bootloading, firmware activation, and persistent audit delivery;
- final CAN/HDLC profiles, unless admitted as bounded Phase 5 experiments;
- final service catalog allocations, final code-generation language, or frozen public APIs;
- safety certification or production qualification.

Privileged services, arbitrary packet injection, raw memory access, shells, and update paths are not included.

## 13. Unresolved-decision gates

Work pauses at a gate only when the choice changes semantics, interoperability, or the validity of a later measurement.

### Gate 0 — platforms and external link

Select host/MCU targets, clock sources, scheduler model, and the one external link profile/technology. The default is local host tests plus the process-isolated virtual datagram/multi-port link, instantiated as every configured segment required by F3–F6.

Gate 0 also approves an executable static topology/configuration schema defining nodes, link instances/segments, VC attachments and routes, endpoints and roles, peers, listeners, forwarders, replicators, queues/bounds, capabilities/fingerprints, `ProducerKey`s, and stable validation reason IDs. It includes schema validation and executable invalid fixtures. A non-loopback UDP selection is an exception and is valid only with an isolated non-routed network, exact interface/peer allowlists, host firewall rules, no NAT or forwarding, and the required unauthenticated-carrier warning.

### Gate 1 — experimental serialization

Freeze a complete, explicitly experimental prototype profile: exact A–D schemas and bounds, temporary literal EIDs, descriptor packing, byte order, numeric assignments, PDU-length rule, virtual-datagram mapping, malformed-input behavior, profile identity, compatibility-fingerprint inputs, working `ProducerKey` metadata/derivation and provenance rules, and byte-exact positive and negative vectors. The `ProducerKey` work must not freeze or imply a final wire source field.

Gates 0 and 1 authorize a reproducible experiment, not a stable wire standard. Implementation beyond isolated comparison experiments cannot begin until both gates are resolved; any changed interpretation receives a new experimental profile identity and vectors.

### Gate 2 — peer and reply lifetime

Choose request-context retention/expiry and authorization checks for autonomous downstream TX. If learned binding is enabled, define its bounded capacity/eviction/restart policy and restrict installation to a statically single-peer registration or named authenticated-origin path plus a preconfigured peer allowlist. Route metadata is not authentication. Request-scoped responses and autonomous streams/events remain separate endpoint registrations.

### Gate 3 — local concurrency

Choose snapshot algorithm, shared-memory ownership, atomics/memory ordering, cache coherency, ISR rules, deterministic queue depths, and multi-caller ordering. Freeze the fixture clock domains, tick/rate/test values, counter widths and reset/saturation/increment semantics, and the concurrency capture fields and expected deltas before running the corresponding fixture.

### Gate 4 — congestion and scheduling

Freeze the tested TX-result vocabulary, capacity-notification mechanism, each queue's overflow policy, QoS scheduling mode, metric registry, complete capture schema, remaining counter semantics and exact expected deltas, deterministic fault seeds and tolerances, and predeclared load/resource budgets. Acceptance runs use only versioned manifests containing those values; changing one invalidates the affected run.

### Gate 5 — optional HDLC/CAN admission

Admit only the exact experimental subset with complete bounds, integrity coverage, parser/reassembly failure behavior, and vectors. Otherwise defer it without blocking prototype acceptance.

### Gate 6 — promotion

After acceptance, decide which APIs, schemas, services, and profiles merit redesign or promotion. Reliable transport begins only with a concrete reliable workload and separate state-machine/resource design.

Until a gate is resolved, the implementation may compare alternatives in isolated tests but must not silently select one as architecture.

## 14. Local/private security controls

The prototype assumes trusted participants on physically controlled hardware, loopback, shared memory, and process-isolated virtual ports. That assumption is a limitation, not authentication.

- Prefer the process-isolated virtual datagram/multi-port link so the default fixture does not expose a network carrier.
- If Gate 0 admits non-loopback UDP, bind only to the exact approved local interfaces and peers on an isolated non-routed network, enforce host firewall rules, disable NAT and IP forwarding, and do not expose it to the Internet.
- Keep route and endpoint configuration static and allowlisted.
- Disable raw injection and fault controls outside the test harness.
- Separate capture files and debug access from ordinary service authority.
- Rate-limit malformed input and diagnostics even on the private network.
- Record the absence of authentication/confidentiality in every profile and test report, and display the explicit unauthenticated-carrier warning for a UDP exception.

Any trust-boundary crossing requires a separately reviewed gateway/security design; see [rationale, use cases, and risks](rationale_use_cases_and_risks.md).
