# Rationale, Use Cases, and Risks

**Status:** Non-normative  
**Purpose:** Explain design intent, deployment fit, tradeoffs, alternatives, adoption strategy, and risks  
**Limitation:** Examples show architectural fit, not implementation readiness, safety qualification, or wire interoperability

Normative scope and definitions belong to the [architecture overview](architecture_overview.md), [core protocol and routing](core_protocol_and_routing.md), [application protocols and services](application_protocols_and_services.md), and [logical links and transports](logical_links_and_transports.md).

## Why this architecture exists

Embedded projects repeatedly need the same bounded capabilities—commands, state, events, health, configuration, logging, update, and test access—but often redesign them for each CAN bus, UART, FIFO, shared-memory path, or host adapter.

The architecture seeks a reusable middle ground:

- smaller and more analyzable than general internetworking;
- more structured and portable than unrelated raw-link messages;
- usable from local IPC/ICC through bus-connected devices;
- compatible with gateways and outer IP systems without requiring IP on every MCU;
- static enough for route, memory, bandwidth, and failure analysis.

Classical CAN is an important constraint, not the only target. Designing around bounded messages and modest overhead also suits CAN FD, serial links, FIFOs, shared memory, and host datagrams.

## Deployment fit

### Development board

A PC and one MCU can use the same service model over UART, USB records, Ethernet/UDP, CAN, or a debug-probe bridge. Local host tools can share `VirtualCircuit::kLocalHost`; board traffic uses a configured VC. Firmware identity, health, telemetry, parameters, logs, and test RPC provide an immediate useful subset.

SWD/JTAG remains a separate privileged path. The message fabric does not make processor halt, arbitrary memory access, or flashing safe merely by expressing them as services.

### Hardware-in-the-loop system

A HIL rig benefits from explicit producer identity, bounded real-time datagrams, passive observation, capture metadata, simulated peers, and separation between rig-control traffic and device-native traffic. Multiple host processes or Main devices can communicate through local and `kMainCompute` domains.

The orchestrator should not enter every real-time path. Cycle-critical plant simulation belongs on the real-time target and its native I/O. Cross-domain simulation or recording is an explicit proxy/replication function, not accidental general routing.

### Industrial robot

The fabric fits bounded controller/drive state, remote I/O, tool services, health, and coordinated status where producers and consumers are configured. Fresh, repeated setpoints are better modeled as time-limited state than as a byte stream.

It is not a replacement for certified safety networks, hardwired safe-torque-off paths, deterministic motion buses, or high-volume vision transport. Safety authority and local enforcement remain explicit and independently justified.

### Road vehicle

The model fits compact CAN/CAN-FD services, Main/zone-controller domains, configured listeners on attached links, explicit gateway paths, health and diagnostics, and configured replication of selected state. It supports several Main devices without pretending that every ECU has a globally routable address.

It does not replace established vehicle diagnostics, calibration, safety mechanisms, or high-bandwidth sensor networks merely for architectural uniformity. Telematics and external connectivity must terminate at a policy-enforcing trust boundary; they must not expose raw internal injection.

### Satellite

Static configuration, bounded resource use, explicit producer authority, local fault handling, and observable loss/restart behavior fit onboard command, health, and low-rate telemetry domains. The same service model can span redundant onboard links where each path is deliberately configured.

The architecture does not solve disrupted RF communications, mission command authentication, recorder management, or bulk downlink. CCSDS procedures, CFDP-like object transfer, delay-tolerant networking, and mission-specific redundancy remain appropriate outside or above the local message fabric.

Across all five examples, the architecture is most credible for bounded local operational and diagnostic messages. Large objects, media, interactive shells, certified safety channels, and long-delay communications often require specialized adjacent systems.

## Central tradeoffs

- **Static configuration over runtime flexibility.** Routes and memberships are analyzable and bounded, but topology changes require configuration and deployment discipline.
- **Communication domains over global addresses.** `VCN + Index` and compact Main identity fit constrained systems, but deliberately prevent arbitrary universal unicast. `UpstreamHost` identifies a configured Main for external/Main relationships; reserved symbolic `UpstreamHost::kLocalHost` represents local IPC/ICC and is distinct from `VirtualCircuit::kLocalHost`. Both numeric encodings remain provisional.
- **Logical consistency over identical wire bytes.** Services can retain semantics across links, while each LLL may use an efficient encoding. This increases the need for precise profiles and conformance tests.
- **Small transport vocabulary over universal behavior.** Each service selects the semantics it needs; integrators must verify compatibility rather than assuming every link and transport combination works.
- **Brokerless publication over dynamic discovery.** Known producers and consumers avoid broker state and discovery traffic, but runtime subscription and ad hoc integration need an explicit external service or proxy.
- **Private-by-default over remote convenience.** Initial exposure is safer and easier to reason about, but remote workflows require intentionally designed gateway infrastructure.
- **Bounded degradation over hidden convenience.** Queue rejection, drops, stale values, and congestion are visible service concerns rather than being hidden behind unbounded buffering.

