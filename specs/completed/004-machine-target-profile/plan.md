# Implementation Plan: Machine Target Profile

**Branch**: `004-machine-target-profile` | **Date**: 2026-07-20 | **Spec**: [spec.md](spec.md)

**Input**: Feature specification from `/specs/004-machine-target-profile/spec.md`

## Summary

Introduce one immutable, bounded and versioned machine-target value whose raw
component identifiers can represent later machines and expansions without a
closed Model B/B+ enumeration. Model B is the sole constructible profile in
this slice. Model B+ 64K is a distinct recognised profile whose attempted
construction returns a typed unavailable result until its later reference-led
implementation. The value enters `BBCMicro` and `MachineRuntime` construction,
is queryable through the runtime owner, crosses the C ABI as a fixed owned
aggregate, maps to a public Swift value, and drives an accessible profile
selector plus explicit active-profile state in the maintained macOS app.

## Technical Context

**Language/Version**: C++20 core, C11-compatible ABI, Swift tools 5.9+

**Documentation Strand**: cross-strand

**Delivery-Plan Trace**: [Specification sequence row 1 — `machine-target-profile`](../../docs/product/MACHINE_DELIVERY_PLAN.md#specification-sequence)

**Supporting Context**: [product vision](../../docs/product/VISION.md), [current architecture](../../docs/ARCHITECTURE.md), [target-profile constraints](../../docs/IMPLEMENTATION_CONSTRAINTS.md#target-profile), [verified status](../../docs/STATUS.md), [host-boundary guide](../../docs/code/host-boundary.md), and [evidence guide](../../docs/code/evidence-and-testing.md). [C1/C2 completed artifacts](../completed/) are evidence only and add no scope.

**Primary Dependencies**: Existing `BBCMicro`, `MachineRuntime`, structured C ABI, `BeebKit`, SwiftUI demo host, Swift Package Manager, Make, XCTest and checked-in Xcode project. Standard-library fixed/value storage only; no new third-party dependency.

**Storage**: In-memory immutable profile values only. No snapshot, preference, firmware or media persistence in this slice.

**Testing**: Test-first C++ profile validation and machine/runtime construction tests; C ABI null, malformed, unsupported, output-preservation and query tests; Swift value/error/query tests; Model B replay regression; focused `Tests/TargetProfile/` contract, aggregate, documentation and application-build scripts; `make test-machine-target-profile`, `make test`, `make sanitize`, `swift test`, `swift build`, maintained macOS/iOS Xcode builds, `make docs-check`, `make format-check`, and `git diff --check`. Product acceptance additionally builds and launches `BeebDemo-macOS`, exercises the Model B and Model B+ 64K choices, observes active identity and rejection/recovery through the UI and accessibility surface, and records the named host/toolchain and result in the feature evidence directory.

**Code Documentation**: Doxygen contracts for the new profile value, constants, validation, construction and query surfaces; Swift-DocC for public profile/support/error values and `BeebMachine` construction/query; responsibility and invariants for internal validation/translation helpers and host selection state; update `docs/code/host-boundary.md` and add a focused target-profile guide if the implementation needs rationale beyond declarations. Run `DOCS_BASE=develop make docs-check` before completion and prohibit increased documentation debt.

**Git Checkpoints**: Each task begins with a focused failing test or process check, passes its named verification, and is committed in Lore format before the next task. Each phase is explicitly closed by its final task commit or a non-empty phase checkpoint; completed-task changes never leak into later commits.

**Target Platform**: Portable Linux/macOS C++ core; macOS 13+ and iOS 16+ Swift build surfaces; direct application observation on a named macOS 13+ host using the checked-in `BeebDemo-macOS` scheme.

**Project Type**: Deterministic emulator library with C ABI, Swift package and native SwiftUI host.

**Performance Goals**: Profile validation, construction and query are bounded control-plane operations, never advance emulated time and add no work to instruction/device hot paths. No new emulation-throughput claim is made. The complete documented application observation remains reproducible in at most 10 minutes as required by SC-004.

**Constraints**: Stable raw 32-bit base identifiers are `0x00000001` for BBC Microcomputer Model B and `0x00000002` for BBC Model B+ 64K, each at component version 1; assigned codes are never reused or reinterpreted. No expansion identifier is assigned. Schema version 1 admits exactly 0...16 expansion identities for structural classification and any count above 16 rejects before storage access; deterministic canonical expansion ordering; no duplicates; classification precedence is malformed, unknown, incompatible, then recognised-unavailable; Model B+ 64K is the only recognised-unavailable identity; unassigned future-option fixtures remain unknown and non-canonical; Model B+ identity never selects Model B behavior; legacy no-argument construction is documented as an explicit Model B convenience, not fallback; runtime-owner ordering, byte-for-byte C failure-output preservation and owned Swift values remain intact. The in-memory aggregate is not a persisted byte format. The additive public profile API advances the synchronized development candidate from 0.3.0 to 0.4.0 at feature completion.

**Scale/Scope**: Two named base identities, zero implemented expansions, up to 16 representable expansions, four language/product boundaries, three user stories and one macOS acceptance journey containing two unique selections: Model B and Model B+ 64K recognised-unavailable. B+ machine behavior, B+ 128K/Master/expansion constants, firmware onboarding, snapshots, serialization, media, timing and polished multi-platform selection remain later slices.

## Constitution Check

*GATE: Passed before Phase 0 research and re-checked after Phase 1 design.*

- [x] The feature is specification-sequence row 1 in `docs/product/MACHINE_DELIVERY_PLAN.md`; no supporting document adds work.
- [x] The cross-strand classification and required current vision, architecture, constraints and status owners are explicit.
- [x] Completed C1/C2 work is evidence only; archived material supplies no scope or completion claim.
- [x] Stable identity transport, Model B construction, distinct B+ recognition and safe rejection form one independently testable slice; all machine-behavior and persistence work is excluded.
- [x] Profile operations do not advance emulated time or add host dependencies to the portable core.
- [x] Failing C++, C, Swift and product-acceptance evidence precedes implementation at each boundary.
- [x] Acceptance builds and launches the maintained macOS application, exercises the documented journey and records visible/accessibility/recovery observations; unit tests cannot close the feature.
- [x] The feature makes no new fidelity or throughput claim; historical labels cite primary Acorn material and known implementation limits stay explicit.
- [x] Profile values are immutable and owned; construction/query threading, C output preservation, ABI shape, schema/component versions and later snapshot suitability are defined in contracts.
- [x] Public contracts, internal validators/translators, host selection invariants, conceptual-guide impact and reproducible Doxygen/DocC validation are planned.
- [x] No user content is read, written or bundled; profile metadata cannot imply firmware ownership or compatibility.
- [x] Profile labels, selected state and rejection are keyboard and VoiceOver observable and do not depend on colour or motion.
- [x] The fixed value contract reuses current status/owner patterns; no dependency or competing configuration service is introduced.
- [x] Documentation changes accompany their code and the documentation-debt gate remains mandatory.
- [x] Every task names focused verification and is committed before the next; phase checkpoints are non-empty and explicit.
- [x] Completion clears `.specify/feature.json`, updates only affected live owners and moves the accepted feature directory to `specs/completed/`.

**Post-design re-check**: Passed. Research separates historically accurate
base labels from optional expansions; the data model represents unknown future
values without accepting or canonically naming them; the 16-entry bound and
multi-defect classification precedence are explicit; contracts preserve runtime
ownership and failure-atomic C outputs; Swift receives value semantics; and the
quickstart requires direct application and accessibility observation. Model B+
64K is explicitly the single unsupported application selection. No constitution
exception remains.

## Project Structure

### Documentation (this feature)

```text
specs/004-machine-target-profile/
├── spec.md
├── plan.md
├── research.md
├── data-model.md
├── quickstart.md
├── contracts/
│   ├── core-profile.md
│   ├── c-profile-api.md
│   ├── swift-profile-api.md
│   └── host-profile-observation.md
├── checklists/requirements.md
├── evidence/
│   └── macos-application-observation.md  # created during implementation
└── tasks.md
```

### Source Code (repository root)

```text
Sources/BeebCore/include/beeb/
├── profile.hpp                         # new stable value and validation contract
├── machine.hpp                         # profile-aware construction/query
└── runtime.hpp                         # owner-serialized profile query
Sources/BeebCore/include/beeb_c.h        # fixed C value, create/query/validate API
Sources/BeebCore/src/
├── profile.cpp                         # canonicalization and validation
├── machine.cpp                         # supported-profile construction
├── runtime.cpp                         # profile command/result
└── beeb_c.cpp                          # C translation and failure containment
Sources/BeebKit/
├── BeebMachine.swift                   # public Swift profile and typed rejection
└── Documentation.docc/BeebKit.md       # profile usage and support distinction
Sources/BeebDemo/main.swift              # accessible selection and active identity
Tests/
├── test_main.cpp                        # C++/C construction, query and atomicity
├── BeebKitTests/BeebMachineTests.swift # Swift value/error/query coverage
└── TargetProfile/
    ├── test-contract.sh
    ├── test-boundaries.sh
    ├── test-aggregate-runner.sh
    ├── test-application-build.sh
    ├── test-documentation.sh
    └── testlib.sh
Makefile                                 # focused target-profile aggregate
docs/code/host-boundary.md               # maintained cross-language rationale
docs/code/target-profile.md              # stable identity/support guide
docs/STATUS.md                            # update only after acceptance passes
docs/product/MACHINE_DELIVERY_PLAN.md     # state change only after acceptance
```

**Structure Decision**: Add one profile module beside the machine/runtime,
extend the existing structured C and Swift seams, and keep the app selector in
the existing shared demo source. A focused `Tests/TargetProfile/` aggregate
provides evidence without renaming this cross-strand slice as C3, which remains
reserved for snapshot continuity.

## Complexity Tracking

No constitution violations require justification. The bounded raw-identifier
value is the smallest design that simultaneously represents unknown future
identities, prevents Swift/C closed-enum fallback and remains suitable for a
later versioned snapshot envelope.
