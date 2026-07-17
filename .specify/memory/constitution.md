<!--
Sync Impact Report
- Version: 1.2.0 -> 1.3.0
- Added principles: none
- Modified principles: VIII. Code Documentation Is Maintained Knowledge
- Modified sections: Specification and Delivery Workflow; Governance
- Updated templates: plan-template.md, tasks-template.md, checklist-template.md
- Updated guidance: docs/CODE_DOCUMENTATION.md, specs/README.md, active C1
  specification and tasks
- Migration: active and future coding work documents private/internal named
  types and interfaces as developer-facing architecture; trivial members and
  self-evident helpers remain exempt from prose-for-coverage
- Deferred items: none
-->

# Beeb Constitution

## Core Principles

### I. Product and Core Are Separate Strands

Every specification MUST identify itself as `product`, `core`, or
`cross-strand`. Product specifications define user problems, journeys, and
outcomes for Machine, Media, and Editor. Core specifications define portable
emulation capabilities and contracts. Cross-strand specifications MUST state
the boundary between those concerns. Product aspirations MUST NOT be reported
as implemented core behavior, and core capability MUST NOT be treated as a
complete product experience. `docs/STATUS.md` is the authority for verified
implementation status. This separation keeps the long-term vision useful
without allowing it to overstate the present system.

### II. The Core Is Deterministic and Portable

The emulated machine clock MUST be the authority for core state transitions;
host wall-clock timing MUST NOT drive emulation behavior. The dependency-light
C++20 core MUST remain independently buildable and testable, with no direct
dependency on UI, audio, file-picker, network, or Apple platform frameworks.
Host integrations belong behind explicit C or Swift boundaries. The same
inputs, initial state, and emulated time MUST produce the same observable core
result. This makes fidelity testable and keeps the core reusable across hosts.

### III. Fidelity Claims Require Evidence (NON-NEGOTIABLE)

Claims about accuracy, compatibility, performance, or timing MUST be supported
by automated tests, reproducible traces, measurements, or a cited primary
reference. Known limitations MUST be recorded in `docs/STATUS.md`, and planned
work MUST remain distinct from completed work. Device and timing work SHOULD be
driven by compatibility evidence and user value rather than checklist
completion. Test ROMs and fixtures MUST have clear, lawful provenance. This
prevents confidence from outrunning evidence.

### IV. Delivery Is Test-First (NON-NEGOTIABLE)

Every behavior change MUST begin with a regression, contract, or acceptance
test that fails for the expected reason before implementation. Work then
follows red-green-refactor: make the smallest correct change, pass the focused
test, and run the relevant wider suite. Boundary changes MUST cover the C++
core, C ABI, and Swift integration where applicable. Documentation-only and
process-only changes do not require synthetic unit tests, but MUST validate
links, formatting, and any affected tooling. Tests are delivery evidence, not
an optional follow-up.

### V. Boundaries Are Safe and Versioned

No C++ exception may cross the C ABI. Public contracts MUST define ownership,
lifetime, nullability, errors, and threading expectations. Cross-thread access
MUST use explicit synchronization or immutable snapshots. Persisted machine
state, media metadata, and other interchange formats MUST be versioned before
they become user-facing. Failures MUST be recoverable and represented in a form
the host can handle. Project releases follow Semantic Versioning, and versions
reported by the C API, Swift API, CLI, `VERSION`, and `CHANGELOG.md` MUST remain
synchronized. Stable boundaries allow the emulator and native product to
evolve independently.

### VI. User Content Remains User-Owned

The project MUST NOT bundle proprietary ROMs, character ROMs, games, or user
media. Imports MUST use host-approved access mechanisms and MUST NOT silently
modify their source. Any mutation workflow MUST provide an explicit export or
save operation before it is considered complete. The product MUST remain
usable without executing downloaded native code or adding a JIT requirement.
These rules protect users, distribution options, and the project's legal
clarity.

### VII. Deliver Accessible Vertical Value Simply

Features MUST be planned as the smallest independently demonstrable and
testable slice that produces product value or unlocks a clearly named product
outcome. Product and UI specifications MUST cover keyboard access, assistive
technology, reduced-motion behavior where relevant, and recovery from failed
imports, exports, or emulation operations. Native hosts SHOULD follow platform
conventions while leaving the core host-agnostic. New dependencies,
abstractions, and layers require a current concrete need and MUST be rejected
when an existing pattern or smaller design suffices. This keeps foundational
work connected to a usable, inclusive product.

### VIII. Code Documentation Is Maintained Knowledge

Supported public C++, C, and Swift contracts MUST document their purpose and,
where applicable, parameters, results, ownership, lifetime, nullability,
failure behavior, threading expectations, side effects, and invariants.
Private/internal named types and interfaces MUST document their purpose,
responsibility boundary, and important invariants, plus ownership, lifetime,
threading, collaborators, and extension constraints where applicable.
Non-obvious hardware behavior, timing decisions, state transitions, and buffer
rules MUST explain the rationale and observable consequences near the code or
link to an authoritative conceptual guide. Comments MUST NOT merely restate
names, types, or self-evident control flow. Cross-component concepts MUST have
browsable guides, and generated API documentation MUST be reproducible from
tracked source. New or changed public and complex code MUST pass documentation
generation and MUST NOT increase recorded documentation debt. Documentation is
part of the contract: when behavior changes, its explanation changes in the
same slice.

