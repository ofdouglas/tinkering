# Routing Hosts, Routes, and Endpoint Access

## Purpose

This document defines the high-level relationship between **Routing Hosts**, statically configured **Routes**, and application **Endpoints**.

The design intentionally separates:

- **Naming:** which service is being referenced;
- **Reachability:** which routing relationship/path a message may traverse;
- **Authority and concurrency:** which local or remote components may access a service, and under what access model.

An endpoint namespace is therefore a naming mechanism, **not an implicit authority domain**. Typed handles and static validation reduce accidental misuse, but they are not by themselves a security or isolation boundary; enforcement against compromised or untrusted software requires an appropriate protection boundary such as MPU/MMU isolation, process isolation, or separation into distinct Routing Hosts.

The central addressing model is:

```text
RouteId + Direction
    -> identifies the statically configured communication relationship/path

Namespace + EndpointId
    -> identifies the service at the receiving Routing Host
```

For communication that remains inside one Routing Host, `RouteId::kLocal` represents local delivery and no inter-host routing is performed.

---

## 1. Routing Host

A **Routing Host** is a group of software components that intentionally share one endpoint-routing domain.

A Routing Host owns:

- one logical Endpoint Router;
- one local EndpointId namespace;
- zero or more Routes to other Routing Hosts;
- local access-control/binding information for its endpoints.

A simple MCU will often contain exactly one Routing Host. This is a recommended default, not a requirement.

A processor may contain multiple Routing Hosts when stronger separation is useful, for example:

- safety and non-safety partitions;
- isolated software domains;
- cores that should communicate only through explicit IPC boundaries;
- independently managed processes on a host computer.

Conversely, multiple execution contexts or cores may share one Routing Host when they intentionally share the same endpoint domain and access model.

The Routing Host is therefore a **logical routing and authority boundary**, not necessarily a physical CPU boundary.

---

## 2. Endpoint Router

Each Routing Host has one authoritative **Endpoint Router** responsible for resolving EndpointIds to locally hosted services.

Conceptually:

```text
EndpointId
    |
    v
Endpoint Router
    |
    +--> callback
    +--> snapshot buffer
    +--> event queue
    +--> RPC/service handler
    +--> other endpoint implementation
```

The router does not inherently require a dedicated task. A minimal implementation may be little more than a statically generated lookup table followed by dispatch to an endpoint implementation.

Components within the same Routing Host do **not** allocate a remote RouteId merely to communicate with one another. They already share the same local endpoint namespace.

Generic representations, traces, or APIs that require an explicit routing value may represent this as:

```text
RouteId = kLocal
Direction = 0
```

`Direction` has no routing meaning for `kLocal` and **shall be zero**. A received canonical local-routing value with nonzero Direction is invalid.

---

## 3. Route

A **Route** is a statically configured communication relationship connecting two Routing Hosts, or in restricted multicast cases a Routing Host and a configured set of receiving Routing Hosts.

A Route has two logical sides:

```text
Side A <---- configured path ----> Side B
```

`Direction` identifies traversal as either:

```text
A -> B
B -> A
```

The side labels are structural. They do not mean requester/responder, producer/consumer, command/status, or initiator/responder. A service may initiate traffic in either valid direction without changing the meaning of the Direction bit.

For systems with an obvious hierarchy, configuration may conventionally place a Main/supervisory host on Side A, but this is not a protocol requirement.

A Route may be implemented by:

- direct local IPC between partitions or cores;
- one physical CAN, UART, BLE, Ethernet, or other link;
- several physical links joined by gateways;
- another statically configured forwarding path.

The packet names the Route; **configuration contains the physical path**.

---

## 4. RouteId and Routing Descriptor

`RouteId` is the canonical identity of a Route. It should not be interpreted as a node address or a dynamic IP-style destination.

The current proposed 16-bit routing field is:

```text
bits 15:14   QoS
bit     13   Direction
bits 12:3    RouteId        // 10 bits
bits  2:0    Reserved
```

Reserved routing bits shall be transmitted as zero. Receivers shall reject unsupported nonzero reserved-bit values rather than silently interpreting them as the current format.

This provides up to 1024 canonical RouteId values while retaining three routing-specific reserved bits for future expansion. RouteIds scale with relationships between Routing Hosts rather than with services or ordinary task-level IPC, so 1024 routes is expected to provide substantial headroom even for large embedded systems. The worst-case pairwise count for `H` Routing Hosts is `H(H-1)/2`, but real embedded systems are expected to be much sparser because Routes are configured only where communication is intended. Configuration tooling shall reject deployments that exceed the canonical RouteId capacity.

Recommended allocation:

```text
RouteId::kLocal = 0
RouteId 1..1023 = statically configured non-local Routes
```

The canonical name is **RouteId**, not `RouteIndex`: the value identifies a configured route throughout the protocol domain rather than merely indexing one local implementation table.

