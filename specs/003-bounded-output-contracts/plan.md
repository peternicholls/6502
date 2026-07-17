# Implementation Plan: Phase C2 Bounded Output Contracts

**Branch**: `003-bounded-output-contracts` | **Date**: 2026-07-17 | **Spec**: [spec.md](spec.md)

**Input**: Feature specification from `/specs/003-bounded-output-contracts/spec.md`

## Summary

Add bounded completed-frame and audio output production behind the C1
`MachineRuntime`, with structured output diagnostics across C++, C, and Swift.
Outputs are published only at the existing completed-instruction/device-tick
safe point. Consumers receive owned results: a three-frame FIFO drops the
oldest unconsumed frame on overflow, and a 4,096-sample mono Float32/48 kHz ring
reports demand around a 2,048-sample target and drops oldest samples on
overflow. Host-side observations calculate emulation rate without feeding host
time into core state. A committed `Beeb6502.xcodeproj` supplies shared macOS,
iOS Simulator, and test schemes over the existing sources and package products.

## Technical Context

**Language/Version**: C++20 core, C11-compatible ABI, Swift tools 5.9+

**Documentation Strand**: core

**Authoritative Context**: [CORE_ROADMAP.md](../../docs/CORE_ROADMAP.md#phase-c2--bounded-frame-audio-and-diagnostic-contracts), [ARCHITECTURE.md](../../docs/ARCHITECTURE.md), [STATUS.md](../../docs/STATUS.md), [C1 runtime contract](../002-runtime-ownership/contracts/runtime-state-machine.md), and [constitution](../../.specify/memory/constitution.md)

**Primary Dependencies**: Existing `MachineRuntime`, `BBCMicro`, renderer, SN76489/audio path, C ABI, BeebKit, Swift Package Manager, Makefile, XCTest, and installed Xcode SDKs. No new third-party dependency.

**Storage**: In-memory bounded output queues and operation-owned result values; no persisted format.

**Testing**: Existing C++ harness and `make test`/`make sanitize`, dedicated `Tests/C2/` aggregate and shell contracts, Swift XCTest, deterministic sustained-production/RSS measurement, `make thread-sanitize`, strict `C2_REQUIRE_TSAN=1 make test-c2-portable` on supported Linux CI, `make test-c2-xcode` on Apple CI, `make format-check`, branch-aware `make docs-check`, and `git diff --check`.

**Code Documentation**: Doxygen and Swift-DocC source comments for changed public contracts; purpose/invariant comments for internal output queues, owned results, producers, and diagnostics; guides in `docs/code/runtime-ownership.md`, `docs/code/host-boundary.md`, `docs/code/timing-model.md`, plus a C2 output guide; generated documentation must remain a build artifact and debt must not increase.

**Git Checkpoints**: Each task starts with a focused failing test or validation, is verified, and is committed in Lore format before the next task. Each phase is checkpointed; the final task may record phase completion without an empty duplicate commit.

**Target Platform**: Portable Linux/macOS C++ core; macOS 13+/iOS 16+ Swift hosts; Xcode 26.3 reference toolchain with generic iOS Simulator destinations and no user-specific signing requirement.

**Project Type**: Deterministic emulator library with C ABI, Swift package, and command-line evidence hosts.

**Performance Goals**: Over at least 60 emulated seconds after a 10-second warm-up, depths never exceed three frames/4,096 samples, produced output balances exactly against consumed+dropped+retained output, RSS growth is at most 16 MiB, and synthetic emulation-rate calculations are within 0.1% of expected. No broader throughput claim.

**Constraints**: C1 owner-only machine access; completed-instruction/device-tick publication; owned outputs only; no host wall-clock state transitions; no callback while synchronization is held; structured failures; no persisted output format; no Metal/AVAudioEngine/UI scheduling; Xcode project references existing sources/package products without duplication and does not replace portable gates.

**Scale/Scope**: One runtime per machine; three retained frames; 4,096 mono Float32 samples at 48 kHz; four independently testable stories; C++, C, Swift, Xcode project metadata/shared schemes, focused scripts, guides, and status evidence.

## Constitution Check

*GATE: Passed before Phase 0 research and re-checked after design.*

- [x] Core strand and current roadmap, architecture, status, and constitution are linked.
- [x] Three bounded output stories plus one independently testable Xcode delivery story are explicit; host presentation behavior, signing/distribution, and persistence are excluded.
- [x] Emulated time and C1 safe point remain authoritative; no host framework enters BeebCore.
- [x] C++, C ABI, Swift, empty/full/sustained, lifetime, and deterministic evidence are planned before behavior changes.
- [x] Capacity, duration, tolerance, and reproducibility are specified for measurable claims.
- [x] Owned C++/C/Swift lifetimes, exact capacities/policies, nullability, statuses, threading, and no persisted format are defined in contracts.
- [x] Public/internal documentation and generated validation are included.
- [x] No user content or accessibility surface is introduced; both are explicitly handled in the spec.
- [x] Existing C1 patterns are reused and no dependency/general scheduler is added.
- [x] Every task and phase has a verification and Lore checkpoint.

**Post-design re-check**: Passed. Research fixes owned-value, queue, sample-format, pressure, rate-observation, and Xcode-project decisions; contracts preserve the C1 owner and safe point; quickstart defines reproducible capacity/RSS/Xcode evidence; no host timing or Apple SDK dependency enters the core boundary.

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
│   ├── c2-output-swift.md
│   └── xcode-project.md
├── checklists/requirements.md
└── tasks.md
```

### Source Code (repository root)

```text
Sources/BeebCore/include/beeb/
├── runtime.hpp
└── output.hpp
Sources/BeebCore/include/beeb_c.h
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
    ├── test-output-measurement.sh
    ├── test-aggregate-runner.sh
    ├── test-xcode-project.sh
    └── test-documentation.sh
Beeb6502.xcodeproj/
├── project.pbxproj
└── xcshareddata/xcschemes/
    ├── BeebDemo-macOS.xcscheme
    ├── BeebDemo-iOS.xcscheme
    └── Beeb6502-Tests.xcscheme
docs/code/
├── runtime-ownership.md
├── host-boundary.md
├── timing-model.md
└── bounded-output.md
```

**Structure Decision**: Keep producers and bounded storage beside the existing runtime and machine, extend the existing public C/Swift seams, give C2 its own aggregate evidence runner, and commit a top-level Xcode project with shared schemes over existing sources/package products. No host presentation behavior, copied core tree, or new runtime dependency is introduced.

## Complexity Tracking

| Addition | Why Needed | Simpler Alternative Rejected Because |
| --- | --- | --- |
| Separate bounded frame/audio output queues | Different payload ownership, cadence, and consumption semantics need independently testable capacity and pressure policies. | One untyped queue would obscure lifetime and demand rules and make the public contract ambiguous. |
| Owned output values across every boundary | Consumers may outlive producer operations and must not see concurrent mutation. | Borrowing renderer-owned storage would invalidate retained output at the next production event. |
| Committed Xcode project | Apple development needs stable shared app/test schemes and clean-checkout `xcodebuild` evidence. | Opening `Package.swift` alone does not provide the requested project-level app delivery surface; duplicating sources in a separate project would create two authorities. |
