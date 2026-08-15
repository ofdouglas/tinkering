# HDLC Logical Link Profile

> **EXPERIMENTAL — NOT A STABLE WIRE PROFILE**
>
> This document records useful framing candidates for prototyping. It does not define `HDLC-v1`, does not claim interoperability, and shall not be used as a released compatibility promise. Every candidate below remains subject to incompatible change until the stabilization criteria are met.

## 1. Scope

This candidate maps the canonical bounded-PDU service from [Logical Links and Transports](logical_links_and_transports.md) onto an ordered byte-stream Hardware Link such as a serial stream or FIFO. The byte stream may still suffer corruption, loss, overrun, or restart; this profile does not assume reliable delivery.

It provides candidate:

- frame delimiting and byte escaping;
- link-level error detection;
- bounded receive parsing and resynchronization;
- carriage of one serialized canonical PDU per frame.

It does not define application services, endpoint allocation, transport reliability, medium access, serial-port parameters, or Hardware Link electrical behavior. Canonical descriptor and routing semantics belong to [Core Protocol and Routing](core_protocol_and_routing.md). Runtime ownership and restart behavior belong to [Link Entity Runtime and Status](link_entity_runtime_and_status.md).

## 2. Candidate decoded-frame model

After delimiter removal and unescaping, the candidate decoded frame is:

```text
+----------------------+----------------------------------+
| Leading link CRC-16  | Serialized canonical PDU         |
| 2 bytes              | bounded, exact layout unresolved |
+----------------------+----------------------------------+
                       <------ CRC coverage --------------->
```

Candidate properties:

- One frame carries exactly one complete canonical PDU.
- The CRC field is first.
- The CRC field is serialized least-significant byte first.
- All other multi-byte fields introduced by this profile would use little-endian serialization.
- The candidate CRC covers every decoded byte after the two CRC bytes and no delimiter or inserted escape byte.
- The receiver validates length and CRC before parsing any canonical PDU field.

The exact serialized canonical PDU layout is unresolved. Consequently, the minimum legal frame length, field offsets after the CRC, reserved-bit rules, and final profile vectors are also unresolved.

## 3. Candidate framing and escaping

### 3.1 Delimiters

`0x7E` is the candidate frame delimiter.

A transmitter emits a delimiter before and after the escaped frame body:

```text
0x7E | escaped(decoded frame) | 0x7E
```

A receiver uses delimiters as resynchronization points. The likely parser model treats a delimiter as both the end of the preceding candidate frame and synchronization for the next frame, permitting adjacent frames to share a delimiter. This behavior must be frozen and covered by vectors before stabilization.

### 3.2 Escaping

Within the decoded frame, each occurrence of either:

- `0x7E`, or
- `0x7D`

is replaced on the byte stream by:

```text
0x7D, original_byte XOR 0x20
```

Thus the canonical substitutions are:

```text
0x7E -> 0x7D 0x5E
0x7D -> 0x7D 0x5D
```

The receiver reverses the XOR transformation before length accounting and CRC calculation. Delimiters and inserted escape bytes are not part of the decoded frame and are not covered by the CRC.

Whether a receiver must reject non-canonical escape pairs such as `0x7D 0x00`, rather than decoding them by the generic XOR rule, is unresolved. A stable profile must choose one behavior.

## 4. Candidate bounds

The decoded-frame limit candidate is:

```text
MAX_DECODED_FRAME_SIZE = 4095 bytes, including the 2-byte CRC
```

This retains the earlier design candidate that limits the CRC-covered body to at most 4093 bytes. The claimed error-detection rationale for that limit must be independently verified for the final CRC profile and protected message set before stabilization.

Derived candidate bounds:

- CRC-covered canonical PDU body: at most 4093 bytes.
- Standalone frame in the worst escaping case: at most `2 + 2 * 4095 = 8192` stream bytes, including two delimiters.
- A streaming receiver need not buffer the escaped representation.
- The minimum legal decoded-frame size is TBD because the canonical PDU serialization is not frozen. It must be greater than two bytes.

An implementation may support a smaller local PDU limit for constrained targets only if deployment configuration proves peer/service compatibility. It shall still reject oversize input safely and expose its actual bound as a capability.

The stable profile must decide whether 4095 is a universal required receive limit, only a protocol maximum, or a profile-family maximum with named smaller-capacity variants.

## 5. Candidate CRC-16

The retained algorithm candidate is **CRC-16/CCITT-FALSE**:

```text
width   = 16
poly    = 0x1021
init    = 0xFFFF
refin   = false
refout  = false
xorout  = 0x0000
check("123456789") = 0x29B1
```

Candidate frame processing:

1. Unescape bytes into the decoded frame while enforcing the decoded-size bound.
2. Require at least the final minimum decoded-frame length.
3. Read the leading CRC as a little-endian 16-bit value.
4. Compute CRC-16/CCITT-FALSE over `decoded_frame[2..end]`.
5. Compare the computed and transmitted values.
6. On mismatch, discard the frame and record a local integrity failure.
7. Only after a match, parse and validate the serialized canonical PDU.

These algorithm-level checks are useful during development but are not sufficient profile conformance vectors:

```text
Protected bytes: <empty>
Computed CRC:    0xFFFF
Wire CRC bytes: FF FF

Protected bytes: 31 32 33 34 35 36 37 38 39  ("123456789")
Computed CRC:    0x29B1
Wire CRC bytes: B1 29
```

An empty protected body is not asserted to be a valid protocol frame; it is only an algorithm test.

Before stabilization, exact vectors shall include complete valid canonical PDUs, escaped CRC and payload bytes, minimum and maximum legal lengths, corrupted fields, and independently calculated expected outcomes.

## 6. No initial flow control

The initial experimental frame contains no flow-control field, credit nibble, receive-window advertisement, or link-control byte.

Local bounded TX rejection and backpressure follow [Logical Links and Transports](logical_links_and_transports.md). They do not provide peer-to-peer flow control. A service that can overrun a peer must currently constrain traffic through static provisioning or its own explicitly defined protocol behavior.

Any future peer flow control requires a separately specified extension or new named profile. It shall not be inserted into reserved bits or inferred from implementation behavior.

## 7. Candidate parser state

A bounded streaming parser likely needs only:

- synchronized/awaiting-frame state;
- accumulating-frame state;
- escape-pending state;
- discard-until-delimiter state after oversize or Hardware Link loss;
- decoded byte count;
- incremental CRC state or bounded frame storage;
- bounded diagnostics.

The parser shall make progress for every input byte and shall not allocate memory based on untrusted lengths.

CRC validation must precede canonical PDU parsing even when an implementation computes the CRC incrementally. If zero-copy receive storage is used, no unvalidated field may select a destination, extension length, endpoint, transport parser, or application handler before CRC success.

## 8. Parser edge cases requiring explicit behavior

The following cases shall have frozen behavior and conformance coverage before the profile can stabilize. Candidate handling is listed where the drafts provide a safe direction.

### 8.1 Synchronization and delimiters

- **Bytes before the first delimiter:** discard as unsynchronized input; optionally count one synchronization-loss event rather than one event per byte.
- **Repeated delimiters:** remain synchronized; do not deliver zero-length frames.
- **Opening delimiter immediately followed by a closing delimiter:** treat as idle/empty framing, not as a PDU.
- **A delimiter after accumulated bytes:** close the candidate frame and begin validation.
- **One delimiter between back-to-back frames:** likely closes one frame and synchronizes the next; this must be made normative.
- **Stream starts or restarts in the middle of a frame:** discard until the next delimiter.

### 8.2 Escapes

- **Escape split across Hardware Link read chunks:** preserve escape-pending state across calls.
- **`0x7D 0x5E` and `0x7D 0x5D`:** decode to `0x7E` and `0x7D`.
- **Escape immediately followed by delimiter:** discard the incomplete frame, count a malformed/dangling-escape event, and use the delimiter to resynchronize.
- **Escape at stream shutdown or driver reset:** discard transient frame state.
- **Non-canonical escaped value:** strict rejection versus generic XOR decoding is unresolved.
- **Unescaped `0x7D`:** it always begins an escape and cannot be emitted as decoded data by itself.

### 8.3 Length and storage

- **Fewer than the final minimum decoded bytes:** discard as undersize without parsing.
- **Exactly the configured maximum:** accept for CRC/PDU validation.
- **One byte beyond the configured maximum:** enter discard-until-delimiter state immediately; do not continue writing storage.
- **Long oversize run with escape bytes:** remain bounded and discard until an actual unescaped delimiter.
- **Worst-case escaping:** enforce decoded length, not the count of stream bytes.
- **Local implementation bound below the protocol candidate maximum:** reject above the advertised/configured local bound; deployment compatibility remains required.

### 8.4 Integrity and PDU validation

