# Classical CAN PDU Adapter

**Status:** Experimental / Work in Progress  
**Scope:** Logical Link Layer adaptation of bounded PDUs to Classical CAN  
**Interoperability:** Not yet claimed; no wire profile is frozen

This document separates three kinds of material:

- **Accepted contract** — architectural requirements that implementations shall preserve.
- **Candidate design** — the current prototype direction, explicitly subject to incompatible change.
- **Unresolved items** — decisions that must not be inferred from old drafts or candidate examples.

Normative words such as **shall** and **must** apply only to the accepted contract unless a named wire profile later adopts a candidate rule. Candidate layouts are explanatory byte layouts, not C or C++ ABI definitions.

## 1. Relationship to the Consolidated Architecture

Read this specification with:

- the [architecture overview](architecture_overview.md) for scope and system boundaries;
- [core protocol and routing](core_protocol_and_routing.md) for endpoint, Virtual Circuit, direction, listener, forwarding, and replication semantics;
- [application protocols and services](application_protocols_and_services.md) for publication and transport use;
- [logical links and transports](logical_links_and_transports.md) for the generic LLL contract and backpressure;
- [prototype and validation](prototype_and_validation.md) for system-level validation;
- [rationale, use cases, and risks](rationale_use_cases_and_risks.md) for non-normative context.

The [document index and authority guide](README.md) controls the relationship between this specification and the source drafts.

## 2. Accepted Adapter Contract

### 2.1 Service

The CAN PDU Adapter is a Classical CAN Logical Link Layer. It carries one complete, bounded upper-layer PDU in `N` constituent CAN data frames.

The adapter shall provide:

1. **Unreliable datagram delivery.** It does not guarantee that an accepted PDU reaches a receiver.
2. **All-or-nothing receive visibility.** A complete validated PDU is delivered upward, or nothing is delivered.
3. **No adapter-level PDU ACK or retry.** Reliability, acknowledgment, duplicate suppression, and end-to-end retransmission belong to a higher transport when required.
4. **Bounded operation.** Maximum PDU size, frame count, active contexts, queueing, and reassembly memory are statically bounded.
5. **Allocation-free feasibility.** A conforming implementation can use fixed buffers and fixed context tables without runtime allocation.

Classical CAN controller acknowledgment and automatic retransmission remain below this adapter. They do not constitute a PDU acknowledgment and do not change the adapter's unreliable service contract.

### 2.2 Framing and ownership invariants

- Every constituent frame of one PDU shall use the same CAN arbitration ID.
- A protocol CAN ID shall have one statically unique transmitting owner in every state where it can be used. Two nodes must not transmit different data under the same ID.
- Raw CAN and protocol-adapter traffic shall occupy disjoint, statically configured CAN-ID domains.
- Raw CAN traffic shall retain raw-link semantics and incur no PDU-adapter data-field overhead.
- Small `N` is preferred because latency, bus occupancy, and probability of whole-PDU loss increase with aggregation depth.
- No upper-layer callback, queue, endpoint, listener, forwarder, or transport may observe a partial PDU.

Static configuration shall prevent CAN-ID collisions; an on-wire discriminator alone does not prove collision-free coexistence.

### 2.3 Topology boundary

This adapter does not provide general ECU-to-ECU unicast routing. In particular, it does not create an address by which ECU X can directly target arbitrary ECU Y across buses.

CAN and other supported links may have any number of passive listeners. An ECU may therefore consume another ECU's publication when higher-layer configuration permits it.

Two distinct higher-layer topology operations are permitted and shall not be conflated:

- **Transparent forwarding** carries a completed publication onward on the same Virtual Circuit while preserving its canonical routing and publication semantics.
- **Cross-VC replication** consumes a completed publication on one Virtual
  Circuit and reoriginates a publication on each statically configured
  destination Virtual Circuit, including state-sharing groups. Continuing the
  same VC across another physical bus segment is transparent forwarding, not
  replication.

Listening, transparent forwarding, and cross-VC replication are explicit pub/sub behaviors. None provides general ECU routing, and none operates on adapter fragments.

