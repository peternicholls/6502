# Implementation Plan: Phase C2 Bounded Output Contracts

**Branch**: `003-bounded-output-contracts` | **Date**: 2026-07-17 | **Spec**: [spec.md](spec.md)

**Input**: Feature specification from `/specs/003-bounded-output-contracts/spec.md`

## Summary

Add bounded completed-frame and audio output production behind the C1
`MachineRuntime`, with structured output diagnostics across C++, C, and Swift.
Outputs are published only at the existing completed-instruction/device-tick
safe point. Consumers receive immutable or explicitly owned results, finite
capacity is enforced with deterministic policies, and host timing remains
outside core state transitions.

## Technical Context

**Language/Version**: C++20 core, C11-compatible ABI, Swift tools 5.9+

**Documentation Strand**: core

**Authoritative Context**: [CORE_ROADMAP.md](../../docs/CORE_ROADMAP.md#phase-c2--bounded-frame-audio-and-diagnostic-contracts), [ARCHITECTURE.md](../../docs/ARCHITECTURE.md), [STATUS.md](../../docs/STATUS.md), [C1 runtime contract](../002-runtime-ownership/contracts/runtime-state-machine.md), and [constitution](../../.specify/memory/constitution.md)

**Primary Dependencies**: Existing `MachineRuntime`, `BBCMicro`, renderer, SN76489/audio path, C ABI, BeebKit, Makefile, XCTest. No new dependency.

**Storage**: In-memory bounded output queues and operation-owned result values; no persisted format.

**Testing**: Existing C++ harness and `make test`/`make sanitize`, focused `Tests/C2/` shell contracts, Swift XCTest, deterministic sustained-production measurement, `make docs-check`, and `git diff --check`.

**Code Documentation**: Doxygen and Swift-DocC source comments for changed public contracts; purpose/invariant comments for internal output queues, views, producers, and diagnostics; guides in `docs/code/runtime-ownership.md`, `docs/code/host-boundary.md`, `docs/code/timing-model.md`, plus a C2 output guide; generated documentation must remain a build artifact and debt must not increase.

**Git Checkpoints**: Each task starts with a focused failing test or validation, is verified, and is committed in Lore format before the next task. Each phase is checkpointed; the final task may record phase completion without an empty duplicate commit.

**Target Platform**: Portable Linux/macOS C++ core; macOS 13+/iOS 16+ Swift hosts.

**Project Type**: Deterministic emulator library with C ABI, Swift package, and command-line evidence hosts.

**Performance Goals**: Bounded memory under at least 60 emulated seconds of sustained output; no throughput claim beyond measured workload; normal polling/draining must not block the owner on host presentation.

**Constraints**: C1 owner-only machine access; completed-instruction/device-tick publication; no host wall-clock state transitions; no callback while synchronization is held; copied inputs; structured failures; no persisted output format; no Metal/AVAudioEngine/UI scheduling.

**Scale/Scope**: One runtime per machine; finite frame/audio capacities; three independently testable stories; C++, C, Swift, focused scripts, guides, and status evidence.

## Constitution Check

*GATE: Passed before Phase 0 research and re-checked after design.*

- [x] Core strand and current roadmap, architecture, status, and constitution are linked.
- [x] Three bounded independently testable output stories are explicit; host presentation and persistence are excluded.
- [x] Emulated time and C1 safe point remain authoritative; no host framework enters BeebCore.
- [x] C++, C ABI, Swift, empty/full/sustained, lifetime, and deterministic evidence are planned before behavior changes.
- [x] Capacity, duration, tolerance, and reproducibility are specified for measurable claims.
- [x] Ownership, lifetime, nullability, statuses, threading, and no persisted format are defined in contracts.
- [x] Public/internal documentation and generated validation are included.
- [x] No user content or accessibility surface is introduced; both are explicitly handled in the spec.
- [x] Existing C1 patterns are reused and no dependency/general scheduler is added.
- [x] Every task and phase has a verification and Lore checkpoint.

**Post-design re-check**: Passed. Research resolves output view and queue policy choices; contracts preserve the C1 owner and safe point; quickstart defines reproducible evidence; no implementation dependency or host timing has entered the core boundary.

## Project Structure

### Documentation (this feature)

```text
specs/003-bounded-output-contracts/
├── spec.md
├── plan.md
├── research.md
├── data-model.md
├── quickstart.md
├── contracts/
│   ├── c2-output-cpp.md
│   ├── c2-output-c-api.md
│   └── c2-output-swift.md
├── checklists/requirements.md
└── tasks.md
```

### Source Code (repository root)

```text
Sources/BeebCore/include/beeb/
├── runtime.hpp
├── output.hpp
└── beeb_c.h
Sources/BeebCore/src/
├── runtime.cpp
├── output.cpp
└── beeb_c.cpp
Sources/BeebKit/BeebMachine.swift
Tests/
├── test_main.cpp
├── BeebKitTests/BeebMachineTests.swift
└── C2/
    ├── test-output-contract.sh
    ├── test-output-lifetime.sh
    ├── test-output-replay.sh
    ├── test-output-races.sh
    └── test-documentation.sh
docs/code/
├── runtime-ownership.md
├── host-boundary.md
├── timing-model.md
└── bounded-output.md
```

**Structure Decision**: Keep producers and bounded storage beside the existing runtime and machine, extend the existing public C/Swift seams, and keep C2 evidence in focused `Tests/C2` scripts. No host presentation code or new target is introduced.

## Complexity Tracking

| Addition | Why Needed | Simpler Alternative Rejected Because |
| --- | --- | --- |
| Separate bounded frame/audio output queues | Different payload ownership, cadence, and consumption semantics need independently testable capacity and pressure policies. | One untyped queue would obscure lifetime and demand rules and make the public contract ambiguous. |
| Operation-owned output views/results | Consumers may outlive producer operations and must not see concurrent mutation. | Borrowing renderer-owned storage would invalidate retained views at the next frame. |
