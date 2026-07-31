# Implementation Plan: Running Model B Workflow

**Branch**: `005-model-b-workflow` | **Date**: 2026-07-31 | **Spec**: [spec.md](spec.md)

## Summary

Deliver the visual/control half of M1 as one bounded vertical feature:
user-owned Model B OS and language-ROM onboarding, retained authorised
assignments, BASIC-ready boot, physical-keyboard input, continuous completed
frame presentation and recoverable controls. Reuse the owner-serialized
runtime and existing C/Swift boundary; do not add audio device, machine
snapshot/session-state persistence,
mobile, Model B+ or timing scope. See [research.md](research.md).

## Technical Context

**Language/Version**: C++20 core/C ABI; Swift 5.9 package; SwiftUI macOS host

**Documentation Strand**: cross-strand

**Delivery-Plan Trace**: [Critical-path slice 2 — `machine-model-b-workflow`](../../docs/product/MACHINE_DELIVERY_PLAN.md#critical-path-slices)

**Supporting Context**: [vision](../../docs/product/VISION.md), [architecture](../../docs/ARCHITECTURE.md), [implementation constraints](../../docs/IMPLEMENTATION_CONSTRAINTS.md), [status](../../docs/STATUS.md), and [evidence strategy](../../docs/code/evidence-and-testing.md). Completed target-profile material is labelled evidence only.

**Primary Dependencies**: Existing C++20 core, C ABI, BeebKit, SwiftUI/Foundation, Xcode project and standard macOS file selection; no new runtime dependency

**Storage**: Private copied ROM bytes in the machine; app-scoped read-only security-scoped bookmark data and non-sensitive display metadata for remembered user selections. M1 assigns the language ROM to fixed sideways bank 12; no bank-selection UI is added.

**Testing**: Focused host and boundary regressions, a focused Model B workflow aggregate, `make test-machine-target-profile`, `make test-c1`, `make test-c2-xcode`, `swift test`, `swift build`, the maintained macOS Xcode build, documentation checks, and direct macOS observation. Add C++/C cases only when this slice changes that boundary. The complete M1 audio-inclusive journey remains later.

**Code Documentation**: Update affected C/C++/Swift public contracts and named host state abstractions; update a conceptual guide only if a host-boundary or runtime invariant changes; run `DOCS_BASE=HEAD make docs-check`.

**Git Checkpoints**: Every task is verified and committed before the next. The final non-empty task commit records feature completion only after acceptance evidence passes.

**Target Platform**: macOS 13+ with a physical keyboard; portable C++/C/Swift contracts remain maintained

**Project Type**: Native desktop application with portable emulator library

**Performance Goals**: Present each available owned completed frame without host-driven emulation and record at least two frame identities during the BASIC journey. No new frame-rate or audio-performance claim.

**Constraints**: Model B only; 16 KiB MOS OS ROM and bounded sideways language-ROM assignment; private ROM copies; balanced file-access lifetime; one runtime owner and safe-point commands; no proprietary bytes in the repository.

**Scale/Scope**: One macOS Model B boot/type/run/video/control journey; no AVAudioEngine, snapshots, mobile adaptation, B+ behavior, media workflow, timing or editor work.

## Constitution Check

*Passed before Phase 0 research and re-checked after design.*

- [x] The feature is named by the sole delivery-plan authority and is classified cross-strand.
- [x] Only current vision, architecture, constraints and verified status define scope; completed material is evidence only.
- [x] The outcome is a bounded vertical slice with explicit non-goals.
- [x] Core determinism, portability, one owner and the completed-instruction safe point remain intact.
- [x] Test-first C++/C/Swift/product evidence, macOS launch observation, accessibility, recovery and lawful content handling are defined.
- [x] Ownership, lifetime, typed failures, ABI boundaries, documentation and generated-documentation validation are defined where affected.
- [x] No dependency, abstraction or constitution exception is introduced; the smaller existing-path design is retained.
- [x] Every task and phase boundary will have a focused verification and Lore checkpoint.
- [x] Completion will update only verified live owners, clear the pointer and archive the feature.

## Project Structure

```text
Sources/
├── BeebCore/
│   ├── include/beeb_c.h             # C boundary if a role/status gap is proven
│   └── src/                         # Model B loading/runtime only when required
├── BeebKit/
│   ├── BeebMachine.swift             # Owned Swift firmware/input/control values
│   └── Documentation.docc/           # Public wrapper contract if changed
└── BeebDemo/main.swift               # macOS onboarding, presentation, input and controls

Tests/
├── test_main.cpp                     # Core/C ABI regression only when core changes
├── BeebKitTests/BeebMachineTests.swift
└── ModelBWorkflow/                   # Focused feature aggregate and host/build probes

specs/005-model-b-workflow/
├── contracts/
├── data-model.md
├── research.md
├── quickstart.md
└── tasks.md
```

**Structure Decision**: Extend the existing vertical path rather than create a
host façade or another lifecycle owner. `ModelBWorkflow` follows
`TargetProfile`: it orchestrates the smallest affected tests without duplicating
broad C1/C2 suites.

## Complexity Tracking

No constitution violations or complexity exceptions are required.
