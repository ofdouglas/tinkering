# Application Protocols and Services

**Status:** Consolidated design contract  
**Maturity:** Provisional; suitable for prototype alignment, not a frozen API or wire specification  
**Scope:** Bounded application protocols, service instances, local endpoint behavior, and brokerless pub/sub

This document owns application/service semantics for the embedded message fabric. [Core protocol and routing](core_protocol_and_routing.md) owns canonical addressing and Virtual Circuits (VCs); [logical links and transports](logical_links_and_transports.md) owns delivery and link contracts; [prototype and validation](prototype_and_validation.md) defines how the requirements below are proved. The [architecture overview](architecture_overview.md) and [README](README.md) define the wider scope and authority.

The shapes shown here are semantic examples. They are not final APIs, field layouts, or wire encodings.

## 1. Role-directed protocols

Every service protocol defines two stable roles:

- **Upstream** originates stimulus, requests, commands, or orchestration.
- **Downstream** receives that stimulus and performs the responder or controlled role.

The roles belong to the service instance, not to physical topology or device class. An ECU may be upstream for fault reporting while a Main logger is downstream. A downstream endpoint may reply immediately, publish feedback later, or originate substantial asynchronous traffic without reversing the roles.

A genuinely symmetric exchange is composed from two directed service instances, one in each direction. A convenience wrapper may present them as one duplex facility, but the underlying authority, routing, and state remain explicit.

## 2. Endpoint identity and service instances

The canonical endpoint identity is:

```text
(namespace: 2 bits, endpoint_id: 16-bit literal)
```

Namespaces `0..2` are deployment-defined. Namespace `3`: **open ecosystem
namespace; allocation governance remains TBD.** Canonical literal endpoint IDs
are `0x0100..0xFFFF`.

Values `0x0000..0x00FF` are reserved for optional link-local aliases. An LLL
may map a configured alias to a literal ID on receive and reverse the mapping
on transmit, but aliases never reach the Service Router, a transport, a
service, capture in canonical form, or application code. Alias configuration
does not change endpoint identity.

An alias-bearing profile fails closed unless both peers derive from the same
generated topology artifact or use compatible alias digests provisioned by an
out-of-band mechanism. An unknown or mismatched digest disables the alias
profile for the affected peer or link, and traffic under that profile remains
disabled. A separately configured literal-ID profile may be used instead, but
there is no automatic fallback or profile negotiation.

`UpstreamHost` normally identifies a configured Main identity. Local IPC/ICC
instead uses the reserved symbolic `UpstreamHost::kLocalHost`, which is not a
node address and is valid only with `VirtualCircuit::kLocalHost`. These symbols
belong to different field types and their numeric values remain provisional. A
local marker is rejected on external ingress or egress, and either local symbol
without the other is invalid. Source/origin and return-path semantics remain
explicitly open; this document does not infer them from `UpstreamHost`.

An endpoint ID identifies a receiving application-protocol endpoint, not a device, route, service name, or software build. A protocol may use several endpoints, and a deployment may instantiate the same protocol several times under different literal IDs. A service instance therefore binds, at minimum:

- one directed protocol role;
- one or more literal endpoint identities;
- a declared transport and size profile;
- configured peer/routing policy;
- bounded RX, TX, and local-delivery behavior;
- service-specific authority, freshness, and error rules.

Default IDs supplied by a reusable library are conveniences only. The deployment owns final allocation and collision checks.

## 3. Five peer-binding modes

Each endpoint registration declares exactly one of five bounded binding modes.
These modes describe where transmission context comes from; they do not create
general routing.

1. **Static binding** — uses one configured peer, or a fixed bounded set
   selected by application policy, for replies and autonomous transmission.
2. **Learned-from-ingress binding** — retains a finite configured number of
   ingress-derived peers for later transmission, but only under an enabled
   authorization mode below.
3. **Request-scoped binding** — may reply only through opaque context supplied
   with a valid incoming request. The context is bounded, cannot be synthesized
   by service code, and expires at a defined scope boundary.
4. **Receive-only** — accepts configured traffic and never transmits protocol
   messages. A passive listener is the common case.
5. **Transmit-only** — publishes through a statically configured destination
   or VC and does not acquire a peer from ingress.

