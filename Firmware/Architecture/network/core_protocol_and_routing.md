# Core Protocol and Routing

## 1. Status and scope

**Status: Provisional core specification.** This document is normative for the
initial prototype where it uses **SHALL**, **SHALL NOT**, **SHOULD**, or **MAY**,
but it is not a frozen wire-protocol release. Items labelled **Provisional** may
change after prototype validation. Items labelled **Deferred** are not part of
the initial prototype.

This document defines the canonical message descriptor, endpoint identity,
service-role direction, Virtual Circuit routing, peer binding, publication,
transparent forwarding, static validation, and core error behavior. It defines
logical semantics above the Logical Link Layer (LLL); it does not require every
LLL to serialize the canonical descriptor byte-for-byte.

The following companion documents are planned as part of the consolidated set:

- [Document map](README.md) — authority, terminology, document status, and
  consolidated open-issue index.
- [Architecture overview](architecture_overview.md) — goals, layering,
  deployments, security boundary, and system-wide invariants.
- [Application protocols and services](application_protocols_and_services.md) —
  endpoint APIs, service patterns, schemas, and application error protocols.
- [Logical links and transports](logical_links_and_transports.md) — LLL
  contracts, capabilities, transport profiles, and concurrency.
- [Link Entity runtime and status](link_entity_runtime_and_status.md) — Link
  Entity lifecycle, recovery, observability, and status behavior.
- [Experimental CAN PDU adapter](can_pdu_adapter_spec.md) — CAN encoding,
  framing, reassembly, and integrity details.
- [Prototype and validation](prototype_and_validation.md) — implementation
  boundary, test matrix, conformance evidence, and promotion criteria.
- [Rationale, use cases, and risks](rationale_use_cases_and_risks.md) —
  non-normative examples, alternatives, adoption concerns, and security risks.

Reliable transport state machines and exact CAN behavior are outside this
document. The initial prototype uses Unreliable Datagram and must not infer
reliable or CAN semantics from this specification.

## 2. Core terms

- **Main**: a device eligible for a compact `UpstreamHost` identity. Main is a
  deployment role, not a required service role.
- **ECU**: a non-Main embedded node. This name does not imply a particular
  processor, operating system, or physical link.
- **Virtual Circuit (VC)**: a statically configured connectivity and policy
  domain. A **Virtual Circuit Number (VCN)** identifies that domain.
- **Index**: a compact, VCN-scoped member identifier. The same numeric Index MAY
  identify different members in different VCs.
- **Upstream service role**: the initiator, controller, or stimulus side of one
  directed service instance.
- **Downstream service role**: the responder or controlled side of that service
  instance.
- **Producer**: the endpoint instance that originates a publication.
- **ProducerKey**: opaque deployment-local metadata identifying one statically
  configured publication producer.
- **Passive listener**: a configured consumer that is not the addressed peer
  and does not participate in peer or transport state.
- **Transparent forwarder**: a configured same-VC relay that preserves
  canonical end-to-end semantics.
- **Cross-VC replicator**: a configured component that terminates an
  observation on one VC and originates a distinct authorized publication on
  another VC.

## 3. Canonical working descriptor

### 3.1 Representation

The canonical representation above the LLL is:

```text
Control       8 bits
Routing      16 bits
EndpointId   16 bits
--------------------
Total        40 bits
```

This 5-byte logical descriptor is **Provisional**. An LLL MAY encode fields in
native metadata, omit values that static configuration reconstructs
unambiguously, or use a named compact profile. Before delivery to the endpoint
router, it SHALL reconstruct the same canonical values. Wire order, bit order,
and profile-specific encodings belong in the LLL specifications.

The working logical partition is:

```text
Control (8)
    Namespace          2
    Reserved           2
    Multicast control  1
    Transport selector 3

Routing (16)
    QoS selector       2
    Direction          1
    UpstreamHost       3
    VCN                5
    Index              5

EndpointId (16)
```

The widths and partition above are the prototype contract, but the packed bit
positions and numeric assignments are not frozen. Reserved control bits and
the multicast-control bit SHALL be transmitted as zero in the initial
prototype. A receiver SHALL treat a nonzero unassigned bit as an unsupported or
malformed descriptor rather than guessing its meaning.

### 3.2 Namespace and endpoint identity

`Namespace` is a 2-bit component of endpoint identity. The provisional
allocation is:

```text
0, 1, 2  deployment-defined namespaces
3        open ecosystem namespace; allocation governance remains TBD
```

The logical endpoint identity is:

```text
(Namespace, EndpointId)
```

Routing fields, service role, physical location, and transport selection are
not part of endpoint identity. A reusable service MAY provide a default
identity, but the deployment owns its final allocations and instances.

Literal `EndpointId` values are `0x0100` through `0xFFFF`. Application code,
transport code, endpoint routing, capture, and forwarding SHALL use literal
IDs.

Values `0x0000` through `0x00FF` are reserved for optional link-local aliases.
An alias:

- exists only inside one statically configured LLL;
- SHALL map to exactly one literal ID in that link configuration;
- SHALL be expanded before delivery above the LLL;
- SHALL NOT be exposed to services, transports, routers, or other LLLs;
- does not change endpoint identity.

Aliasing is disabled unless explicitly configured. An alias-bearing profile
SHALL fail closed unless both peers derive their alias tables from the same
generated topology artifact or have compatible alias digests provisioned by an
out-of-band mechanism. An unknown or mismatched digest SHALL disable the alias
profile for the affected peer or link, and traffic using that profile SHALL
remain disabled. A separately configured literal-ID profile MAY be used
instead, but implementations SHALL NOT fall back to or negotiate that profile
automatically. Alias-table and digest consistency are
configuration-validation requirements.

### 3.3 QoS

**Deferred:** numeric QoS ordering is unresolved. The 2-bit field is an opaque
selector for a named, profile-defined symbolic class. APIs, configuration, and
design documents SHALL use symbolic class names and SHALL NOT assume that a
larger or smaller numeric value has higher priority. Class names, numeric
mapping, arbitration mapping, and scheduling behavior belong in the link and
prototype specifications.

### 3.4 Direction

`Direction` selects the **destination service role**. It does not describe
physical motion, hierarchy, ingress/egress, or which device is currently
transmitting.

Conceptually its two values are:

```text
ToUpstreamRole
ToDownstreamRole
```

The exact symbolic and numeric encoding is **Provisional**.

An endpoint's upstream/downstream role is fixed for one directed service
instance. A downstream endpoint MAY respond immediately or transmit
asynchronously later; when sending to its upstream peer it selects the upstream
destination role. This transmission does not reverse either endpoint's role.
Genuinely symmetric protocols SHOULD compose two oppositely directed service
instances.

### 3.5 UpstreamHost

`UpstreamHost` normally identifies a configured Main device. It:

- SHALL NOT be interpreted as a general node or ECU address;
- SHALL NOT substitute for the complete downstream target `(VCN, Index)`;
- does not prove that any service role is placed on that Main;
- does not imply physical upstream/downstream topology;
- SHALL be validated against the deployment's configured Main identities.

Local IPC/ICC uses the reserved symbolic value
`UpstreamHost::kLocalHost`. This value is not a Main identity or node address
and is valid only when the VCN field contains
`VirtualCircuit::kLocalHost`. The two symbols belong to different field types;
their numeric values are **Provisional** and are not required to match. A
router or LLL SHALL reject `UpstreamHost::kLocalHost` on external ingress or
egress, and SHALL reject either local symbol without the other.

A Main MAY host either service role. The exact `UpstreamHost` value allocation
and the relationship between `UpstreamHost` and a Main's Index on
`kMainCompute` are open issues.

## 4. Virtual Circuits and addressing

### 4.1 VC semantics

A VCN identifies a static connectivity and policy context, not a physical bus
and not a hop-by-hop route. A VC MAY be implemented by one link, several
heterogeneous links joined by transparent forwarders, a local IPC mechanism,
or a statically selected subset of a physically richer topology.

The packet carries the VCN and Index; configuration contains the physical path.
Nodes SHALL NOT discover or install routes dynamically merely because traffic
is observed.

An Index is meaningful only within its VCN. Individual Index assignments used
as unicast members SHALL be unique within that VC. No globally unique node
address is defined by this protocol.

### 4.2 Downstream target

The complete node-level target for a message addressed to a downstream service
role is:

```text
(VCN, Index)
```

`EndpointId` and `Namespace` then select the service endpoint at that target.
Neither `Index` alone nor `UpstreamHost` alone is a complete downstream target.