## Why not general routing

General routing would require broader addressing, loop handling, route discovery or convergence, failure policy, more dynamic state, and a larger security surface. Those costs conflict with the target: known nodes, known communication paths, bounded resources, and build-time validation.

Static VCs still permit heterogeneous multi-link paths and configured Main-to-Main traffic. They do not permit arbitrary ECU-to-ECU unicast, particularly across buses. A configured listener on an attached link can consume a visible publication, but listening itself creates no forwarding or fan-out. Transparent forwarding may make the publication visible on another attached link within the same VC; selected cross-VC state replication remains explicit.

Where general routing is actually needed, IP or another established network should provide it outside the embedded fabric. The boundary should be an explicit carrier, proxy, or policy-enforcing gateway.

## Why not a mandatory broker

Most target systems know their producers and consumers at integration time. Direct configured delivery avoids a mandatory central service, dynamic subscription state, discovery traffic, and broker failure semantics. It also lets local snapshot storage and bus visibility serve simple one-to-many publication efficiently.

A broker can still be useful on a host, in a lab, or across enterprise systems. It should be an adapter at that boundary, not a prerequisite for an MCU to publish health or consume a command.

## Why not a universal byte stream

The primary data model is a discrete bounded message with explicit size, age, producer, and handling policy. A universal reliable stream would add connection state, head-of-line blocking, flow-control coupling, and pressure to buffer data whose age may already make it useless.

Interactive consoles, large logs, firmware images, and payload files remain valid needs. They are better served by a bounded object-transfer service or an established native stream on capable nodes. Their requirements should not determine the cost or failure behavior of every small control endpoint.

## Why reliability is not always safer

Delivery reliability and safe application behavior are different properties.

For sampled state or repeated control intent:

- retransmission increases age;
- ordered delivery can block newer data behind an old loss;
- queues can preserve obsolete commands;
- reconnect can replay stale intent;
- acknowledgment proves only the defined peer interaction, not that every observer acted safely.

For these cases, a fresh sequenced value, explicit maximum age, timeout reaction, and current-state feedback can be safer than retransmission. The E2E profile intended to support such checks is still provisional.

Reliability remains valuable for finite transactions and objects: configuration commits, command acceptance, diagnostic operations, and image chunks. However, the Reliable transport is wholly TBD. No reliable state machine, encoding, or interoperability should be inferred from examples in this document.

## Adoption and ecosystem strategy

The architecture should earn adoption incrementally rather than require a platform rewrite.

1. Provide immediately useful read-only services and host tools: identity, health, metrics, logging, capture, and configuration fingerprints. Capture is read-only; replay is privileged test-only injection, separately gated and disabled by default.
2. Coexist with existing raw CAN, serial, IPC, and application protocols.
3. Add one service or link at a time, with a clear removal path.
4. Keep core, service libraries, LLLs, platform adapters, and host tools separable.
5. Support bare metal and common RTOS environments without imposing a task model, allocator, build system, or application framework.
6. Treat Linux tooling, simulation, capture, and Python integration as product capabilities rather than afterthoughts.
7. Demonstrate acceptable CAN efficiency and bounded MCU cost with measured prototypes.
8. Publish exact profiles and conformance vectors before claiming cross-implementation interoperability.

The strongest adoption wedge is likely a useful service/tool pair, not the abstract protocol alone. A health or boot service that works across several targets and links is more persuasive than a broad architecture without production evidence.

## Principal risks

### Architecture and implementation

- **Architecture before proof.** Interfaces may appear clean until real CAN, serial, multicore, congestion, and restart behavior is measured.
- **Configuration failure.** Static routing trades protocol dynamics for code-generation, review, rollout, and fingerprint consistency risks.
- **Leaky link differences.** Message limits, arbitration, ordering, integrity, and medium access cannot be erased by a common API.
- **Resource surprises.** Queue state, copies, transport state, serialization, and diagnostics may exceed MCU budgets.
- **False maturity.** A coherent descriptor or draft packet layout is not an interoperable profile.
- **Producer ambiguity.** Incorrect simulation, failover, or replication can create competing sources for control-relevant state.
- **Overload cascades.** Diagnostics, retries, or forwarding can amplify a failing node unless bounded and rate-limited.