### 2.4 Diagnostics and interoperability

- Implementations shall maintain bounded local counters for accepted, rejected, completed, aborted, delivered, and discarded work.
- Malformed traffic shall be counted and dropped locally; it shall not cause an adapter-level error response.
- Exact counter names, widths, aggregation, and remote diagnostic schema remain unresolved.
- Independent interoperability shall not be claimed until an immutable named profile and its positive and negative conformance vectors are published.
- An incompatible wire change requires a new profile name/version. A published profile is never silently reinterpreted.

## 3. Accepted State-Machine Requirements

### 3.1 Transmit

Before emitting the first frame, TX shall:

1. validate the selected route/profile and CAN-ID ownership;
2. verify that the complete PDU fits configured bounds;
3. obtain all storage and queue capacity required by its documented submission contract;
4. preserve a stable view of the PDU bytes and metadata for the duration of frame emission.

TX shall then emit one finite frame sequence under one CAN ID. It shall not report remote delivery success. If emission aborts after the first frame, TX shall stop the attempt, release bounded local state, and increment a local abort reason; it shall not retry the PDU as adapter behavior.

An implementation may reject a local submission because of congestion or invalid configuration. Such rejection means the PDU did not enter the link and is distinct from loss after transmission begins.

### 3.2 Receive

RX shall keep incoming bytes private to the adapter until all required frames are present and all applicable structural, length, mapping, and integrity checks pass.

Each active reassembly context shall have statically bounded storage and lifetime. On any failure that makes completion ambiguous or invalid, RX shall discard the complete context. It shall never deliver the validated prefix of an incomplete or rejected PDU.

The final transition is atomic from the upper layer's perspective:

```text
collecting -> complete and valid -> one whole-PDU delivery
collecting -> invalid/incomplete  -> discard with no delivery
```

The exact context key, collision policy, timeout, and ordering rules are wire-profile decisions and are not accepted as frozen below.

## 4. Current Candidate Wire Design — Not Frozen

Everything in this section is a prototype candidate. It must not be used as an interoperability claim.

### 4.1 Candidate profile summary

- Classical CAN data frames with at most eight data bytes.
- `N = 1..16`; `N = 1..4` is recommended and larger values require explicit justification.
- One `SequenceControl` byte in every protocol frame.
- An explicit `START` indication, a message generation, and a four-bit frames-remaining countdown.
- Exact encoded length derived from each frame's DLC; no unused-byte trailer.
- No additional PDU CRC for `N = 1`.
- CRC-8 for `N = 2..3`; CRC-16 for `N >= 4`.
- The multi-frame PDU CRC appears in the START frame.
- A compact endpoint alias and VCN appear in the START frame.
- One active PDU per CAN ID and no same-ID interleaving as the safe prototype default.

### 4.2 Candidate `SequenceControl`

```text
bit  7      START
bits 6:4    MessageGeneration[2:0]
bits 3:0    FramesRemaining
```

Candidate interpretation:

- The START frame has `START = 1` and `FramesRemaining = N - 1`.
- Every continuation has `START = 0`.
- `FramesRemaining` decrements by one on each constituent frame.
- The final frame has `FramesRemaining = 0`.
- A single-frame PDU has `START = 1` and `FramesRemaining = 0`.

The START frame also currently combines the upper generation bits with VCN:

```text
VirtualCircuitAndGeneration:
bits 7:5    MessageGeneration[5:3]
bits 4:0    VCN
```

This yields a candidate six-bit generation. Continuations carry only the low three bits and would bind to state established by START. Generation width, advancement, wrap handling, and binding rules are unresolved.

### 4.3 Candidate compact START layouts

The compact candidate assumes three per-PDU metadata bytes: `ProtocolControl`, `VirtualCircuitAndGeneration`, and an 8-bit endpoint alias.

