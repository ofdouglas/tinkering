# Embedded Message Fabric Architecture

**Status:** Consolidated document index and authority guide  
**Maturity:** Draft; suitable for prototype alignment, not a wire-interoperability claim  
**Scope:** Static, bounded embedded messaging for local/private deployments

This directory is being consolidated from exploratory drafts into a smaller document set. The architecture is a statically configured embedded message fabric, not a general internetwork. It supports bounded service messages across local IPC/ICC, MCU links, buses, and explicitly configured gateways.

## Reading order

1. [Architecture overview](architecture_overview.md) — scope, layers, supported communication domains, invariants, security posture, and maturity.
2. [Core protocol and routing](core_protocol_and_routing.md) — authoritative endpoint identity, service roles, addressing, Virtual Circuits (VCs), listeners, forwarding, and replication.
3. [Application protocols and services](application_protocols_and_services.md) — authoritative service archetypes, endpoint bindings, pub/sub behavior, schemas, sizing, and local endpoint behavior.
4. [Logical links and transports](logical_links_and_transports.md) — authoritative link contract, transport semantics, concurrency, and link integrations.
5. [Link Entity runtime and status](link_entity_runtime_and_status.md) — authoritative runtime-link ownership, lifecycle, recovery, supervision, and status responsibilities.
6. [HDLC logical-link profile](hdlc_logical_link_profile.md) — experimental serial/FIFO framing candidate and stabilization gaps.
7. [CAN PDU adapter specification](can_pdu_adapter_spec.md) — experimental Classical CAN profile and its open wire-format issues.
8. [Prototype and validation](prototype_and_validation.md) — implementation boundary, test matrix, conformance work, and promotion criteria.
9. [Rationale, use cases, and risks](rationale_use_cases_and_risks.md) — non-normative motivation, deployment fit, tradeoffs, alternatives, adoption strategy, and risks.

## Document status and ownership

| Document | Status | Owns |
|---|---|---|
| `README.md` | **Index / authority guide** | Reading order, document authority, maturity summary, and source traceability |
| `architecture_overview.md` | **Consolidated draft** | Architectural scope, boundaries, high-level model, and invariants |
| `core_protocol_and_routing.md` | **Provisional core specification** | Canonical protocol and routing definitions |
| `application_protocols_and_services.md` | **Consolidated provisional design contract** | Application/service contracts and archetypes |
| `logical_links_and_transports.md` | **Stable generic contracts; wire profiles unstabilized** | Logical-link and transport contracts |
| `link_entity_runtime_and_status.md` | **Provisional runtime contract** | Runtime-link ownership, lifecycle, recovery, supervision, and status |
| `hdlc_logical_link_profile.md` | **Experimental / not a stable wire profile** | Serial/FIFO framing candidate |
| `can_pdu_adapter_spec.md` | **Experimental / work in progress** | Classical CAN mapping and PDU adaptation |
| `prototype_and_validation.md` | **Actionable prototype plan; disposable validation implementation.** | Prototype scope, tests, vectors, and readiness gates |
| `rationale_use_cases_and_risks.md` | **Non-normative consolidated draft** | Motivation, examples, alternatives, adoption, and risk analysis |

## Current maturity