This document intentionally does not freeze more general source/destination
field behavior. In particular, it does not invent a universal source address
field or redefine `UpstreamHost` to fill that gap. Profiles and endpoint APIs
shall be validated against the accepted downstream-target rule before broader
field behavior is standardized.

### 4.3 Symbolic reserved VCs

The VCN space reserves the following symbolic identities. Their numeric values
are **Provisional** and SHALL be assigned by the consolidated profile:

- `kLocalHost`: communication entirely within one host or node.
- `kMainCompute`: communication among configured Main devices.

Traffic on `VirtualCircuit::kLocalHost` SHALL remain inside the current host
and SHALL carry `UpstreamHost::kLocalHost`. It SHALL NOT be emitted onto an
external LLL, forwarded by a gateway, or reconstructed from external ingress.
A local implementation MAY use Index to select local cores, tasks, processes,
or routing domains, subject to static configuration. The numeric assignments
of both local symbols remain **Provisional**.

`kMainCompute` is an ordinary statically routed VC with Main-only membership.
Each participating Main has a VCN-scoped Index. Main-to-Main services use the
same endpoint identity, Direction, peer binding, and forwarding rules as other
services. The protocol does not define a separate inter-Main message format.

## 5. Endpoint peer binding and replies

Peer binding is an endpoint/router configuration behavior, not a new wire
field. One endpoint registration SHALL have exactly one of these five binding
modes:

1. **Static binding** — a peer or target is fixed by generated configuration.
2. **Learned-from-ingress binding** — a bounded peer binding may be learned
   only from an authorized ingress mode described below.
3. **Request-scoped binding** — an incoming request yields an opaque
   `ReplyContext` valid only for the associated operation or bounded lifetime.
4. **Receive-only** — the endpoint consumes configured traffic and has no
   network transmit peer.
5. **Transmit-only** — the endpoint originates configured traffic and has no
   inbound peer binding.

Learned-from-ingress binding SHALL be disabled on unauthenticated multi-access
LLLs. It MAY be enabled only for either a statically single-peer ingress or a
named authenticated-origin profile, and only within a preconfigured allowlist
of permitted origin, service, VC, Index, and interface constraints. Route, VC,
Index, interface, and descriptor validation are necessary routing checks; they
are not authentication. Each enabled learned mode SHALL define finite
capacity, expiry, replacement/eviction policy, restart invalidation or
restoration behavior, and auditable learn, replace, expire, reject, and clear
events. Prototype conformance SHALL test all enabled binding modes rather than
assuming learned-from-ingress binding is unconditionally available.

A protocol that needs request-scoped replies and autonomous transmission SHALL
use separate endpoint registrations, or explicit bounded named sub-bindings
whose independent authority and lifetime are part of the service
configuration. Separate registrations are RECOMMENDED for the initial
prototype. Each named sub-binding SHALL have one declared binding mode,
authority, and lifetime. This requirement specifies composition, not API names
or spelling. A complex multi-peer/session service may manage an explicitly
bounded collection of named sub-bindings, but it does not gain arbitrary node
addressing.

`ReplyContext` SHALL contain or reference enough validated router state to
construct a role-correct reply without requiring application code to manipulate
raw VCN, Index, Direction, or `UpstreamHost` fields. It SHALL be opaque to
ordinary service code, bounded in lifetime and storage, invalidated according
to documented restart/configuration rules, and unusable as authority for a
different service operation. Its exact fields are intentionally not specified
here.

## 6. Supported communication patterns

### 6.1 Main and ECU

A common command pattern places the upstream service role on a Main and the
downstream role on an ECU. Traffic to the ECU selects the downstream role and
uses the ECU's complete `(VCN, Index)` target. A response or later asynchronous
transmission from the ECU selects the upstream destination role and retains the
same endpoint roles. `UpstreamHost` identifies the configured Main involved; it
does not address the ECU. Local-only service instances instead use the paired
`UpstreamHost::kLocalHost` and `VirtualCircuit::kLocalHost` symbols.

The placement may be reversed for a service. For example, an ECU error source
may hold the upstream role while a Main logger holds the downstream role.
Device category never overrides the service's configured roles.

### 6.2 Main and Main

Main-to-Main communication occurs on `kMainCompute`. One Main hosts the upstream
role and another the downstream role for a directed service instance. The
downstream Main is targeted by `(kMainCompute, Index)`. A duplex relationship is
two directed instances.