```text
N = 1:
byte 0      SequenceControl
byte 1      ProtocolControl
byte 2      VirtualCircuitAndGeneration
byte 3      endpoint_alias
byte 4..    upper-layer PDU bytes

N = 2..3:
byte 0      SequenceControl
byte 1      CRC-8
byte 2      ProtocolControl
byte 3      VirtualCircuitAndGeneration
byte 4      endpoint_alias
byte 5..    upper-layer PDU bytes

N >= 4:
byte 0      SequenceControl
byte 1..2   CRC-16
byte 3      ProtocolControl
byte 4      VirtualCircuitAndGeneration
byte 5      endpoint_alias
byte 6..    upper-layer PDU bytes

Continuation:
byte 0      SequenceControl
byte 1..    upper-layer PDU bytes
```

The endpoint alias is link-local and would map statically to the canonical endpoint identity before delivery. Any optional group or multicast representation must use a separate explicit mechanism; `UpstreamHostAlias` cannot carry multicast or broadcast semantics. Exact control bits, CRC byte order, VCN semantics, endpoint-alias profiles, and the separate multicast representation remain unresolved.

### 4.4 Candidate DLC rule

Non-final multi-frame frames would normally use `DLC = 8`. The final frame, including an `N = 1` frame, would use the DLC for its exact encoded byte count. Bytes beyond DLC would not exist for reassembly or CRC coverage.

Zero-length PDU validity, minimum legal DLCs, short non-final frames, and all malformed-DLC behavior remain unresolved.

### 4.5 Candidate standard-ID mappings

The default 11-bit candidate reserves an enable/discriminator bit:

```text
bits 10:9   CAN priority code
bits  8:4   Index
bit     3   Direction
bits  2:1   UpstreamHostAlias
bit     0   protocol enable

protocol enable = 0   raw/legacy CAN domain
protocol enable = 1   PDU-adapter domain
```

The two-bit `UpstreamHostAlias` has exactly four singular alias codes:

```text
00  alias 0 -> profile-defined singular canonical UpstreamHost
01  alias 1 -> profile-defined singular canonical UpstreamHost
10  alias 2 -> profile-defined singular canonical UpstreamHost
11  alias 3 -> profile-defined singular canonical UpstreamHost
```

A future named profile must define a static one-to-one mapping from every alias code to exactly one singular canonical `UpstreamHost` value. None of the four codes has broadcast, multicast, wildcard, or other collective-host semantics.

An alternate expanded-host candidate uses:

```text
bits 10:9   CAN priority code
bits  8:4   Index
bit     3   Direction
bits  2:0   UpstreamHostAlias
```

The three-bit `UpstreamHostAlias` has exactly eight singular alias codes:

```text
000  alias 0 -> profile-defined singular canonical UpstreamHost
001  alias 1 -> profile-defined singular canonical UpstreamHost
010  alias 2 -> profile-defined singular canonical UpstreamHost
011  alias 3 -> profile-defined singular canonical UpstreamHost
100  alias 4 -> profile-defined singular canonical UpstreamHost
101  alias 5 -> profile-defined singular canonical UpstreamHost
110  alias 6 -> profile-defined singular canonical UpstreamHost
111  alias 7 -> profile-defined singular canonical UpstreamHost
```

The profile must define a static one-to-one mapping from every alias code to exactly one singular canonical `UpstreamHost` value. There is no implicit all-host code and no broadcast, multicast, or wildcard semantics. Canonical `UpstreamHost` values that are special, unrepresentable, or absent from the configured alias map shall be rejected. `UpstreamHost::kLocalHost` is invalid on external CAN and shall be rejected on transmit and receive.

`CAN priority code` is a link-local arbitration input, not an encoded canonical QoS value. A future named profile must provide explicit mapping vectors from supported symbolic canonical QoS values to CAN priority codes and the resulting CAN arbitration order. An implementation shall never copy canonical QoS bits into this field or infer numeric ordering among canonical QoS symbols.

The expanded mapping has no discriminator and therefore assumes the complete selected standard-ID space is assigned consistently to the protocol. Neither mapping is frozen. Canonical Direction, `UpstreamHost`, and `Index` mapping rules belong to the core routing contract and remain under reconciliation. These CAN-ID naming and semantic corrections do not change either candidate's bit allocation, layout, or capacity.