- **Architecture direction — consolidated draft.** The system is a static, bounded embedded message fabric. Service upstream/downstream roles are independent of physical topology.
- **Communication domains — supported direction.** `VirtualCircuit::kLocalHost` supports same-host IPC/ICC; `VirtualCircuit::kMainCompute` supports Main-to-Main communication; configured VCs support Main/ECU and multi-link deployments.
- **Routing scope — constrained.** A downstream target is the complete `VCN + Index`. `UpstreamHost` identifies a configured Main for external/Main relationships and reserves symbolic `UpstreamHost::kLocalHost` for local IPC/ICC. It is distinct from `VirtualCircuit::kLocalHost`; both numeric encodings remain provisional. Arbitrary ECU-to-ECU unicast, especially across buses, is not supported.
- **Publication — supported direction.** A configured listener on an attached link may consume a visible publication; listening itself creates no forwarding or fan-out. Transparent forwarding may make a publication visible on another attached link within the same VC. Explicit configured cross-VC state replication remains separate.
- **Unreliable Datagram — completed current transport definition.** Exact interoperable wire profiles still depend on their owning link specifications and conformance vectors.
- **Sequenced/E2E-Protected Datagram — Provisional.** Required semantics and exact profile remain to be completed.
- **Reliable delivery — wholly TBD.** No reliable wire format, state machine, or interoperability is claimed.
- **CAN and HDLC — experimental.** Encodings, integrity details, error behavior, and conformance vectors are not yet stable interoperability contracts.
- **Security — initial local/private posture.** The initial prototype excludes shell, memory/MMIO access, update, fault control, replay, and raw injection. Experimental versions require a separate development build and cannot be enabled solely by runtime network configuration. IP composition is possible, but a secure remote gateway is not an initial feature.

## Terminology

Use [core protocol and routing](core_protocol_and_routing.md) for normative definitions of `VCN`, `Index`, `UpstreamHost`, service direction, endpoint identity, listener, forwarder, and replicator. Use [application protocols and services](application_protocols_and_services.md) for service and pub/sub terminology, [logical links and transports](logical_links_and_transports.md) for transport and link terminology, and [Link Entity runtime and status](link_entity_runtime_and_status.md) for runtime ownership, recovery, and status.

This index and the [overview](architecture_overview.md) use those terms only to explain scope. They do not define numeric values, bit layouts, serialization, or wire compatibility.

## Known gaps

- Final canonical numeric encodings and immutable named wire profiles.
- Golden/conformance vectors and independent interoperability testing.
- Complete Sequenced/E2E-Protected Datagram metadata, validation, reset, freshness, and failure semantics.
- Complete Reliable transport peer model, state machine, flow control, retransmission, reset, and resource bounds.
- Stable CAN and HDLC framing, integrity coverage, malformed-input behavior, and profile selection.
- Generated VC/route configuration, loop and reachability validation, capacity analysis, and deployed-configuration fingerprints.
- Final endpoint/router/link APIs, bounded backpressure behavior, and local IPC memory/coherency requirements.
- A reviewed security architecture for authentication, authorization, replay protection, key lifecycle, audit, and remote gateways.
- Production criteria for safety-relevant, vehicle, industrial, or space deployments.

Open gaps remain explicit. A draft's proposed number, layout, or algorithm does not become authoritative merely because the corresponding consolidated specification is incomplete.

## Authority: consolidated set versus source drafts

The consolidated document set is the current authority within each document's declared scope. Topic-specific normative documents own detailed definitions; this README owns document authority; the overview owns only architecture-level boundaries and invariants; the rationale document is non-normative.

When a consolidated document conflicts with a source draft, the consolidated document controls. When a delegated consolidated document marks an issue unresolved, the issue remains unresolved; an older draft does not regain normative authority. The controlling decisions recorded in the consolidation effort override contrary draft material.

All original Markdown drafts remain unchanged as historical inputs for provenance, design intent, and recovery of omitted context. They are not deleted, rewritten, or silently promoted into the current specification.

Consolidated specifications remain in this directory; historical inputs are stored in `../drafts/network/`.

## Source traceability

Classification means:

- **Baseline** — starting point for consolidation, subject to later controlling decisions.
- **Specialized input** — topic-focused material retained, reconciled, or explicitly deferred in an owning consolidated document.
- **Superseded input** — an older lineage consulted for provenance but not current authority.
- **Historical input** — early intent or context retained without normative force.