`UpstreamHost` remains a compact Main identity but is not assumed to equal,
alias, or derive from the `kMainCompute` Index. That relationship is TBD and
configuration SHALL treat the two identities as distinct until resolved.

### 6.3 ECU observation without ECU unicast

Arbitrary ECU-to-ECU unicast is not supported. In particular, the core router
SHALL NOT turn one ECU's Index into a general address for commanding another
ECU on the same or a different VC.

An ECU MAY nevertheless consume another ECU's publication when the receiving
link exposes the traffic and a passive-listener subscription explicitly allows
it. This is observation of a configured producer, not peer addressing.

## 7. Publications, listeners, and producer identity

A publication SHALL carry runtime metadata containing an opaque,
deployment-local `ProducerKey`. `ProducerKey` is assigned by static
configuration for prototype, configuration, and capture use. For received
traffic it SHALL be derived or attached only after ingress and routing
validation; a locally originating endpoint obtains only the key assigned to
its static registration. It is not a frozen wire field. Endpoint identity,
physical visibility, and unvalidated source fields SHALL NOT create a
`ProducerKey`.

Exactly one producer SHALL be authorized for each `ProducerKey` unless the
service defines explicit multi-producer authority and arbitration. Endpoint
identity alone may be reused by several nodes, so consumers and capture SHALL
retain the validated `ProducerKey`. The exact source-field and return-path
semantics remain open and are not frozen by this metadata rule.

A passive listener:

- MAY be one of any number of statically configured listeners consuming an
  explicitly authorized publication visible on an attached ingress link;
- is not the addressed peer and receives no delivery guarantee;
- SHALL NOT acknowledge, alter flow control, or affect peer state;
- SHALL NOT cause the producer to infer successful delivery;
- SHALL retain the publication's validated `ProducerKey`;
- SHALL preserve producer attribution when storing or explicitly republishing
  data.

Physical visibility and endpoint identity do not create a subscription, reply
authority, or forwarding authority. Listeners and producers SHALL be declared
by static configuration. A transparent forwarder MAY carry an authorized
publication on the same VC to another attached link; listening alone SHALL NOT
cause forwarding. A transparent forwarder or listener never becomes the
producer merely because it retransmits or stores a value.

## 8. Forwarding and cross-VC publication

### 8.1 Same-VC transparent forwarding

Forwarding is disabled unless explicitly configured. A transparent forwarder
MAY continue one VC over another physical link. It SHALL:

- preserve the VCN and Index;
- preserve `(Namespace, EndpointId)`;
- preserve Direction and `UpstreamHost`;
- preserve end-to-end transport state and application payload;
- avoid terminating, acknowledging, regenerating, or translating end-to-end
  transport state;
- never become the producer or service peer;
- modify only hop-local LLL framing required by the egress profile.

If an egress LLL cannot preserve or unambiguously reconstruct these canonical
semantics, the route is invalid. Transparent forwarding never changes VCN.

### 8.2 Cross-VC replication

Cross-VC publication is not routing. A component that consumes a publication
on one VC and emits it on another is a **cross-VC replicator**. It terminates
one observation and originates a distinct publication under the destination
VC's policy.

Every cross-VC replication edge SHALL be explicitly configured, bounded,
observable, loop-free, and distinguishable from a transparent forwarder. Each
edge SHALL declare:

- the eligible publication message types;
- the authorized source `ProducerKey`;
- the destination `ProducerKey` and its authority owner;
- how original provenance is represented at the destination; and
- the conflict policy.

Commands, RPC requests or responses, service errors, and arbitrary traffic
SHALL NOT traverse a cross-VC replication edge unless the service separately
defines explicit proxy semantics, authority, correlation, failure behavior,
and bounds. For control-relevant state, original provenance SHALL be
representable and retained. Configuration and runtime validation SHALL reject
competing producers for a destination `ProducerKey` unless the service defines
explicit authority and arbitration. A cross-VC replicator SHALL NOT silently
masquerade as the original producer.

Device-wide status published over several healthy VCs is one example of
intentional cross-VC publication. It is not evidence that those VCs form one
route.

### 8.3 Multicast and groups

**Deferred:** the initial prototype defines no generic multicast wire request,
no generic group-Index range, and no dynamic group membership. The multicast
control bit remains zero/unassigned.

