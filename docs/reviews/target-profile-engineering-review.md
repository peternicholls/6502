# Target-profile engineering review

**Status:** Non-authoritative follow-up considerations
**Reviewed:** 2026-07-21
**Source feature:** `specs/completed/004-machine-target-profile/`

This review records engineering observations made after the target-profile
feature passed acceptance. It does not reopen feature 004, select future work or
add scope to the programme. Any change described here must first belong to a
named row or gate in the
[Machine delivery plan](../product/MACHINE_DELIVERY_PLAN.md) and then be judged
inside the relevant new specification.

The immediate next specification may assess the firmware-related observation
below. Other observations remain dormant until their owning delivery slice is
selected; proximity in this document is not priority.

## What feature 004 established well

### Immutable, extensible identity values

`ProfileComponentIdentity` and `MachineTargetProfile` are complete owned values
with no mutators. Raw identifiers and versions remain representable, so later or
invalid values can cross boundaries without being normalized into a known
profile. The fixed expansion envelope provides ABI headroom for up to sixteen
entries; more than sixteen still requires a new schema and an explicit
compatibility decision.

### Explicit validation order

`validateMachineTargetProfile` is a linear sequence of early rejections. Its
shape makes two safety properties visible:

1. a declared expansion count above sixteen rejects before expansion storage is
   obtained or indexed; and
2. multi-defect values retain the precedence `malformed`, `unknown`,
   `incompatible`, then `recognised unavailable`.

That directness is more important than reducing the function's line count.

### Deliberate boundary translation

The C adapter copies profile fields explicitly in both directions. Although the
mapping is repetitive, it avoids relying on C/C++ object layout, padding or
aliasing and preserves every raw field, including unused slots. The translation
is therefore contract code, not incidental boilerplate.

### Correct ownership boundaries

`ActiveCall` uses RAII to admit a C call, retain the handle state and release the
admission exactly once. The registry and handle-state mutexes protect token
lifetime and destruction ordering; `operation(...)` contains exceptions from
adapter callables.

Machine mutation and observation are serialized elsewhere: `MachineRuntime`
submits commands to its bounded FIFO and only its owner thread dereferences
`BBCMicro`. Future maintenance must keep those responsibilities distinct.

### Honest support and diagnostics

Model B+ 64K crosses C++, C, Swift and the application as a distinct identity,
but construction remains explicitly unavailable. Swift supplies the base
profile error and the application separately explains that the active Model B
session was retained. Neither layer falls back or presents identity transport
as B+ machine behavior.

Public C, C++ and Swift declarations document their ownership, support and
failure contracts, while
[Machine Target Profiles](../code/target-profile.md) owns the cross-language
rationale.

## Considerations for future specifications

### 1. Keep validation phases legible as rules grow

No refactor is justified by feature 004 alone. If later profile work adds enough
rules to obscure the existing order, a future specification may evaluate
separate structural and support-classification helpers.

Any such change must retain the count-before-storage rule, the exact rejection
precedence, raw-value diagnostics, the prohibition on normalization and the existing
multi-defect regression matrix. A shorter function is not an improvement if
those properties become harder to audit.

### 2. Reassess explicit translation only when the schema changes

The current field-wise C/C++ mapping is intentionally repetitive and should
remain so while schema version 1 is stable. If another schema or assigned
expansions materially increases the mapping surface, shared component-level
helpers may be considered, provided they still:

- copy fields rather than object representations;
- preserve all fixed slots and the declared count independently;
- keep C outputs untouched on failure; and
- retain compile-time agreement for public constants and capacities.

Do not replace the mapping with `memcpy`, layout assertions or reinterpretation
between the public C aggregate and private C++ classes.

### 3. Make firmware requirements profile-aware when firmware work owns them

`BeebMachine.loadOSROM` currently requires 16 KiB and reports a Model B-specific
error. That is accurate because Model B is the only constructible profile. It is
not target-profile debt and should not be generalized pre-emptively.

The `machine-firmware-onboarding` specification should decide how firmware type,
size, assignment and diagnostics relate to the selected profile. Later B+
implementation must make its own reference-backed decision. The specification
should also decide whether Swift preflight validation remains useful or whether
all profile-dependent validation should come from the runtime; moving wording
into C++ is not automatically the cleaner boundary.

### 4. Organize the Swift surface without fragmenting ownership

`BeebMachine` is intentionally the Swift owner of one opaque C token and adds no
second lock around `MachineRuntime`. Its public surface will grow as snapshots,
media and inspection arrive, so source organization should be reassessed when a
relevant feature makes the problem concrete.

File-level extensions or owner-retaining views are safer first options than
multiple facades independently holding the raw handle. Any later design must
preserve one destruction lifetime, synchronous typed status mapping and the
rule that no UI object borrows or mutates live core state directly.

### 5. Treat diagnostic capacity as an explicit ABI constraint

The C ABI owns a fixed 256-byte status-message buffer and documents that a
diagnostic may be truncated. Current profile messages are concise and retain a
stable machine-readable support category, so this is not a feature-004 defect.

Future specifications that need richer conflict information should prefer
structured fields or a separately versioned retrieval contract over composing
an unbounded list into the string. Core diagnostics need not be globally capped
for the sake of one adapter, but every C-facing message must remain useful when
copied into the documented capacity.

## Revisit map

| Consideration | Earliest relevant programme slice | Required evidence before change |
| --- | --- | --- |
| Profile-aware firmware requirements | `machine-firmware-onboarding` | Reference-backed size/type rules, typed rejection and observed onboarding recovery |
| Validation phases and translation helpers | A feature that assigns expansions or a new profile schema | Existing precedence/boundary matrix plus new schema compatibility fixtures |
| Swift API organization | Snapshot, media or C6 host work that materially expands the wrapper | One-handle lifetime, concurrency and failure-mapping regression evidence |
| Richer C diagnostics | A feature requiring information that cannot remain actionable within 255 bytes | Versioned public contract and cross-language ownership tests |
| Persisted profile identity | `snapshot-format-v1` | Semantic encoding, bounds, compatibility and failure-atomic restore evidence |

These are review prompts, not acceptance criteria. Each owning specification may
retain the present design when evidence shows that change would add more
complexity or risk than value.