- **CRC mismatch:** discard locally before PDU parsing; do not generate a per-frame peer error.
- **Valid CRC but malformed canonical PDU:** record a PDU-parse diagnostic and discard or report through the bounded local diagnostics policy.
- **Valid CRC and unsupported canonical feature/profile:** follow the core protocol's bounded unsupported-feature behavior; do not let diagnostics recurse.
- **CRC bytes themselves require escaping:** escape them exactly like every other decoded byte.
- **CRC field byte order:** read and write little-endian.

### 8.5 Hardware Link failures

- **RX overrun, dropped DMA region, or explicit byte-loss indication:** abandon the current frame and discard until a delimiter.
- **Driver stop/restart:** clear parser transient state.
- **Read chunk contains several complete frames:** deliver each complete valid PDU in order until an upward queue becomes full; the policy for remaining bytes must be explicit and bounded.
- **Upward receive port is full after a valid frame:** drop, pause Hardware Link draining, or retain one bounded completed frame according to configured policy; count the event. Never overwrite unrelated storage silently.

## 9. TX behavior

TX submission follows the stable LLL acceptance and ownership contract.

For an accepted PDU, the experimental encoder:

1. validates the configured decoded-frame bound;
2. serializes the canonical PDU using the still-unresolved canonical layout;
3. computes the candidate CRC over that serialized PDU;
4. emits the little-endian CRC first;
5. escapes the CRC and PDU bytes;
6. emits delimiters according to the final framing rule.

The encoder shall either reserve enough bounded capacity before acceptance or retain sufficient owned state to resume partial Hardware Link writes. It shall not report acceptance and then depend on caller-owned bytes after their documented lifetime.

Partial byte-stream progress is local implementation state. It does not split the canonical PDU at the LLL service boundary.

## 10. Bounded diagnostics

At minimum, an implementation should maintain bounded counters or latched observations for:

- frames accepted;
- unsynchronized input/synchronization loss;
- empty delimiter runs, if diagnostically useful;
- dangling or invalid escape;
- undersize frame;
- oversize frame;
- CRC mismatch;
- canonical PDU parse/validation failure after valid CRC;
- RX storage or delivery-port exhaustion;
- Hardware Link overrun/loss indication;
- TX rejection and TX cancellation on restart;
- parser/driver restart count.

Diagnostics shall not allocate per-error records without a fixed bound, emit one log for every bad byte indefinitely, or generate recursive network errors. Repeated failures should be counted, aggregated, and rate-limited for remote publication. Counter width and saturation/wrap behavior remain to be specified.

## 11. Explicit unresolved wire and API issues

This experimental profile is blocked on:

1. Exact canonical PDU serialization, including descriptor layout, reserved bits, transport placement, and PDU minimum length.
2. Final profile name and version; `HDLC-v1` is intentionally not assigned.
3. Confirmation or revision of the 4095-byte decoded limit and minimum required implementation capacity.
4. Independent validation of CRC-16/CCITT-FALSE suitability and the length/error-detection claim.
5. A complete set of exact full-frame CRC, escaping, boundary, and invalid-input vectors.
6. Strict versus permissive handling of non-canonical escape pairs.
7. Normative repeated/shared-delimiter behavior.
8. Exact behavior when upward delivery blocks while more stream bytes are already available.
9. Hardware Link error signaling required to detect byte loss and force resynchronization.
10. TX completion semantics and timestamp meaning for byte-stream drivers.
11. Diagnostic counter widths, saturation, reset, and publication schema.
12. Whether named smaller-capacity variants are interoperable profile variants or deployment constraints.
13. Serial parameters and any multidrop/half-duplex medium-access profile, if required; they are not part of this base candidate.

## 12. Conformance and stabilization criteria

This document may be promoted only after:

- the canonical PDU layout it carries is stable;
- a named, immutable profile and version are assigned;
- every wire field, byte order, bound, reserved value, and malformed-input behavior is normative;
- exact encoder/decoder vectors cover ordinary, escaped, boundary, and invalid frames;
- CRC vectors are independently reproduced by at least two implementations;
- streaming parsers pass arbitrary input chunking, fuzzing, and long-noise/oversize tests with bounded memory and time;
- C++, host-tool, and at least one embedded implementation interoperate using the same vectors;
- restart, byte-loss, RX overrun, delivery congestion, and partial-TX tests pass;
- capability/configuration checks reject incompatible local bounds;
- diagnostics are bounded and non-recursive;
- an implementer can build a parser and encoder without making an interoperability decision not stated in the profile.

Until every criterion is satisfied and reviewed, this remains an experimental prototype profile. Broader integration and promotion evidence belongs in [Prototype and Validation](prototype_and_validation.md).