Initial one-to-many behavior is limited to:

- physical link visibility plus configured passive listeners;
- separately configured transmit/publication instances; or
- explicit bounded cross-VC replicators.

Multicast/group encoding, fan-out ownership, producer attribution, error
behavior, and reliable interaction require a later specification.

## 9. Static route and configuration validation

Each VC SHALL have a deterministic, statically generated forwarding topology.
Physical loops MAY exist, but the active forwarding graph for every configured
VC and target SHALL be loop-free. The initial protocol does not add TTL,
spanning-tree negotiation, route discovery, or duplicate-path suppression to
repair invalid configuration at runtime.

Build-time tooling SHALL reject, at minimum:

- duplicate individual Index assignments within a VC;
- undefined or illegal VCN, Index, namespace, EndpointId, or symbolic
  reservation use;
- missing next hops and unreachable configured targets;
- forwarding cycles and unintended ambiguous replication;
- `kLocalHost` paths that touch an external LLL;
- cross-VC entries represented as transparent routes;
- ingress paths not authorized for the VC;
- use of `UpstreamHost::kLocalHost` without `VirtualCircuit::kLocalHost`, or
  either local marker on external ingress or egress;
- inconsistent endpoint-alias mappings, unknown alias digests, and mismatched
  alias digests for alias-bearing profiles;
- learned-from-ingress enablement on unauthenticated multi-access links or
  outside its configured peer/origin allowlist;
- competing producers for one `ProducerKey` without explicit authority and
  arbitration;
- incomplete or overbroad cross-VC replication edges;
- LLL/profile, MTU, descriptor-field, and transport capability mismatches.

Every deployed node SHALL expose a version or fingerprint covering the
network-relevant static configuration, plus the capabilities needed to validate
its configured paths. Fingerprints need not appear in every application
message. Management/diagnostic services SHOULD compare expected and observed
fingerprints and capabilities at integration time and runtime.

A mismatch SHALL NOT cause dynamic route creation or silent reinterpretation.
The affected operation SHALL fail closed or enter an explicitly configured
degraded mode, with a local diagnostic record.

## 10. Error handling, congestion, and recovery

Receivers and routers SHALL validate descriptors, endpoint configuration,
route authority, reserved values, profile capabilities, and message bounds
before dispatch or forwarding. Invalid traffic SHALL be dropped or rejected at
the detecting boundary and counted by a stable local reason.

Core reason categories SHOULD include malformed descriptor, unknown namespace
or endpoint, unknown VC or target, no route, unexpected ingress, unsupported
transport/profile, capability or fingerprint mismatch, prohibited
local-marker ingress or egress, producer conflict, congestion, and
reassembly/link failure. Exact enums and wire reporting are defined elsewhere.

Remote infrastructure-error reports are optional. When enabled, they SHALL be
bounded, rate-limited, aggregatable, and lower in scheduling importance than
critical service traffic. An error report SHALL NOT trigger another
infrastructure-error report. Local counters remain authoritative even when a
remote report is suppressed or cannot be sent.

Transmit congestion and a temporarily unavailable route/link are normal
runtime outcomes. A transmit API SHALL report non-acceptance explicitly.
Services SHALL define bounded retry, replacement, queueing, or drop behavior and
SHALL NOT advance protocol state as though an unaccepted message had entered
the transport.

A link or Link Entity MAY restart without resetting the node. Restart SHOULD
discard uncertain transient framing, reassembly, queue, and hop-local state
while retaining static configuration, persistent counters, latched fault
information, and defined higher-layer ownership. Services and transports own
any required conversation recovery. Health/status publication over an
unaffected VC is an explicit publication operation, not automatic rerouting.

## 11. Core invariants

1. The canonical working descriptor is 8-bit Control, 16-bit Routing, and
   16-bit `EndpointId`; profile encodings must reconstruct it.
2. Endpoint identity is `(Namespace, EndpointId)`, with services using literal
   IDs `0x0100..0xFFFF`.
3. Low EndpointId aliases are private to one LLL and never cross its upper
   boundary.
4. Direction selects the destination service role, not physical movement.
5. A downstream endpoint may transmit asynchronously without reversing roles.
6. `UpstreamHost` normally identifies a configured Main and is not a general
   node address; its reserved local marker is valid only with the reserved
   local VC marker and never crosses an external LLL.
