# Architecture Overview

**Status:** Consolidated architecture draft  
**Authority:** Normative for architectural scope, boundaries, and invariants; detailed definitions are delegated  
**Interoperability:** Not claimed; unresolved numeric encodings and wire profiles remain open

## Purpose

This architecture defines a small, statically configured message fabric for embedded systems. It carries discrete, bounded service messages across local software, multicore devices, embedded buses, point-to-point links, and explicitly configured gateways.

It is intended to let service protocols remain stable while deployment placement and compatible link technology change. It is not a smaller version of the Internet and does not make arbitrary nodes globally addressable.

## Goals

- Stable boundaries between application services, endpoint dispatch, transport semantics, logical links, and hardware links.
- Bounded behavior suitable for MCUs, FPGA softcores, RTOS systems, and host tools.
- One service model for same-core calls, inter-core IPC/ICC, Main/ECU traffic, Main-to-Main traffic, and constrained static forwarding.
- Brokerless publication with known producers and configured consumers.
- Link independence within explicit message-size, timing, and transport constraints.
- Static analysis of routes, capacity, loops, identities, and configuration compatibility.
- Observable congestion, loss, restart, and configuration failures.
- Composition with IP or specialized bulk/streaming systems at explicit boundaries.

## Non-goals

- General internetworking, arbitrary meshes, or dynamic routing.
- Arbitrary ECU-to-ECU unicast, especially across physical buses or VCs.
- Runtime route discovery, address assignment, or mandatory service discovery.
- A general broker or globally dynamic subscription system.
- A universal reliable byte stream or transparent segmentation of unbounded application data.
- Making every service support every transport or link.
- Implicit remote access, raw link injection, or a built-in secure Internet gateway.
- Claiming safety certification, cryptographic protection, or wire interoperability from the architecture alone.

## Five-layer model

1. **Application / Service** — owns message meaning, roles, timing, freshness, authority, and application failure behavior. See [application protocols and services](application_protocols_and_services.md).
2. **Endpoint / Service Router** — binds configured endpoint identities to local producers, consumers, storage, and dispatch behavior. Routing details are owned by [core protocol and routing](core_protocol_and_routing.md).
3. **Protocol / Transport** — supplies a small set of bounded delivery semantics selected for an endpoint. See [logical links and transports](logical_links_and_transports.md).
4. **Logical Link Layer (LLL)** — carries bounded PDUs over a concrete link while preserving or reconstructing the required canonical semantics. Link-specific encoding may differ.
5. **Hardware Link** — provides the fundamental frame, datagram, byte, FIFO, or shared-memory transfer mechanism.

The layers are contracts, not mandatory threads, processes, or allocation boundaries. A constrained implementation may combine them while preserving their responsibilities.

## Supported topology and communication domains

The model supports:

- one process or core;
- multiple tasks or cores on one host;
- point-to-point Main/ECU links;
- one or more Main devices and multiple ECUs on a broadcast bus;
- multiple Main devices communicating in the configured `kMainCompute` domain;
- loop-free, statically configured paths across unlike links;
- publications consumed by configured listeners on attached links;
- explicit, configured cross-VC state replication.

Three domain classes are central:

- **`VirtualCircuit::kLocalHost`** — local IPC/ICC. Traffic remains inside the host and is not emitted on an external link.
- **`VirtualCircuit::kMainCompute`** — a configured domain for Main-to-Main communication using the same service model as other traffic.
- **Deployment VCs** — statically configured communication domains for Main/ECU and multi-link connectivity.

A system may also compose the fabric with an outer IP network, native stream, file-transfer service, or specialized real-time network. That outer system supplies capabilities outside this architecture and creates an explicit gateway, proxy, or trust boundary.

## Service roles and peer selection

Every directed service protocol has an **upstream** role and a **downstream** role. These are service semantics:

- upstream is the stimulus, controller, or initiator side;
- downstream is the responder or controlled side.

The roles do not describe physical hierarchy. They do not reverse when the downstream side sends a response or later asynchronous traffic. A Main, ECU, task, or core can implement either role for a particular service. Symmetric interaction is composed from directed service instances rather than inferred from topology.

Addressing remains deliberately asymmetric and bounded:

- a downstream target is identified by the complete `VCN + Index`; an `Index` is not a global node address;
- `UpstreamHost` identifies a configured Main for external/Main relationships, but reserves symbolic `UpstreamHost::kLocalHost` for local IPC/ICC;
- `VirtualCircuit::kMainCompute` permits Main-to-Main services without redefining service roles;
- `VirtualCircuit::kLocalHost` permits same-core and cross-core services without exposing internal placement externally.

`UpstreamHost::kLocalHost` and `VirtualCircuit::kLocalHost` are distinct; both numeric encodings remain provisional. The canonical fields, endpoint identity, reply binding, and receive/transmit-only cases belong to [core protocol and routing](core_protocol_and_routing.md). This overview assigns no wire layouts.

## Virtual Circuits

A VC is a statically configured connectivity context. It can describe one physical bus, one point-to-point link, or a loop-free path through several links. The message identifies the VC and relevant member/target; deployment configuration contains the physical route.

VCs provide bounded communication and policy domains. They do not create global addresses or authorize arbitrary traffic between domains. Physical loops may exist, but each active VC forwarding topology must be deterministic and loop-free unless a future specification explicitly defines another bounded mechanism.