Learned-from-ingress binding is disabled on unauthenticated multi-access LLLs.
It may be enabled only on a statically single-peer ingress or under a named
authenticated-origin profile, and only within a preconfigured allowlist. The
allowlist constrains permitted origin, service, VC, Index, and interface
values. Route, VC, Index, interface, and descriptor validation are not
authentication. Every enabled learned mode defines finite capacity, expiry,
replacement/eviction, restart invalidation or restoration, and auditable
learn, replace, expire, reject, and clear events. Prototype conformance tests
all enabled binding modes; learned-from-ingress mode is not assumed to be
unconditionally enabled.

Protocols needing richer multi-peer sessions maintain explicit bounded session
state above these primitives. They must not treat arbitrary recent traffic as
permission to transmit.

### Replies and autonomous downstream transmission

The receive path supplies parsed service data separately from an opaque reply context. A request-scoped responder can return a success or service error without interpreting raw `VCN`, `Index`, direction, or `UpstreamHost` fields.

Delayed work must make the lifetime rule explicit. If a response can outlive the callback or dispatch operation, the infrastructure must provide a bounded retained reply handle or require another binding mode; retaining references to transient ingress state is invalid.

Request-scoped context authorizes only the corresponding response flow.
Periodic feedback, completion events, unsolicited alarms, and other autonomous
downstream messages require a static or learned-from-ingress binding configured
for that purpose. A successful request must not silently install a durable peer
unless the endpoint registration is explicitly configured for
learned-from-ingress binding.

A protocol requiring both request-scoped replies and autonomous transmission
uses separate endpoint registrations or explicit bounded named sub-bindings
that each have one declared binding mode, authority, and lifetime. Separate
registrations are recommended for the initial prototype. This is a composition
rule, not frozen API spelling.

## 4. Service message contracts

The default service-message prefix is semantically:

```text
u8 protocol_version
u8 message_type
```

`protocol_version` identifies an incompatible interpretation of the service protocol. It does not change for an implementation-only hotfix. `message_type` distinguishes requests, successful responses, service errors, snapshots, feedback, and events and also helps detect endpoint/profile misconfiguration. A separate magic field is optional, not standard.

Each serious service is defined by a canonical schema over bytes, not by a C/C++ ABI. Its specification must state:

- exact serialization, byte order, bit numbering, signed representation, and length interpretation;
- every field width, range, enum value, and invalid value;
- engineering units, scale, offset, clock domain, and validity metadata where applicable;
- transmitter values and receiver behavior for padding and reserved fields;
- required, optional, and safely ignorable message types;
- malformed-input, unknown-version, unknown-type, and unsupported-feature behavior;
- freshness, sequence, correlation, duplicate, and restart semantics where applicable.

Generated classes and structs are views or codecs derived from the schema; native padding, enum layout, alignment, and endianness never define the wire contract. Decoders must safely handle unaligned data and reject lengths before field access. Bounded arrays, strings, and opaque byte sequences are allowed; unbounded MCU allocation is not.

Sub-byte integers, explicitly sized enums, fixed-point values, and reserved fields are first-class schema features. Compact encoding is encouraged when it preserves clarity and is justified by a constrained profile, but this document does not select a final schema language or serialization format.

### Version, compatibility, and build identity

Three identities remain separate:

- **Protocol version** — the major wire-semantic interpretation carried in each service message.
- **Compatibility fingerprint** — a stable, preferably generated fingerprint over the canonical schema and all communication-relevant profile/configuration choices. DEETS or equivalent management infrastructure may expose it.
- **Build identity** — the exact software build, reported by BIO or equivalent.

An internal hotfix may change build identity without changing compatibility. A wire-visible build option changes compatibility even if the protocol version remains valid. An incompatible redesign changes protocol version and compatibility. A build hash must not be used as a substitute for schema compatibility.

## 5. Size and profile rules

Services use one of three sizing patterns:

- **Fixed-small** — every message has a modest fixed maximum, preferably efficient on constrained links.
- **Bounded-profile** — a finite named profile changes capacities such as maximum entries, samples, or string bytes while preserving message meaning and state-machine behavior.
- **Bulk-transfer** — small control/status messages coordinate a separately bounded segmented or object-transfer facility.

A profile may alter capacity, batching, or storage bounds. It must not silently remove semantic fields, redefine units, or change protocol behavior merely to fit a link. Published profiles are named, immutable, and independently testable; incompatible changes require a new profile identity.

Every service instance declares maximum encoded message size, supported transport profiles, expected and maximum rates, burst depth, freshness/deadline requirements, and compatible LLL capacities. Broad portability does not require every service to fit every link.