7. The complete downstream node target is `(VCN, Index)`.
8. Index is VCN-scoped; the packet identifies a VC and member while static
   configuration contains the physical route.
9. Paired `UpstreamHost::kLocalHost` and `VirtualCircuit::kLocalHost` traffic
   never reaches an external LLL; the symbols have distinct field types and
   provisional numeric values.
10. Main-to-Main traffic uses `kMainCompute`; its Index relationship to
    `UpstreamHost` remains unresolved.
11. Arbitrary ECU-to-ECU unicast is not a core routing capability.
12. Passive observation does not create a subscription, peer, producer, reply
    or forwarding authority, acknowledgment obligation, or delivery guarantee.
13. Same-VC transparent forwarding preserves canonical identity, routing,
    transport state, and payload and never changes producer or peer.
14. Cross-VC replication is explicit publication origination, not routing, and
    each edge is constrained by message type, producer authority, provenance,
    and conflict policy.
15. Each VC's configured forwarding graph is loop-free and validated before
    deployment.
16. The initial prototype has no generic multicast request and uses symbolic
    QoS classes without numeric ordering assumptions.
17. Configuration disagreement is diagnosed; it never authorizes dynamic
    reconfiguration or guessed semantics.

## 12. Open issues

The following are explicit specification gaps, not implementation latitude:

1. Freeze packed bit positions, byte order, reserved-bit policy, and named
   canonical/wire profile versions.
2. Assign numeric values separately for `UpstreamHost::kLocalHost`,
   `VirtualCircuit::kLocalHost`, `kMainCompute`, transport selectors, Direction,
   and symbolic QoS classes.
3. Define the exact relationship, if any, between `UpstreamHost` and
   `kMainCompute` Index.
4. Complete source/origin and return-path semantics without weakening the
   accepted downstream target `(VCN, Index)` or turning `UpstreamHost` into a
   general address.
5. Specify the `ReplyContext` API, lifetime representation, authorization, and
   behavior across endpoint, router, and Link Entity restart.
6. Freeze neither `ProducerKey` nor source/origin metadata as a wire field;
   define any later interoperable provenance representation for duplicate
   EndpointIds, capture, passive listeners, and cross-VC attribution.
7. Standardize QoS symbols, numeric mapping, scheduling contracts, and
   link-specific arbitration behavior.
8. Decide whether and how multicast/group addressing is added after the
   unicast/publication prototype, including interaction with producer identity
   and transport semantics.
9. Define configuration-fingerprint coverage, capability schema, comparison
   protocol, and degraded-mode policy.
10. Define immutable conformance profiles and vectors in the link/transport and
    CAN specifications.
11. Specify Reliable Transport separately when validated by a concrete service.

## Appendix A. Non-normative routing examples

### A.1 Main command and asynchronous ECU status

Main A's upstream endpoint sends a command to the downstream endpoint at
`(MotionVC, MotorEcuIndex)`. The ECU later sends status to the upstream role
without changing either endpoint's role. `UpstreamHost` identifies Main A; it
does not address the motor ECU.

### A.2 Passive same-VC observation

Steering ECU publishes state toward its configured Main peer on `ChassisVC`.
Brake ECU can see the frame on the shared link and has a static passive-listener
subscription. It consumes the value without acknowledging it or becoming
Steering ECU's peer. Its listener retains Steering ECU's validated
`ProducerKey` and gains no reply or forwarding authority.

### A.3 Transparent heterogeneous continuation

A message on `ControlVC` crosses CAN, a gateway, and UART. The gateway changes
LLL framing only. VCN, Index, endpoint identity, roles, transport state, and
payload remain unchanged, so the gateway is neither producer nor peer.

### A.4 Explicit cross-VC status replication

An ECU status publication is observed on `PowertrainVC`. A configured cross-VC
replicator emits a separate summary on `TelemetryVC`. Its edge names the source
and destination `ProducerKey` values, eligible status message type, provenance,
authority owner, and conflict policy. The second message is an explicit
publication under `TelemetryVC` policy, not a route continuation.

### A.5 Main-to-Main service

Main A's upstream service instance addresses Main B's downstream instance at
`(kMainCompute, MainBIndex)`. Main B replies using request-scoped context.
Configuration treats `MainBIndex` and Main B's `UpstreamHost` identity as
distinct until their relationship is standardized.
