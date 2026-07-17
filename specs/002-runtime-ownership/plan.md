# Implementation Plan: Phase C1 Runtime Ownership

**Branch**: `002-runtime-ownership` | **Date**: 2026-07-15 | **Spec**: [spec.md](spec.md)

**Input**: Feature specification from `/specs/002-runtime-ownership/spec.md`

## Summary

Place one C++20 `MachineRuntime` between hosts and `BBCMicro`. Its owner thread
is the only code allowed to touch the contained machine. A bounded FIFO accepts
synchronous commands; while running, the owner interleaves fixed emulated-cycle
execution slices with queued commands and records both in an ordered ledger.
Every slice and command ends at the existing completed-instruction/device-tick
boundary. Replace the public 0.1 C operation shapes with a 0.2 structured-status
contract and map those results into typed Swift errors. This is an intentional
pre-1.0 boundary revision, documented and versioned rather than hidden behind
sentinels or duplicated compatibility layers.

## Technical Context

**Language/Version**: C++20, C11-compatible public header, Swift tools 5.9+

**Documentation Strand**: core

**Authoritative Context**: [C1 roadmap](../../docs/CORE_ROADMAP.md#phase-c1--runtime-ownership-and-recoverable-boundaries), [architecture](../../docs/ARCHITECTURE.md), [status](../../docs/STATUS.md), and [constitution](../../.specify/memory/constitution.md)

**Primary Dependencies**: C++ standard `std::jthread`, mutex, condition variable,
deque/variant/promise facilities; existing C ABI and Swift Package. No new package.

**Storage**: In-memory bounded command queue and diagnostic/result values only;
no persisted format. Test ledgers are generated under `.build/c1/`.

**Testing**: Existing C++ harness, focused `Tests/C1/` shell contracts, Swift
XCTest, a dedicated ThreadSanitizer build, existing sanitizer/C0 gates.

**Code Documentation**: Existing Doxygen + DocC pipeline; public declaration
contracts and developer documentation for every changed private/internal named
type or interface; `docs/code/runtime-ownership.md`, architecture,
host-boundary, and timing-guide updates; `make docs-check` retains zero debt.

**Git Checkpoints**: Each task begins with its named failing test where behavior
changes, passes focused verification, and is committed in Lore format before
the next task begins. The final task commit in every phase also records phase
completion; no empty duplicate checkpoint is created.

**Target Platform**: Portable C++ core on current Linux/macOS CI; Swift wrapper
on macOS 13+/iOS 16+ hosts.

**Project Type**: Emulator library, C ABI, Swift package, command-line hosts.

**Performance Goals**: No throughput promise. Command queue is bounded at 64;
pause/control latency is bounded by one 2,048-cycle minimum execution slice plus
scheduler time. C0 throughput remains comparison evidence only.

**Constraints**: One owner; no host clock in state transitions; no callback
while runtime synchronization is held; no exception across C; copied inputs;
safe shutdown; later bus-cycle work must retain the safe point.

**Scale/Scope**: One runtime per machine, four lifecycle states, seventeen command
kinds, one status taxonomy, C++/C/Swift contracts, race/replay evidence.

## Constitution Check

*GATE: Passed before research and re-checked after design.*

- [x] Core strand and authoritative documents are linked.
- [x] One independently testable runtime boundary; C2/C3/C4 concerns are excluded.
- [x] Fixed emulated slices preserve determinism and portability without host frameworks.
- [x] C++, C, Swift, stress, and documentation tests precede behavior changes.
- [x] Claims are limited to named contract, replay, and race evidence.
- [x] Ownership, lifetime, errors, threading, ABI revision, and no persisted format are explicit.
- [x] Public, private/internal named-abstraction, and conceptual documentation
      plus zero-debt validation are planned.
- [x] Content provenance is unchanged; accessibility is N/A for core-only work.
- [x] The owner/queue is justified; no dependency or general scheduler is added.
- [x] Every task has focused verification and a Lore commit; every phase has an explicit checkpoint.

**Post-design re-check**: Passed. Contracts define the complete state/command
matrix, safe point, status lifetime, bounded queue, shutdown, and intentional
0.2 boundary migration. The model and quickstart make replay/race claims
reproducible without widening scope.

## Project Structure

### Documentation (this feature)

```text
specs/002-runtime-ownership/
├── spec.md
├── plan.md
├── research.md
├── data-model.md
├── quickstart.md
├── contracts/
│   ├── runtime-state-machine.md
│   ├── c-runtime-api.md
│   └── swift-runtime-api.md
├── checklists/requirements.md
└── tasks.md
```

### Source Code (repository root)

```text
Sources/BeebCore/
├── include/beeb/runtime.hpp       # owner, states, commands, results
├── include/beeb/machine.hpp       # low-level owner-thread-only machine
├── include/beeb_c.h               # 0.2 structured public boundary
└── src/
    ├── runtime.cpp
    ├── machine.cpp
    └── beeb_c.cpp
Sources/BeebKit/BeebMachine.swift
Tests/
├── test_main.cpp                  # C++ runtime + C boundary contracts
├── BeebKitTests/BeebMachineTests.swift
└── C1/
    ├── test-runtime-contract.sh
    ├── test-runtime-replay.sh
    ├── test-runtime-races.sh
    └── test-documentation.sh
docs/
├── ARCHITECTURE.md
├── STATUS.md
├── CORE_ROADMAP.md
└── code/
    ├── runtime-ownership.md
    ├── host-boundary.md
    └── timing-model.md
```

**Structure Decision**: Add one focused runtime beside the existing machine,
not synchronization inside every device. Direct `BBCMicro` construction remains
for owner-thread unit tests; all supported host paths own `MachineRuntime`.
Generated stress/replay evidence stays untracked under `.build/c1/`.

## Complexity Tracking

| Addition | Why Needed | Simpler Alternative Rejected Because |
| --- | --- | --- |
| One owner thread and bounded FIFO | Sustained execution and cross-thread commands require exclusive ownership with back-pressure. | A mutex around direct calls cannot pause a long-running owner fairly or define command ordering. |
| Fixed execution-slice ledger | Makes the exact command/execution interleaving replayable without making wall time authoritative. | Replaying host commands alone omits how much emulated work occurred before arrival. |
| Intentional 0.2 C API revision | Every fallible operation needs an operation-scoped status and unambiguous output. | Parallel v1/v2 APIs retain ambiguous public entry points and duplicate the boundary during an early pre-1.0 release. |