## 5. Candidate Capacity

For the compact candidate at maximum DLC, let:

```text
C = 0 bytes for N = 1
C = 1 byte  for N = 2..3
C = 2 bytes for N >= 4
```

Every frame spends one byte on `SequenceControl`, and the PDU spends three bytes on compact metadata. Maximum upper-layer PDU capacity is therefore:

```text
P_max(N) = 8N - N - 3 - C
         = 7N - 3 - C
```

Candidate maxima for `N = 1..16` are:

```text
N:       1   2   3   4   5   6   7   8   9  10  11  12  13  14  15  16
P_max:   4  10  17  23  30  37  44  51  58  65  72  79  86  93 100 107 bytes
```

If an optional multicast extension consumes `M` bytes and the selected transport consumes `T` bytes, candidate application capacity becomes:

```text
A_max(N) = 7N - 3 - C - M - T
```

These values exclude CAN arbitration, control, frame CRC, ACK, stuffing, and inter-frame overhead. They also assume the compact alias profile and the candidate CRC thresholds.

Reliable transport is not designed. Its overhead remains `T_reliable = TBD`; therefore this specification makes **no fixed reliable-capacity claim**.

## 6. Prototype-Safe Candidate State Machines

These state machines define a conservative prototype baseline, not a frozen wire contract.

### 6.1 TX candidate

```text
IDLE
  -> PREPARE
  -> EMIT_START
  -> EMIT_CONTINUATIONS
  -> COMPLETE
  -> IDLE

Any local failure after EMIT_START
  -> ABORT
  -> IDLE
```

`PREPARE` would select the smallest legal `N`, encode metadata, select CRC by `N`, compute the CRC when present, and assign the next generation. The sender would emit all frames under one CAN ID in countdown order and would not interleave another PDU under that ID.

Different CAN IDs may naturally interleave through CAN arbitration. Whether one adapter may schedule multiple local CAN IDs concurrently is an implementation resource decision.

### 6.2 RX candidate

The safe baseline keeps at most one active PDU per CAN ID:

```text
IDLE + valid START
  -> allocate/reset bounded context
  -> COLLECTING

COLLECTING + expected continuation
  -> append bytes
  -> refresh bounded timeout

COLLECTING + expected final frame
  -> validate length, mappings, structure, and CRC
  -> DELIVER whole PDU or DISCARD
  -> IDLE

COLLECTING + any ambiguous or invalid event
  -> DISCARD complete context
  -> IDLE
```

A new valid START under a CAN ID would discard and replace an incomplete context under that ID. An unexpected continuation, generation mismatch, or countdown mismatch would discard the affected context. This fail-closed behavior is suitable for a prototype, but duplicate, stale, out-of-order, collision, and replacement details must be frozen by the eventual profile.

A fixed global context pool may bound simultaneous reassembly across different CAN IDs. Pool exhaustion discards the newly arriving candidate PDU or follows another deterministic profile rule; it must never allocate dynamically or evict silently without a counter.

## 7. Discard and Abort Accounting

The exact diagnostics schema is unresolved, but prototype instrumentation should distinguish at least:

- invalid or unsupported CAN frame class;
- protocol ID outside configured ownership/domain;
- illegal DLC or encoded length;
- malformed START or illegal `N`;
- continuation without an active context;
- unexpected START/context collision;
- generation mismatch or stale frame;
- duplicate, skipped, or out-of-order countdown;
- reassembly timeout;
- context-pool or buffer exhaustion;
- PDU larger than configured maximum;
- CRC mismatch;
- invalid control/reserved bits;
- unknown endpoint alias or VCN mapping;
- unknown, duplicate, special, unrepresentable, or non-singular `UpstreamHostAlias` mapping;
- external-CAN use of `UpstreamHost::kLocalHost`;
- multicast, broadcast, or wildcard use through `UpstreamHostAlias`;
- missing or incompatible alias-map digest or fail-closed mapping configuration;
- missing or invalid symbolic QoS-to-`CAN priority code` mapping;
- invalid route, group, or broadcast encoding;
- upper-layer delivery rejection;
- TX validation, congestion, queue, controller, or mid-sequence abort.