Service and deployment configuration refer to QoS only by symbolic class name.
Numeric ordering, numeric priority comparisons, and final symbolic-to-numeric
mapping remain open.

Classical CAN remains useful design pressure, especially at small aggregation depths, but its consolidated PDU format is unfinished. Capacity estimates from draft CAN layouts are not stable application contracts; see the [experimental CAN PDU adapter specification](can_pdu_adapter_spec.md).

## 6. Bounded local endpoint behavior

Network delivery and local concurrency are separate policies selected for each service instance.

### Latest-value snapshots

A latest-value endpoint retains one coherent decoded snapshot. One receive/publish writer may update it while multiple local readers observe either the previous or next complete value, never a torn mixture. Suitable implementations include a seqlock or double buffer, subject to the platform memory and cache-coherency contract.

Snapshots carry enough metadata for consumers to assess validity: receive or source sequence, time/age basis, producer restart epoch where required, and decode/E2E status where available. A snapshot is not an event history. Replacing an unread value is normal and is counted when diagnostically useful.

### Bounded event queues

An event endpoint uses a fixed-capacity FIFO or ring when every retained event matters. It defines:

- producer and consumer concurrency;
- ordering and sequence/gap behavior;
- overflow policy: reject newest, drop oldest, coalesce defined events, or enter a fault state;
- observable overflow and high-water counters;
- restart and retained-state behavior.

Queue capacity is a deployment/profile parameter with a static resource cost. "Queue until memory is available" is not a valid MCU policy.

### Single-writer and multi-caller TX

A **single-writer TX endpoint** is owned by one task, thread, core, or serialized execution context. A **multi-caller TX endpoint** adds a bounded thread-safe funnel in front of the same protocol endpoint.

Multi-caller support must define arbitration, ordering, per-call result, ISR allowance, wakeup behavior, and storage ownership. It must not convert an application endpoint into an unbounded multi-producer queue. When ordering or caller authority affects service meaning, those semantics belong to the service protocol rather than being hidden in the funnel.

## 7. TX rejection and congestion

Transmit submission can fail during normal operation. Every transmitting service defines bounded behavior for local rejection and does not busy-spin.

Candidate result categories include accepted, replaced/coalesced, congested, invalid route, invalid message/profile, and link unavailable. Names and API representation are provisional. Local rejection means the message never entered the transport; a later reliable timeout would mean that transport had accepted ownership. These states must not be conflated.

Required service policies:

- **Latest value:** retain at most one pending value; a newer value may replace it; retry on notification, tick, or timer.
- **Command or high-integrity operation:** expose rejection to the caller or retain the exact bounded operation; do not advance protocol state before the defined commitment point.
- **Event stream:** enqueue in a bounded FIFO and apply the declared overflow policy.
- **Best-effort diagnostic:** drop, aggregate, or suppress under congestion and increment counters.
- **DRIP detail response:** retain a bounded cursor, pause generation, and resume when capacity returns.

Successful local enqueue is the earliest normal TX-state advancement point. A protocol may deliberately require a later point, such as service acceptance, but Reliable transport and its commitment rules remain TBD.

The LLL/router path should expose bounded capacity notification and metrics such as occupancy, high-water mark, rejection, drop, replacement, and completion timestamp where supported. Services must remain correct if notifications are coalesced or delayed.

## 8. Pub/sub and listeners

Pub/sub is brokerless and statically configured:

- each publication is associated after ingress/routing validation with an
  opaque deployment-local `ProducerKey` assigned by static configuration for
  prototype, configuration, and capture use;
- one authoritative producer originates for each `ProducerKey` unless an
  explicit authority/arbitration protocol says otherwise;
- any number of configured local consumers may read a shared snapshot or event queue;
- any number of statically configured passive listeners may consume an
  explicitly authorized publication visible on an attached ingress link
  without becoming addressed peers;
- a configured transparent forwarder may preserve and carry an authorized
  publication on the same VC to another attached link;
- a configured cross-VC replicator may consume an eligible publication on one
  VC and originate a distinct publication on another VC.

`ProducerKey` is runtime metadata, not a frozen wire field. For received
traffic it is derived only after ingress and routing validation succeed; a
locally originating endpoint receives only the key assigned to its static
registration. Endpoint identity and physical visibility alone do not establish
producer identity. A listener retains the validated `ProducerKey` with
consumed data.

A listener does not acknowledge, control flow, acquire reply or forwarding
authority, or gain delivery guarantees. Physical visibility and endpoint ID do
not create a subscription. Same-VC forwarding can carry a publication to
another attached link; listening alone cannot. Physical bus broadcast does not
imply network-wide broadcast.