| Source draft | Classification | Primary consolidation use |
|---|---|---|
| [`network_stack_design_updated.md`](../drafts/network/network_stack_design_updated.md) | **Baseline** | Whole architecture; reconciled into all owning documents |
| [`network_stack.md`](../drafts/network/network_stack.md) | **Superseded input** | Early stack model and terminology history |
| [`network_stack_ecosystem_updated.md`](../drafts/network/network_stack_ecosystem_updated.md) | **Superseded input** | Ecosystem, layering, and service portability lineage |
| [`recent_protocol_design_updates.md`](../drafts/network/recent_protocol_design_updates.md) | **Specialized input** | Service roles, congestion, scheduling, diagnostics, and bounded behavior |
| [`network_architecture_refinements.md`](../drafts/network/network_architecture_refinements.md) | **Specialized input** | VC topology, local/Main domains, validation, security, and MVP direction |
| [`static_virtual_circuit_routing.md`](../drafts/network/static_virtual_circuit_routing.md) | **Specialized input** | Static VC concepts and configured forwarding |
| [`link_entity_recovery_and_status.md`](../drafts/network/link_entity_recovery_and_status.md) | **Specialized input** | Link/entity lifecycle, recovery, and status |
| [`endpoint_identifiers_revised.md`](../drafts/network/endpoint_identifiers_revised.md) | **Specialized input** | Endpoint-identity lineage; reconciled with later controlling decisions |
| [`endpoint_identifiers.md`](../drafts/network/endpoint_identifiers.md) | **Superseded input** | Earlier endpoint-identity model |
| [`application_protocol_archetypes_updated.md`](../drafts/network/application_protocol_archetypes_updated.md) | **Specialized input** | Prototype service archetypes and IPC/ICC cases |
| [`application_protocol_archetypes.md`](../drafts/network/application_protocol_archetypes.md) | **Superseded input** | Earlier archetype lineage |
| [`service_catalog_and_schema_conventions.md`](../drafts/network/service_catalog_and_schema_conventions.md) | **Specialized input** | Service catalog, schema, compatibility, and naming guidance |
| [`service_size.md`](../drafts/network/service_size.md) | **Specialized input** | Message-size and constrained-link analysis |
| [`flexible_logical_link_layer_design.md`](../drafts/network/flexible_logical_link_layer_design.md) | **Specialized input** | Generic logical-link responsibilities and APIs |
| [`transport.md`](../drafts/network/transport.md) | **Specialized input** | Transport design history and unresolved reliability work |
| [`pooled_linked_spsc_design.md`](../drafts/network/pooled_linked_spsc_design.md) | **Specialized input** | Bounded local queue/concurrency mechanism |
| [`can_pdu_adapter_reorganized.md`](../drafts/network/can_pdu_adapter_reorganized.md) | **Specialized input** | Latest experimental CAN PDU adapter lineage |
| [`can_pdu_adapter.md`](../drafts/network/can_pdu_adapter.md) | **Superseded input** | Earlier CAN adapter design |
| [`can_pdu_payload_capacity.md`](../drafts/network/can_pdu_payload_capacity.md) | **Specialized input** | Canonical specialized CAN capacity analysis |
| [`can_pdu_payload_capacity(1).md`](../drafts/network/can_pdu_payload_capacity(1).md) | **Superseded input** | Exact duplicate of the canonical CAN capacity input |
| [`hdlc_service_protocol_design_rev3.md`](../drafts/network/hdlc_service_protocol_design_rev3.md) | **Specialized input** | Latest experimental HDLC design lineage |
| [`hdlc_service_protocol_design.md`](../drafts/network/hdlc_service_protocol_design.md) | **Superseded input** | Earlier HDLC design |
| [`distributed_embedded_system_examples.md`](../drafts/network/distributed_embedded_system_examples.md) | **Specialized input** | Board, HIL, robot, vehicle, and satellite examples |
| [`project_adoption_and_security_risks.md`](../drafts/network/project_adoption_and_security_risks.md) | **Specialized input** | Adoption strategy and security guardrails |
| [`RFP.md`](../drafts/network/RFP.md) | **Specialized input** | Reliable Transport design requirements and history |

These 25 drafts form the traceability baseline. Their retention does not imply equal maturity or authority.