## Technical and Product Constraints

- The supported foundation is a dependency-light C++20 core, a stable C ABI,
  a Swift `BeebKit` wrapper, and native SwiftUI hosts built with Swift Package
  Manager. Platform facilities such as Metal and AVAudio belong on the host
  side of the boundary.
- The portable build and test path MUST remain usable through the repository
  Makefile. A new third-party dependency requires an explicit specification and
  plan justification, including why existing code or platform APIs are
  insufficient.
- Code documentation MUST use language-appropriate generators that produce
  cross-referenced browsable output. Generated output is a build artifact, not
  an authoritative tracked source. Documentation tooling MUST remain build-time
  only and MUST NOT enter the emulator runtime dependency graph.
- Current documentation is split into two authorities: `docs/product/` for the
  wider Machine, Media, and Editor vision, and the core roadmap, architecture,
  references, and status documents under `docs/` for emulator delivery. Files
  under `docs/Archive/` are historical input, not current specifications.
- Product specifications MUST trace to `docs/product/VISION.md` and the active
  product roadmap horizon. Core specifications MUST trace to
  `docs/CORE_ROADMAP.md`, `docs/ARCHITECTURE.md`, and `docs/STATUS.md`.
- Compatibility, frame-rate, latency, or throughput targets MUST state how they
  will be measured. Visual presentation policy belongs in the host; machine
  timing remains a core concern.
- Releases MUST update `CHANGELOG.md` and follow `docs/RELEASING.md`. Breaking
  public contract or persisted-format changes require a major-version decision
  or an explicit migration and compatibility plan.

## Specification and Delivery Workflow

1. Start a bounded vertical slice with `/speckit-specify`; use
   `/speckit-clarify` when requirements contain material ambiguity.
2. Classify the slice as product, core, or cross-strand and link its current
   source documents. Record non-goals so the abandoned implementation details
   in historical documents cannot enter scope implicitly.
3. A specification MUST include an independently testable outcome, measurable
   success criteria, edge and failure cases, recovery behavior, evidence needed
   for fidelity claims, content/legal implications, and accessibility coverage
   for user-facing work (or a concrete `N/A` rationale).
   Every coding specification MUST also state which public contracts,
   non-obvious behavior, and conceptual guides change, or give a concrete
   documentation `N/A` rationale.
4. `/speckit-plan` MUST pass every Constitution Check before Phase 0 research
   and MUST repeat that check after design. Any exception belongs in Complexity
   Tracking with a rejected simpler alternative.
5. `/speckit-tasks` MUST put failing tests or other required evidence before
   implementation tasks, preserve user-story independence, and pair affected
   coding tasks with documentation source and generated-output validation.
   Run `/speckit-analyze` before implementation when multiple artifacts or
   strands are involved.
6. Each task MUST be verified and committed before work begins on another task.
   Each phase MUST have its completion changes committed before the next phase
   begins. The final task commit MAY serve as the phase checkpoint when it
   explicitly records phase completion; an empty duplicate commit MUST NOT be
   created. Commits MUST keep task scope reviewable, preserve unrelated user
   changes, and follow the repository Lore commit format.
7. Implementation MUST use focused verification while iterating, then run the
   affected repository gates: `make test`, `make sanitize`, `swift test`, and
   `swift build` as applicable. Coding changes MUST also run the documentation
   generation and link/markup quality gates. All changes MUST pass
   `git diff --check`.
8. Completion MUST update affected status, architecture, roadmap, release, or
   user documentation. A feature is complete only when its acceptance evidence
   passes and no known limitation is presented as delivered behavior.

Repository work also follows the active `AGENTS.md` operating contract,
including its cleanup, verification, and commit-history requirements.

## Governance

This constitution is the highest project-level authority for specifications,
plans, and tasks. A conflicting feature artifact MUST be corrected before work
continues. `AGENTS.md` may add execution rules but MUST NOT weaken these
principles.

Amendments require a written rationale, an impact report, migration guidance
for affected active specifications, and synchronized changes to dependent Spec
Kit templates. Constitution versions use Semantic Versioning: MAJOR for an
incompatible governance change or principle removal, MINOR for a new principle
or materially expanded obligation, and PATCH for clarification without changed
intent. Every plan and implementation review MUST verify compliance; unresolved
violations block implementation or release.

The per-task and per-phase commit requirement applies prospectively to active
and future work. Amendments do not require rewriting compliant historical
commits or creating empty commits solely to mark a phase.

**Version**: 1.3.0 | **Ratified**: 2026-07-15 | **Last Amended**: 2026-07-15