Raw-domain frames are not malformed protocol frames; they are delivered to the configured raw path or ignored according to raw-link configuration.

## 8. Security and Non-Goals

The CAN frame CRC and candidate PDU CRC detect accidental corruption. They do not provide authentication, authorization, confidentiality, freshness, replay protection, or proof of source identity.

A node with bus access may inject, replay, delay, suppress, or flood frames. Static CAN-ID ownership is a configuration invariant, not a cryptographic control. Resource bounds, acceptance filters, timeouts, priority analysis, and rate-limited diagnostics reduce fault impact but do not make an untrusted bus secure.

This adapter does not provide:

- general ECU-to-ECU routing or internetworking;
- dynamic discovery, route negotiation, or profile negotiation;
- adapter-level reliable delivery;
- transport ordering or duplicate suppression above one reassembly attempt;
- end-to-end safety protection or security;
- arbitrary segmentation for large objects or byte streams;
- multicast membership or state-replication policy;
- remote diagnostic responses to malformed traffic.

Security-sensitive or remotely reachable deployments require reviewed higher-layer protection and gateway policy.

## 9. Unresolved Wire and Behavior Decisions

The following remain explicitly open:

1. Exact bit layouts, `ProtocolControl`, reserved-bit policy, and multicast extension.
2. Canonical routing reconstruction, Direction mapping, and named-profile symbolic QoS-to-`CAN priority code` mapping vectors and arbitration order.
3. The final standard-ID `UpstreamHostAlias`/`Index` profile, its static one-to-one singular canonical `UpstreamHost` map, ownership rules, rejection of special or unrepresentable hosts and external-CAN `UpstreamHost::kLocalHost`, alias-map digest, and fail-closed configuration behavior.
4. Standard versus extended identifiers and handling of RTR, error, and CAN FD frames.
5. Generation width, allocation, wrap, interleaving, and reassembly context keys.
6. Whether one active PDU per CAN ID becomes the mandatory safe profile.
7. Reassembly timeout definition, start point, refresh behavior, and configuration units.
8. Duplicate, out-of-order, skipped, stale, unexpected-START, and collision behavior.
9. CRC-8 and CRC-16 polynomial, initialization, reflection, xor-out, residue, coverage, byte order, and test vectors.
10. Whether CRC coverage includes CAN ID, generation, `N`, exact length, metadata, and all PDU bytes.
11. Zero-length PDUs, minimum DLCs, short non-final frames, and every DLC edge case.
12. Compact alias and literal endpoint profiles, mapping validation, and unknown-alias behavior.
13. Group and broadcast encoding, interaction with passive listeners, and proof that multicast, broadcast, and wildcard semantics cannot pass through `UpstreamHostAlias`.
14. Diagnostics counter schema, widths, saturation, reset, publication, and rate limiting.
15. Profile identifiers, configuration fingerprints, and how tools prove peer compatibility without dynamic negotiation.

No implementation should fill these gaps silently and call the result interoperable.

## 10. Prototype Validation Matrix

