# Implementation constraints

**Status:** Supporting technical requirements for unfinished work
**Updated:** 2026-07-19

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

## Target profile

The profile contract must:

- represent a stable base machine plus explicit versioned expansions;
- commit Model B and Model B+ 64K identities;
- reserve later B+ 128K, Master-family, Tube, Econet and storage/peripheral
  identifiers without claiming support;
- flow through core, C, Swift, snapshots and host configuration; and
- reject unsupported identifiers without falling back to another machine.

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

Sequence labels in the delivery plan are planning containers. Each child gets a
stable feature identity, its own Spec Kit artifacts and independently passing
evidence. A container is never implemented as one umbrella feature.
