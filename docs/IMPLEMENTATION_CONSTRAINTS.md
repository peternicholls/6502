# Implementation constraints

**Status:** Supporting technical requirements for unfinished work
**Updated:** 2026-07-31

This file constrains technical design after a slice is selected from the
[Machine delivery plan](product/MACHINE_DELIVERY_PLAN.md). It is not a roadmap,
backlog or completion ledger. [STATUS.md](STATUS.md) records what exists.

## Global constraints

- Keep the C++20 core deterministic, dependency-light and host-agnostic.
- Preserve the `MachineRuntime` owner and quiescent completed-instruction/device
  boundary unless a feature explicitly versions a replacement.
- Bound every queue, payload and untrusted input before mutation.
- Reject unknown profiles, expansions, persisted formats and media without
  partially changing the active machine.
- Keep C++ exceptions inside C++; C and Swift receive owned values and typed
  failures.
- Define fixtures, observation intervals and tolerances for every fidelity,
  timing, latency or throughput claim.
- The first Model B host workflow remains bounded to typed firmware roles,
  fixed language bank 12, physical-key matrix input, owned completed-frame
  presentation and independent lifecycle controls; do not add text injection,
  host-side emulation time or speculative persistence to close this slice.

## Delivery economy

- Prefer the smallest vertical feature that proves a user or machine outcome.
  Do not create separate features solely for core, C, Swift and host portions of
  one inseparable journey.
- Keep tasks narrow inside a vertical feature. A feature may cross layers
  without becoming permission to absorb a second outcome or speculative
  abstraction.
- Reuse the established owner, boundary adapters, queues, fixtures and host
  patterns. Add a new layer only when the selected outcome cannot remain clear
  or safe without it.
- Verification is proportional: focused tests while implementing, affected
  wider regressions at slice closure and the full maintained matrix at milestone
  or release closure. Safety, persistence, concurrency and public-boundary
  changes still receive the regression depth their failure risk requires.

## Profile-consuming work

The completed profile contract provides a stable versioned base plus a bounded
ordered expansion list across C++, C and Swift. Downstream work must:

- preserve the permanent Model B and Model B+ 64K identities and their raw
  round-trip values;
- treat Model B as supported and Model B+ 64K as recognised-unavailable until a
  separately accepted implementation supplies its machine behavior;
- leave B+ 128K, Master-family, Tube, Econet and storage/peripheral identifiers
  unassigned until their own specifications allocate them;
- persist the complete identity in snapshots and host configuration rather than
  infer it from firmware or media; and
- retain bounded validation precedence and reject unsupported identities
  without fallback or partial mutation.

## C3 — session continuity

Snapshots use a bounded, versioned envelope. They record profile/expansion
identity, CPU, memory, ROM selection, devices and mounted-media identity at the
quiescent safe point. Save/load operations cross C and Swift with owned data.
Restore validates the complete envelope before committing state; corruption,
oversize data or incompatible identity leaves the active session unchanged.
Stale frame/audio output cannot survive restore.

## C4 — bus-cycle timing

Cycle refinement must express CPU operations as ordered bus phases, advance
devices at the correct phase and sample interrupt/wait inputs at documented
boundaries. Existing instruction tests remain the semantic layer; new bus-trace
fixtures prove timing. The owner may not expose half-completed bus operations.
C4 must preserve version-1 snapshot meaning or provide an explicit migration.

## Model B+ 64K

Research precedes implementation. A dedicated reference/fixture slice must fix
the processor, memory, display, firmware and disc-controller claims using
primary sources and lawful fixtures. The implementation remains a separate
profile, not Model B with inferred patches. Model B regression evidence stays
mandatory.

## C5 — media

Disc and tape/file work remain separate specifications. Imported user media is
copied into private machine state. Protection, controller/profile compatibility
and failure diagnostics are explicit. Modified content leaves only through an
explicit export; source files are never silently mutated.

## C6 — inspection and editor bridge

Read-only inspection uses stable owner-produced observations. Breakpoints and
watchpoints are bounded commands. Memory edits are validated atomic
transactions. BASIC tokenization, injection and retrieval operate at explicit
program boundaries and report conflicts rather than discarding either side.
Product inspection and bridge-validation specifications must demonstrate these
contracts before editor transformation work expands them.

## Specification rule

Each feature row in the delivery plan gets a stable feature identity, its own
Spec Kit artifacts and independently passing evidence. Milestone gates are
closed by integrated evidence from the final contributing feature and do not
need empty validation features. A vertical feature may implement a single
outcome across core, C, Swift and host; unrelated outcomes and speculative
future support remain separate.