| Area | Required prototype cases | Pass condition |
|---|---|---|
| Capacity boundaries | `N=1,2,3,4,16`; payloads immediately below, at, and above each maximum | Minimal legal `N`; oversize rejected before START |
| Exact length | Every legal candidate final DLC; short and malformed DLCs | Exact reconstructed length or deterministic discard |
| Loss | Drop START, middle, and final frames | No partial delivery; timeout/counter is correct |
| Ordering | Duplicate, swap, skip, delay, and replay constituent frames | No false delivery; deterministic discard accounting |
| Generation | Rapid wrap, stale continuations, reset, and new START collision | No cross-generation assembly |
| Concurrency | Several CAN IDs interleaved; attempted same-ID interleaving | Bounded correct contexts; safe same-ID behavior |
| Integrity | Corrupt every protected field and payload position | CRC behavior matches golden vectors |
| Domains/IDs | Raw and protocol frames across reserved ID boundaries; duplicate-owner configuration | Correct dispatch; configuration collision rejected |
| Mapping | Valid/invalid VCN, endpoint-alias, and `UpstreamHostAlias` mappings; four-code and eight-code singular host maps; compact/literal experiments | Canonical metadata reconstructed only from a configured one-to-one singular mapping, otherwise PDU discarded |
| Mapping configuration | Matching, missing, stale, and incompatible alias-map digests; duplicate canonical targets; fail-closed startup and reconfiguration | Only the intended alias map becomes active; mismatch or ambiguity cannot enable traffic |
| Host markers/groups | External-CAN `UpstreamHost::kLocalHost`; special and unrepresentable hosts; attempted multicast, broadcast, or wildcard through `UpstreamHostAlias` | Every case rejected; no group delivery is inferred from a host alias |
| QoS/arbitration | Named-profile vectors for every supported symbolic canonical QoS value and CAN priority code; attempts to copy or numerically order canonical QoS | Code and arbitration order match vectors exactly; implicit mapping is rejected |
| Frame classes | Standard, extended, RTR, and CAN FD inputs | Profile policy applied consistently |
| Resource bounds | Full context pool, queue exhaustion, timeout storms, malformed-frame fuzzing | No allocation, overrun, livelock, or unbounded diagnostics |
| Topology | Passive same-bus listeners, same-VC transparent forwarding, and configured cross-VC replication | Transparent forwarding preserves canonical semantics; replication consumes/reoriginates; neither implies ECU unicast |
| Implementation | At least two independent encoders/decoders consume the same vectors | Byte-identical output and matching accept/reject results |

System validation shall also measure worst-case RAM, CPU time, queue occupancy, latency, bus load, and counter behavior under error storms.

## 11. Stabilization Exit Criteria

The adapter may advance from Experimental/WIP only when:

1. Every item in Section 9 is resolved or explicitly excluded by the named profile.
2. Frame classes, CAN-ID mapping, byte layouts, bit numbering, DLC rules, maximum `N`, timeout, and all state transitions are normative.
3. CRC algorithms and complete coverage are justified and published with check values and byte-exact vectors.
4. Valid and invalid conformance vectors cover all boundary and failure cases.
5. Static tooling detects duplicate CAN-ID ownership, domain overlap, illegal mappings, incompatible profiles, and impossible PDU/MTU configurations.
6. Fixed memory and timing bounds are demonstrated on representative small targets.
7. Two independent implementations interoperate under loss, corruption, wrap, restart, and malformed-input testing.
8. Diagnostics are bounded, locally useful, and incapable of recursive error traffic.
9. Cross-document terminology and routing semantics agree with the consolidated core and LLL contracts.
10. A security and fault-containment review documents the trusted-bus assumptions and denial-of-service limits.

Completion of Reliable Transport is not an exit criterion for the unreliable adapter. Any later reliable-capacity statement must use the separately specified `T_reliable` and its own conformance work.

## 12. Source Lineage

This specification consolidates the experimental material in:

- [`can_pdu_adapter_reorganized.md`](../drafts/network/can_pdu_adapter_reorganized.md);
- [`can_pdu_adapter.md`](../drafts/network/can_pdu_adapter.md);
- [`can_pdu_payload_capacity(1).md`](../drafts/network/can_pdu_payload_capacity(1).md);
- [`can_pdu_payload_capacity.md`](../drafts/network/can_pdu_payload_capacity.md);
- the Classical CAN sections of [`network_stack_design_updated.md`](../drafts/network/network_stack_design_updated.md);
- relevant updates in [`recent_protocol_design_updates.md`](../drafts/network/recent_protocol_design_updates.md) and [`network_architecture_refinements.md`](../drafts/network/network_architecture_refinements.md).

Those files remain historical source inputs. This document controls within its declared scope while preserving all unfinished wire decisions as unfinished.