### RouteId::kLocal

`kLocal` identifies delivery within the current Routing Host.

```text
RouteId::kLocal + Namespace + EndpointId
    -> local endpoint access

non-local RouteId + Direction + Namespace + EndpointId
    -> service access through a configured inter-host Route
```

Normal local application code does not need to serialize or explicitly supply `kLocal`; direct typed endpoint handles are preferred. The value exists so canonical envelopes, generic tooling, and routing representations have a coherent local case.

---

## 5. Routes Connect Hosts, EndpointIds Select Services

A Route is allocated between **Routing Hosts**, not between individual tasks, callbacks, or services.

For example, if a three-core MCU treats each core as a separate Routing Host, it might define:

```text
Route::kCore0Core1
Route::kCore0Core2
Route::kCore1Core2
```

All services exchanged between Core 0 and Core 1 use `kCore0Core1`; `Namespace + EndpointId` selects the specific destination service.

A networked ECU may expose dozens of services while using only one Route to a Main controller. Statically configured ECU-to-ECU, Main-to-Main, partition-to-partition, and similar peer relationships are also valid Routes; the protocol does not impose a Main/ECU addressing asymmetry.

Likewise, multiple local tasks may consume the same `ReadSnapshotEndpoint<T>` without allocating additional Routes. They are accessing one service in one local endpoint domain.

This prevents RouteId count from scaling with the number of services or ordinary task-level IPC relationships. RouteIds scale primarily with the connectivity graph between Routing Hosts.

---

## 6. EndpointId Is Service Identity, Not Access Permission

`EndpointId` identifies a service or message endpoint within a Routing Host.

A globally known EndpointId does **not** imply that:

- every local task may call or write it;
- every remote Routing Host may reach it;
- every Route may carry it;
- the endpoint accepts arbitrary concurrent writers.

This distinction is fundamental:

> **EndpointId provides naming. Static bindings and typed local access provide authority.**

Ordinary application code should not normally receive an unrestricted API such as:

```cpp
router.send(arbitrary_endpoint_id, arbitrary_payload);
```

Instead, generated/static configuration should provide components with typed handles only to the endpoint operations they are permitted to use.

Example:

```cpp
class MotorControlTask {
    SnapshotReader<VehicleState> vehicle_state_;
    EventWriter<MotorStatus> motor_status_;
};
```

The system may therefore have one canonical endpoint namespace while normal application code receives only a capability-like subset of it. These handles are primarily a compile-time/API discipline. Code that can bypass them and directly access the router is not thereby security-isolated; stronger enforcement requires an appropriate Routing Host/protection-domain boundary.

---

## 7. Remote Reachability Is Explicitly Bound

Sharing a Route does **not** make the entire endpoint namespace of the remote Routing Host reachable.

Each Route and Direction has a statically configured set of allowed endpoint bindings. Bindings are directional and are defined by `(Namespace, EndpointId)`; the same EndpointId may be bound in both directions when a service intentionally exposes peer instances or symmetric semantics.

Example:

```text
Route::kCore0Core1

A -> B:
    VehicleState
    SensorSnapshot
    CommandQueue

B -> A:
    HealthStatus
    CompletionEvent
```

A received message is dispatched only if:

```text
(RouteId, Direction, Namespace, EndpointId)
```

matches a valid configured binding.

This prevents a Route from becoming an accidental global-namespace tunnel between Routing Hosts. The same rule applies to cross-core IPC, inter-device networking, and multi-hop routed paths.

Requester/responder, producer/consumer, command/status, and similar **service roles are application/service properties**, not meanings of the Route Direction bit. Direction remains purely structural: Side A to Side B or Side B to Side A.

---

## 8. Local Endpoint Access and Concurrency

Endpoints define their own local access semantics. Sharing an Endpoint Router does not make every endpoint safe for arbitrary concurrent access.

### ReadSnapshotEndpoint<T>

Intended for one producer and many readers.

A typical implementation may use a seqlock, double buffer, or equivalent coherent snapshot mechanism.

```text
1 writer -> N readers
```

Readers within the same Routing Host require neither separate RouteIds nor separate endpoint instances.

### SingleWriterEndpoint<T>

Exactly one configured producer may write the endpoint.

Useful when concurrent or ambiguous producers would violate service semantics.

### EventQueueEndpoint<T>

Provides bounded queued/event delivery with explicitly defined producer/consumer rules.

### GloballyWritableEndpoint<T>

Explicitly permits multiple writers and provides the required synchronization.

This should be selected deliberately rather than emerging accidentally from a generic router API.

The Endpoint Router does not silently make every endpoint safe for arbitrary concurrent access.

---

## 9. IPC and Networking Use the Same Model

The same logical model applies across different physical scopes.

### Same Routing Host

