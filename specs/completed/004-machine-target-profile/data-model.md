# Data Model: Machine Target Profile

## ProfileComponentIdentity

Represents one stable base-machine or expansion component without treating the
known values as a closed set.

| Field | Rule |
| --- | --- |
| identifier | Non-zero unsigned 32-bit stable code; raw unknown values remain representable for rejection |
| version | Positive unsigned 16-bit contract version for that component |
| reserved | Zero on input and output in schema version 1; non-zero rejects as malformed |

Equality compares all fields. Human-readable names are derived only for known
constants and are not part of stable identity.

## MachineTargetProfile

Complete immutable identity supplied when a machine is created.

| Field | Rule |
| --- | --- |
| schema version | Exactly 1 for this contract |
| base | Exactly one `ProfileComponentIdentity` |
| expansion count | 0...16 inclusive; 16 is structurally valid and any value above 16 is malformed before expansion storage is inspected |
| expansions | First `expansion count` entries are canonical ascending identifier/version order |
| unused expansion slots | Zero-filled and ignored on output; non-zero input rejects to preserve one canonical representation |

The two committed constructors produce `BBC Microcomputer Model B` and `BBC
Model B+ 64K`, both component version 1 with no expansions. B+ 128K,
Master-family, Tube, Econet, ADFS, storage and peripheral options are not
assigned public constants in this slice. The envelope can carry their later
assigned identifiers without changing shape, but a raw test fixture labelled
with one of those names remains unknown and non-canonical today.

| Base identity | Identifier | Component version | Support in this slice |
| --- | --- | --- | --- |
| BBC Microcomputer Model B | `0x00000001` | 1 | supported |
| BBC Model B+ 64K | `0x00000002` | 1 | recognised unavailable |

These code/version pairs are permanent. They cannot be reused or reinterpreted.
No expansion identifier is assigned by this feature.

## ProfileSupport

Pure validation result, separate from identity.

| State | Meaning | Construction result in this slice |
| --- | --- | --- |
| supported | Canonical Model B with no expansions | May create a runtime |
| recognisedUnavailable | Canonical Model B+ 64K only | Typed unsupported result; no machine created |
| unknown | Identifier or version has no recognised contract | Typed unknown-profile result; no machine created |
| incompatible | Known components cannot be combined | Typed incompatible-profile result; no machine created |
| malformed | Schema, count, ordering, duplicates, reserved fields or unused slots violate the envelope | Typed invalid-profile result; no machine created |

Each non-supported result contains an operation-owned diagnostic suitable for
developer/user recovery. Support status is not persisted as identity because it
may improve in later releases.

Validation classifies the complete input in this order:

1. **malformed** — invalid schema-zero fields, count above 16, non-zero reserved
   or unused slots, or non-canonical ordering/duplicates;
2. **unknown** — structurally valid but unassigned identifier or unsupported
   schema/component version;
3. **incompatible** — every component is known but is used in the wrong role or
   combination, including a base-machine identifier in an expansion slot;
4. **recognisedUnavailable** — canonical Model B+ 64K;
5. **supported** — canonical Model B.

The first category in that precedence wins for a multi-defect input at every
language boundary. A structurally valid profile with exactly 16 unassigned,
ordered expansion entries reaches `unknown`; a declared count of 17 reaches
`malformed` without reading past the fixed aggregate.
Supporting more than sixteen expansions requires a later schema version and an
explicit compatibility decision; version 1 never truncates excess entries.

## ActiveMachineProfile

The immutable `MachineTargetProfile` owned by one successfully created
`BBCMicro` and its `MachineRuntime`. It is observed through a serialized runtime
query and copied through C and Swift. It never aliases caller storage and cannot
change during that runtime's lifetime.

## HostProfileSelection

Application presentation state with deliberately separate fields:

| Field | Rule |
| --- | --- |
| requested profile | Current picker choice: Model B or Model B+ 64K |
| active profile | Profile queried from the live runtime, or absent when no runtime exists |
| support/rejection | Latest typed result and actionable description |
| running machine | Replaced only after requested-profile construction succeeds |

The selected control may display B+ 64K while active remains Model B after
rejection, but the two labels and status must make that distinction explicit.

## Validation and state transitions

```text
Raw value
  -> validate envelope
       -> malformed: reject, preserve caller output and active machine
       -> canonical value
            -> unknown/incompatible: reject, preserve active machine
            -> recognised unavailable: reject explicitly, preserve active machine
            -> supported Model B: construct paused runtime with immutable profile

Host request
  -> validate and construct candidate
       -> failure: retain prior runtime and active-profile observation
       -> success: install candidate, then release prior runtime
```

Repeated canonical Model B construction produces equal identity values. No
transition mutates an existing runtime profile. No validation, construction or
query advances emulated time.

The C aggregate is an in-process semantic carrier, not a persisted byte layout.
Padding, byte order, truncation and trailing serialized bytes are deliberately
undefined here and belong to the later snapshot-envelope specification.