The [prototype and validation](prototype_and_validation.md) plan should test these risks with heterogeneous links, congestion, stale configuration, restart, malformed input, and measured CPU/RAM/wire costs.

### Security and debug

The base fabric does not authenticate peers, encrypt traffic, authorize operations, or provide cryptographic replay protection. Private topology reduces exposure; it does not make a compromised node trustworthy.

High-risk capabilities include:

- arbitrary memory or MMIO access;
- unrestricted shell or RPC execution;
- replay or raw CAN/link injection;
- fault injection and control overrides;
- firmware update and boot control;
- JTAG/SWD, calibration, trace, and processor halt;
- broad forwarding from an IP-connected host.

The initial prototype excludes shell, memory/MMIO access, update, fault control, replay, and raw injection. Experimental versions require a separate development build, remain separately gated and disabled by default, and cannot be enabled solely by runtime network configuration. Capture remains read-only. Read-only and write/control authority must be distinct. Diagnostic traffic must be bounded so a faulty node cannot destabilize healthy control traffic.

A future remote gateway needs a reviewed threat model, strong peer and message authentication, authorization by service/operation/domain, replay protection, key lifecycle, rate limiting, audit, and safe session/reset behavior. Tunneling through TLS or a VPN alone does not establish correct endpoint authorization.

## Alternatives and when to use them

- **Raw CAN plus a DBC or project-specific messages** — appropriate for a small fixed system where minimal overhead and existing tooling outweigh cross-link reuse.
- **CANopen, J1939, UDS/DoIP, XCP, AUTOSAR communication, or another domain stack** — preferable when its standardized services, compliance, ecosystem, or vehicle integration already match the requirement.
- **UDP/IP or TCP/IP** — preferable when routability, established security tooling, sockets, and host interoperability matter more than minimal MCU overhead.
- **DDS, SOME/IP, Zenoh, MQTT, or another broker/discovery ecosystem** — preferable when dynamic discovery, rich QoS, enterprise integration, or large-node environments justify the footprint and operational model.
- **Native RTOS IPC or shared-memory APIs** — preferable for tightly local interactions that do not benefit from service portability or network placement.
- **Dedicated deterministic or certified safety networks** — required when scheduling, certification, redundancy, or safety evidence exceeds this fabric's scope.
- **CCSDS, CFDP, or disruption-tolerant networking** — preferable for space links, long delay, scheduled contacts, and resilient object delivery.

These are not mutually exclusive choices. A deployment can use the embedded fabric for bounded local services and established systems for diagnostics, bulk transfer, safety, media, or external networking.

## Future research

- Final canonical semantics, numeric assignments, immutable named profiles, and golden vectors.
- Independent implementations and differential/conformance testing.
- Complete E2E sequencing, identity, freshness, reset, CRC/MAC, and failure behavior.
- Reliable bounded-message requirements and state machine, derived from concrete transactions and object transfer.
- Experimental CAN and HDLC profile completion, malformed-input handling, and measured efficiency.
- Generated VC/route configuration with loop, reachability, ingress, capacity, and compatibility validation.
- Bounded scheduling, backpressure, reservation, overload, and diagnostic-storm analysis.
- Recovery/status semantics for link loss, endpoint restart, stale state, and configuration mismatch.
- Local IPC/ICC copy, cache-coherency, synchronization, queue, and placement costs.
- Static failover and redundant-path behavior without introducing general dynamic routing.
- Optional end-to-end authentication and a secure, least-privilege remote gateway architecture.
- Evidence needed for safety-relevant, industrial, vehicle, and space use.

Research outcomes become authoritative only when incorporated into their owning consolidated specification and validated. This document does not assign unresolved numbers, freeze wire layouts, or assert interoperability.

## Related documents

- [Document index and source traceability](README.md)
- [Architecture overview](architecture_overview.md)
- [Core protocol and routing](core_protocol_and_routing.md)
- [Application protocols and services](application_protocols_and_services.md)
- [Logical links and transports](logical_links_and_transports.md)
- [Link Entity runtime and status](link_entity_runtime_and_status.md)
- [HDLC logical-link profile](hdlc_logical_link_profile.md)
- [CAN PDU adapter specification](can_pdu_adapter_spec.md)
- [Prototype and validation](prototype_and_validation.md)