```text
Component A
    |
    | EndpointId
    v
local Endpoint Router
    |
    v
Component B / endpoint implementation
```

No non-local RouteId is required. Canonically this is `RouteId::kLocal`.

### Different Routing Hosts on One Device

```text
Core / Partition A
    |
    | RouteId + Direction
    v
Core / Partition B
    |
    | EndpointId
    v
service
```

The Route may be implemented using shared memory, a hardware mailbox, FIFO, or another IPC mechanism.

### Different Devices / Multi-Hop Network

```text
Host A
    |
    | RouteId + Direction
    v
LLL / gateways / physical links
    |
    v
Host B
    |
    | EndpointId
    v
service
```

The application-level addressing model is unchanged. Only the mechanism implementing the Route differs.

Cross-core/partition IPC is therefore a subset of the same static routing model used for distributed networking.

---

## 10. Source Identity, Multicast, and Observers

The base Route model has a hard source-identity invariant:

> **For every valid `(RouteId, Direction)`, exactly one active source identity exists.**

For a normal bidirectional Route, exactly one active participant exists on Side A and exactly one active participant exists on Side B. Either direction may be used, and Direction uniquely identifies which participant is the producer for that message.

This means a separate runtime `ProducerKey` is not required for ordinary routed traffic: `(RouteId, Direction)` identifies the configured producer relationship. Earlier designs that derived a separate `ProducerKey` from ingress/routing metadata are superseded by this model.

### Multicast Routes

A restricted multicast Route may define:

```text
Side A:
    one active transmitter

Side B:
    multiple receive-only participants
```

Only the configured multicast direction is valid. The receiving participants cannot transmit in the reverse direction on that Route because doing so would make source identity ambiguous. Feedback, acknowledgements, or status may use separate unicast Routes or a higher-level multicast-control design.

Multicast is useful for commands or publications whose semantic destination is a configured set of Routing Hosts. It is not required merely because additional nodes wish to observe traffic.

### Passive Observers

A Route may also define **passive observer bindings**. An observer is authorized to receive selected traffic on a Route without becoming a participant in the Route's source/destination relationship. Observer configuration is directional and endpoint-scoped, conceptually:

```text
(RouteId, Direction, Namespace, EndpointId) -> receive-only observer
```

Observers:

- may receive only explicitly bound endpoints;
- never acquire transmit authority on the observed Route;
- do not change the Route's source identity;
- do not become transport peers merely by observing traffic;
- do not participate in acknowledgement/retry state unless a higher-level service explicitly defines such semantics.

A logger or diagnostic monitor can therefore observe a publication without requiring a separate multicast Route or additional transmission.

If multiple participants on one side must actively transmit in the same Direction, the base `RouteId + Direction` model is insufficient to identify the producer and additional semantics would be required.

### Forwarders and Replicators

A **transparent forwarder** preserves the canonical `RouteId`, Direction, endpoint identity, and end-to-end transport semantics while replacing only hop-local/link framing as necessary. Forwarding does not make the gateway the producer or transport peer.

A component that intentionally receives traffic under one Route and originates semantically new traffic under another Route is a **replicator**, not a transparent forwarder. Replication carries new authority and should be represented explicitly in static configuration.

## 11. Constrained-Link Route Aliases

A constrained Logical Link Layer may encode a canonical RouteId using a smaller link-local alias.

The alias is not part of the canonical route identity. It is expanded by the receiving LLL before delivery upward and may be re-encoded differently on the next physical segment.

### Recommended 11-bit Classical CAN mapping

With the previous `UpstreamHostAlias` removed, the recommended CAN arbitration-ID allocation can provide a 7-bit Route alias:

```text
bits 10:9   CAN priority code
bits  8:2   RouteAlias       // 7 bits, 0..127
bit     1   Direction
bit     0   protocol enable

protocol enable = 0   raw/legacy CAN domain
protocol enable = 1   PDU-adapter domain
```

Thus a physical Classical CAN segment can carry up to 128 directly encoded Route aliases while the canonical RouteId namespace remains larger. Configuration tooling shall reject a CAN segment whose set of traversing Routes cannot be represented by its available alias space.

For the recommended profile, the 2-bit CAN priority code should map one-to-one with the canonical 2-bit QoS value so canonical QoS can be reconstructed without consuming CAN payload bytes. A different mapping requires an explicit named profile with unambiguous reconstruction semantics.

Static configuration/code generation defines:

```text
CAN RouteAlias <-> canonical RouteId
```

The same canonical Route may therefore use different RouteAlias values on different CAN segments of a multi-hop path. All peers on a physical segment must agree on the alias table; generated configuration should include a digest/fingerprint or equivalent compatibility check so mismatched RouteAlias definitions fail closed rather than silently misroute traffic.

For simple systems, aliases may be allocated to closely resemble the low canonical RouteId values, but software above the LLL must not depend on alias identity.

