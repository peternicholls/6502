# Implementation Plan: Core Baseline Evidence

**Branch**: `001-core-baseline-evidence` | **Date**: 2026-07-15 | **Spec**:
[spec.md](spec.md)

**Input**: Feature specification from
`/specs/001-core-baseline-evidence/spec.md`

## Summary

Turn the current emulator state into a reproducible C0 foundation without
changing production emulation behavior. Add one aggregate verification command,
lawfully generated deterministic boot/video references, explicit reference
update and performance-measurement flows, and browsable code documentation.
The evidence tooling observes only existing public machine/CPU/frame surfaces.
Doxygen documents the C++ core and C ABI, Swift-DocC documents `BeebKit`, and a
generated landing page connects those API references to focused conceptual
guides. The documentation gate covers all supported public surfaces and
ratchets existing internal debt rather than encouraging comments that restate
obvious code.

## Technical Context

**Language/Version**: C++20, C11-compatible public header, Swift tools 5.9+

**Documentation Strand**: core

**Authoritative Context**: [C0 roadmap](../../docs/CORE_ROADMAP.md#phase-c0--baseline-evidence),
[architecture](../../docs/ARCHITECTURE.md), [current status](../../docs/STATUS.md),
and [constitution](../../.specify/memory/constitution.md)

**Primary Dependencies**: Existing standard library, Make, Bash, Swift Package
Manager, test harness, demo-ROM generator, and headless runner; Doxygen as a
build-time tool; official `swiftlang/swift-docc-plugin` as a documentation-only
Swift package dependency pinned to a reviewed tools-5.9-compatible release

**Storage**: Tracked text/PPM references and provenance under
`Tests/Fixtures/C0/`; generated runs and documentation under `.build/c0/` and
`.build/docs/`; no database or persisted user format

**Testing**: Existing `make test`, `make sanitize`, `swift test`, `swift build`,
plus strict Bash integration/negative tests for verification, references,
measurements, and documentation quality

**Code Documentation**: Doxygen with `EXTRACT_ALL=NO` and warnings promoted to
failure for C/C++; DocC for Swift; `docs/code/` conceptual Markdown; one
`make docs` entry point producing `.build/docs/index.html`; a tracked debt
inventory and changed-surface ratchet validated by `make docs-check`

**Target Platform**: Portable core and evidence verification on current Linux
and macOS CI hosts; Swift and DocC portions on the current macOS Swift host

**Project Type**: Dependency-light emulator library, C ABI, Swift package,
command-line evidence tools, and native demo host

**Performance Goals**: Record at least five samples of a named clean-room
workload with median and range; establish comparison data only, with no release
threshold or product promise

**Constraints**: No proprietary ROM or media; no production core/API change;
deterministic evidence; ordinary verification cannot mutate references; no
runtime documentation dependency; generated outputs are not authoritative;
diagnostics are textual and do not rely on colour

**Scale/Scope**: One aggregate C0 flow; behavioral/sanitizer/version/C/Swift
boundary groups; one clean-room ROM workload; one bitmap and one Mode 7 exact
reference; one measurement schema; all supported public headers and Swift API;
representative CPU, machine, timing, host-boundary, evidence, and testing guides

## Constitution Check

*GATE: Passed before Phase 0 research and re-checked after Phase 1 design.*

- [x] The feature is classified as core and links the current core roadmap,
      architecture, status, and constitution.
- [x] The outcome is a bounded, independently demonstrable evidence and
      maintainability slice; hardware-fidelity and runtime changes are explicit
      non-goals.
- [x] The evidence tools observe current deterministic public state and add no
      wall-clock or host-framework dependency to production emulation.
- [x] Negative integration tests precede the aggregate verifier, immutable
      references, measurement validation, and documentation-quality behavior;
      existing C++, C-boundary, and Swift tests remain required evidence.
- [x] Boot, frame, version/error, and throughput claims name exact signatures,
      comparisons, suites, and measurement records; performance is explicitly
      descriptive.
- [x] No production ownership, lifetime, threading, ABI, or persisted format is
      changed. Evidence artifacts have explicit immutability and update
      contracts.
- [x] Fixtures are generated from redistributable repository sources with
      provenance; proprietary firmware, character generators, games, and user
      media are excluded.
- [x] No product UI is added. Text summaries identify failures without relying
      on colour, and failed verification leaves references unchanged.
- [x] Public contracts, complex timing/hardware rationale, conceptual guides,
      generated navigation, and documentation debt are explicitly designed and
      validated.
- [x] The build-time-only documentation tools are justified below; no new
      runtime abstraction or dependency is introduced.
- [x] Generated evidence and documentation stay under `.build/`; changed code
      cannot increase tracked documentation debt.

**Post-design re-check**: Passed. The contracts keep reference updates separate,
the model makes provenance and validity explicit, the quickstart exercises each
failure-sensitive path, and the documentation design enforces useful coverage
without broadening C0 into a production refactor.

## Project Structure

### Documentation (this feature)

```text
specs/001-core-baseline-evidence/
├── spec.md
├── plan.md
├── research.md
├── data-model.md
├── quickstart.md
├── contracts/
│   ├── baseline-verification.md
│   ├── code-documentation.md
│   ├── evidence-reference.md
│   └── measurement-record.md
├── checklists/
│   └── requirements.md
└── tasks.md
```

### Source Code (repository root)

```text
Makefile
Doxyfile
Package.swift
Package.resolved
.gitignore
.github/workflows/ci.yml

Tools/
├── beeb-evidence/main.cpp
├── beeb-headless/main.cpp
└── make-demo-rom/main.cpp

scripts/
├── build-docs.sh
├── measure-c0.sh
├── update-c0-reference.sh
└── verify-c0.sh

Sources/
├── BeebCore/
│   ├── include/beeb/*.hpp
│   ├── include/beeb_c.h
│   └── src/*.cpp
└── BeebKit/
    ├── Documentation.docc/
    │   └── BeebKit.md
    └── *.swift

Tests/
├── test_main.cpp
├── BeebKitTests/
└── C0/
    ├── testlib.sh
    ├── test-baseline-verifier.sh
    ├── test-demo-rom.sh
    ├── test-documentation.sh
    ├── test-fixture-evidence.sh
    ├── test-measurement-record.sh
    └── test-reference-update.sh

Tests/Fixtures/C0/
├── README.md
├── documentation-debt.txt
├── manifest.txt
├── approved-state.txt
├── bitmap.ppm
└── mode7.ppm

docs/
├── CORE_BASELINE.md
├── CODE_DOCUMENTATION.md
└── code/
    ├── architecture.md
    ├── evidence-and-testing.md
    ├── host-boundary.md
    └── timing-model.md

.build/
├── c0/                         # generated evidence and measurements
└── docs/                       # generated landing page and API sites
```

**Structure Decision**: Keep evidence implementation outside the production
core and use the existing Make/Swift build surfaces. `Tools/beeb-evidence`
creates canonical observable outputs; strict scripts orchestrate policy and
negative tests. Exact approved references live with test fixtures, while every
derived run and rendered documentation site remains ignored under `.build/`.
Documentation comments stay beside the symbols they govern, and conceptual
cross-component explanations live under `docs/code/`.

## Complexity Tracking

| Addition | Why Needed | Simpler Alternative Rejected Because |
| --- | --- | --- |
| Doxygen build-time tool | The portable C++ core and C ABI need linked, searchable API and source documentation with missing-contract diagnostics. | Handwritten Markdown duplicates declarations, cannot reliably cross-link symbols, and cannot enforce public-surface coverage. |
| Official Swift-DocC package plugin, exact compatible release pinned during setup | `BeebKit` needs the language-native symbol graph, source-comment rendering, and browsable static site from Swift Package Manager. | Feeding Swift to Doxygen loses Swift-native relationships and diagnostics; handwritten API pages drift. |
| Two generated API sites behind one landing page | Each language gets its strongest native generator while contributors get one discovery point. | A single non-native generator weakens one language; merging generator internals adds maintenance with no emulator value. |

Both additions are documentation-only. Graphviz and call-graph generation are
excluded from C0 because they add installation and output complexity without a
current learning requirement.