Every cross-VC replication edge is explicit, bounded, observable, and
loop-free, and declares eligible publication message types, source
`ProducerKey`, destination `ProducerKey` and authority owner, provenance
representation, and conflict policy. Commands, RPC requests or responses,
service errors, and arbitrary traffic are forbidden on such an edge unless the
service defines explicit proxy semantics, authority, correlation, failure
behavior, and bounds. Control-relevant state requires representable original
provenance. Configuration and runtime validation reject competing producers
for a destination `ProducerKey` unless an explicit authority/arbitration
protocol resolves them. Cross-VC replication does not create general
ECU-to-ECU routing.

Dynamic subscriber discovery, runtime route creation, a mandatory broker, and implicit fan-out inferred from endpoint identity are outside the base model. See [core protocol and routing](core_protocol_and_routing.md) for listener, forwarding, and replication distinctions.

## 9. Service-level errors and diagnostics

Service-level failure belongs to the service stream. A protocol may define an error-response message alongside request and success types, optionally using a reusable compact representation such as BRO. Useful categories include malformed request, unsupported version/type/operation, invalid argument or state, permission denied, busy, and rejected. Correlation data is included only where the protocol semantics require it.

Infrastructure faults such as no route, unknown endpoint, transport mismatch, malformed envelope, reassembly failure, and unexpected ingress are recorded by generic metrics/diagnostics even when the application endpoint is receive-only or does not implement error messages. They are not automatically converted into service responses.

Diagnostic containment is mandatory:

- an error report never recursively generates another remote error report;
- local counters and latched evidence remain the source of truth;
- remote reporting is optional, bounded, rate-limited, and normally lower priority than control traffic;
- repeated errors are aggregated and suppressed counts remain observable;
- malformed or babbling peers cannot force unbounded diagnostic work;
- ordinary endpoints do not receive every other node's errors.

DRIP is the preferred pattern: publish a small bounded summary periodically, then return larger bounded detail only on explicit request. Loss of the remote diagnostic path must not erase local evidence.

## 10. Safe use of the service catalog

The catalog in the source drafts is a set of candidates, not an automatic standard library or allocation authority. A service should be promoted only when it has a clear semantic boundary, bounded schema, explicit privilege model, compatibility metadata, resource profile, test vectors, and at least one real deployment.

Useful catalog guidance:

- MENU describes configured endpoint instances; it does not enable them or create routes.
- DEETS reports communication compatibility; BIO reports build identity; WHO reports hardware identity.
- CAPS, if retained, selects among finite precompiled profiles; it does not negotiate arbitrary encodings.
- TABS is suitable for disposable scalar prototypes. Features graduate to dedicated services when they need transactions, timing, richer state, or interaction.
- BRO is an optional service-error shape, not a substitute for local infrastructure diagnostics.
- TELL/DRIP-style logs and diagnostics are bounded and best effort; they never block critical work indefinitely.
- SUS, AMA, ROSE, fault injection, firmware update, raw capture/injection, and
  similar dangerous or privileged services are outside the initial prototype.
  They require a separate development build, build-time enablement, and
  local-only, non-forwardable, allowlisted operations. Runtime configuration
  alone cannot enable them.

Catalog names remain provisional until their contracts and endpoint allocations are published. The local/private security posture and gateway risks are discussed in [rationale, use cases, and risks](rationale_use_cases_and_risks.md).

## 11. Required declaration for each service

Before a service is considered implementable, its specification records:

- roles, authority, binding mode, and permitted autonomous traffic;
- endpoint instances and literal-ID allocation policy;
- message types, canonical schema, units, reserved fields, and serialization;
- protocol version, compatibility inputs, and profile identity;
- state, event, command, RPC, or object semantics;
- size/rate/burst bounds, freshness, ordering, loss, duplicate, and restart behavior;
- local snapshot/queue/concurrency policy;
- TX rejection, congestion, timeout, and overflow behavior;
- service-error and nonrecursive diagnostic behavior;
- transport/LLL compatibility and resource budget;
- passive listener, transparent forwarder, cross-VC replicator, `ProducerKey`,
  provenance, capture, and security policy.

Reliable transport is wholly TBD. CAN and HDLC are experimental until their owning documents define immutable profiles and golden vectors. No service may claim reliable or cross-link wire interoperability merely by referring to an exploratory draft.