---

## 12. Static Configuration and Code Generation

Static configuration is expected to define:

- Routing Hosts;
- local endpoint ownership;
- Route Side A / Side B participants;
- physical/link path for each Route;
- valid Directions;
- allowed EndpointIds per Route and Direction;
- multicast and observer membership;
- endpoint concurrency/access type;
- constrained-link aliases such as CAN RouteAlias and EndpointId aliases.

Code generation should eventually produce:

- endpoint lookup tables;
- Route routing/forwarding tables;
- local typed endpoint handles;
- per-Route endpoint allowlists;
- compile-time/static validation;
- CAN acceptance/filter and alias configuration;
- invalid-binding diagnostics.

A forwarding implementation may conceptually reduce to:

```text
(RouteId, Direction)
        |
        v
static route lookup
        |
        +--> local delivery
        +--> output Port
        +--> bounded output Port set for multicast
```

Useful validation includes:

- endpoint IDs referenced but not hosted;
- remote endpoints reachable on an unintended Route;
- multiple writers to a single-writer endpoint;
- invalid multicast reverse direction;
- routing loops;
- RouteAlias collisions;
- missing route entries or next hops;
- inconsistent Route definitions across participating devices;
- incompatible RouteAlias/EndpointAlias tables across peers on one link;
- canonical or per-link Route capacity overflow;
- observer bindings that accidentally grant transmit authority.

Generated deployments should expose or embed a configuration fingerprint/version sufficient to detect incompatible route/binding/alias configurations during integration or startup diagnostics.

---

## 13. Route Delivery Semantics

A Route defines static reachability, not an independent reliability, ordering, timing, or flow-control guarantee. Those properties come from the selected transport and the constituent Logical Link Layers.

A transparent forwarder may drop a message when bounded egress resources are exhausted; it does not create implicit backpressure or reliability beyond the configured transport. Multi-hop Routes likewise provide no stronger ordering guarantee than the selected transport/profile explicitly defines. Services that require freshness, deadlines, ordering, or stronger delivery semantics must select or define the appropriate transport/service profile.

---

## 14. Core Design Invariants

1. **Routes connect Routing Hosts, not individual tasks or services.**
2. **`RouteId + Direction` identifies a statically configured communication relationship/path; `Namespace + EndpointId` identifies the receiving service.**
3. **For every valid `(RouteId, Direction)`, exactly one active source identity exists.**
4. **Components in the same Routing Host communicate through the local endpoint namespace and do not allocate non-local RouteIds for ordinary local IPC.**
5. **`RouteId::kLocal` is the canonical representation of local delivery and Direction shall be zero.**
6. **Reserved routing bits are transmitted as zero; unsupported nonzero values are rejected.**
7. **Direction identifies traversal between fixed Side A and Side B. Its meaning does not change with requester/responder, initiator, producer/consumer, or service-role semantics.**
8. **EndpointId is service identity, not authorization.**
9. **Remote endpoint access requires an explicit `(RouteId, Direction, Namespace, EndpointId)` binding.**
10. **Ordinary application components should receive typed/configured endpoint handles rather than unrestricted router access, but typed handles alone are not an isolation/security boundary.**
11. **Endpoint implementations define their concurrency and ownership semantics.**
12. **One processor = one Routing Host is a convenient default, not a mandatory architectural boundary.**
13. **Statically configured ECU-to-ECU, Main-to-Main, partition-to-partition, and other peer Routes are permitted; reachability exists only where explicitly configured.**
14. **Passive observers are receive-only bindings and never become active producers or transport peers merely by observing traffic.**
15. **Transparent forwarders preserve canonical Route, Direction, endpoint identity, and end-to-end transport semantics while changing only hop-local framing/routing state.**
16. **A replicator that re-originates traffic onto another Route is distinct from a transparent forwarder and carries explicit new authority.**
17. **`(RouteId, Direction)` supplies configured producer identity for ordinary routed traffic; a separate runtime `ProducerKey` is not required by the base model.**
18. **Constrained links may alias RouteId on the wire, but aliases are link-local encodings and never replace the canonical RouteId above the LLL.**
19. **A Route provides no reliability, ordering, freshness, timing, or flow-control guarantee beyond the selected transport and constituent links.**

---

## 15. Superseded Concepts

This Route-based model supersedes earlier routing concepts that depended on `VCN`, `Index`, `UpstreamHost`, `UpstreamHostAlias`, `kMainCompute`, paired local-host markers, or a derived runtime `ProducerKey`. Those concepts should not be mixed with the Route model in normative specifications or generated configurations.

The controlling model is:

```text
RouteId + Direction
    -> configured Routing-Host relationship and source/destination orientation

Namespace + EndpointId
    -> service identity at the receiving Routing Host

Static bindings
    -> reachability and authority
```