Exact VC identifiers, target fields, group behavior, ingress rules, and forwarding state are delegated to [core protocol and routing](core_protocol_and_routing.md).

## Publication, listening, forwarding, and replication

These operations are distinct:

- **Publisher** — originates a service value or event under explicit producer authority.
- **Subscriber/consumer** — is a configured recipient that applies the service's storage, timing, and validation behavior.
- **Configured listener** — consumes a publication visible on an attached link without becoming its addressed peer, reliable peer, producer, or forwarding authority.
- **Transparent forwarder/repeater** — carries a received message along a statically configured path within its communication context while preserving the canonical service and transport semantics. It may make a publication visible on another attached link within the same VC, but does not originate application state.
- **Cross-VC replicator** — explicitly consumes selected state in one VC and republishes it into another configured VC. It is a policy and failure-semantics boundary, not transparent forwarding and not general ECU routing.

Listening itself creates no forwarding or fan-out. Identity preservation or translation at a replicator must be specified by its owning service/configuration. Receivers do not forward or replicate by default. See [core protocol and routing](core_protocol_and_routing.md) and [application protocols and services](application_protocols_and_services.md) for the detailed contracts.

## Security posture

The initial supported deployment is local/private and assumes the embedded fabric is physically or host isolated unless a connected device or gateway is compromised. The core architecture does not provide authentication, authorization, confidentiality, or cryptographic replay protection.

Initial implementations and examples must:

- exclude shell, memory/MMIO access, update, fault control, replay, and raw injection from the initial prototype;
- require a separate development build for experimental versions of excluded capabilities; runtime network configuration alone cannot enable them;
- favor read-only observation and diagnostics;
- require explicit configuration for forwarding and cross-VC replication;
- keep remote/IP exposure outside the recommended initial feature set;
- treat SWD/JTAG, memory access, calibration, firmware update, and raw packet injection as privileged trust boundaries.

IP composition remains possible through an outer carrier or explicit proxy. A secure policy-enforcing remote gateway requires a separate reviewed threat model, authentication, authorization, replay protection, key lifecycle, rate limiting, and audit design; it is not an initial architecture feature.

## Architecture invariants

1. Messages and all associated queues, retries, and state are bounded by configuration.
2. Application services are link-independent only within declared size, timing, and transport compatibility.
3. Service upstream/downstream roles are independent of physical topology and device class.
4. A downstream target is the full `VCN + Index`; an `Index` has no global meaning.
5. `UpstreamHost` identifies a configured Main for external/Main relationships; reserved symbolic `UpstreamHost::kLocalHost` identifies local IPC/ICC and is not a general host or ECU address.
6. `UpstreamHost::kLocalHost` is distinct from `VirtualCircuit::kLocalHost`; both numeric encodings remain provisional. `VirtualCircuit::kLocalHost` traffic remains local, and `VirtualCircuit::kMainCompute` provides the supported Main-to-Main domain.
7. Arbitrary ECU-to-ECU unicast, particularly across buses or VCs, is unsupported.
8. A configured listener on an attached link may consume a visible publication without becoming a peer or producer; listening creates no forwarding or fan-out.
9. Cross-VC state sharing requires an explicit configured replicator.
10. Forwarding is explicit, static, loop-free by configuration, and non-originating; it may make a publication visible on another attached link within the same VC.
11. Published state has explicit producer/authority rules; replication does not create accidental competing producers.
12. Transport is selected by service semantics. Reliability is not assumed to be safer than loss with freshness detection.
13. Congestion, rejection, drops, stale data, restart, and configuration mismatch require bounded, observable handling.
14. Security-sensitive capabilities are least-privilege and disabled by default.
15. No unresolved number, bit layout, serialization, or profile is implied to be stable.

## Current maturity

| Area | Status |
|---|---|
| Scope, layers, role independence, and static VC direction | **Consolidated draft** |
| `VirtualCircuit::kLocalHost`, `VirtualCircuit::kMainCompute`, Main/ECU domains, listeners, and configured replication | **Supported architecture direction** |
| Unreliable Datagram semantics | **Completed current transport definition** |
| Sequenced/E2E-Protected Datagram | **Provisional** |
| Reliable transport | **Wholly TBD** |
| CAN PDU adapter | **Experimental** |
| HDLC profile | **Experimental** |
| Numeric encodings, immutable wire profiles, and conformance vectors | **Unresolved** |
| Secure remote gateway | **Not an initial feature** |

Implementation and promotion criteria belong to [prototype and validation](prototype_and_validation.md). Until the delegated specifications and vectors are complete, independently developed wire implementations must not be presumed interoperable.

## Related documents

- [Document index and source traceability](README.md)
- [Core protocol and routing](core_protocol_and_routing.md)
- [Application protocols and services](application_protocols_and_services.md)
- [Logical links and transports](logical_links_and_transports.md)
- [Link Entity runtime and status](link_entity_runtime_and_status.md)
- [HDLC logical-link profile](hdlc_logical_link_profile.md)
- [CAN PDU adapter specification](can_pdu_adapter_spec.md)
- [Prototype and validation](prototype_and_validation.md)
- [Rationale, use cases, and risks](rationale_use_cases_and_risks.md)
