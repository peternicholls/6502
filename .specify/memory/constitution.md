<!--
Sync Impact Report
- Version: 1.3.0 -> 1.4.0
- Added principle: IX. One Forward Programme Direction
- Modified principles: none
- Modified sections: Technical and Product Constraints; Specification and
  Delivery Workflow; Governance
- Updated templates: spec-template.md, plan-template.md, tasks-template.md,
  checklist-template.md
- Updated guidance: AGENTS.md, CONTRIBUTING.md, docs/README.md,
  docs/product/README.md, specs/README.md
- Migration: all active and future features must trace first to a named row or
  gate in docs/product/MACHINE_DELIVERY_PLAN.md; supporting catalogues,
  completed features and archived material cannot select work
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

### IX. One Forward Programme Direction

`docs/product/MACHINE_DELIVERY_PLAN.md` MUST be the sole authority for delivery
order, current direction, programme gates, committed machine profiles and
promotion of later work. Every new product, core or cross-strand specification
MUST trace to a named row or gate in that plan before planning begins. Work not
named there MUST first amend the plan through review. Product vision, product
and core catalogues, architecture, status, completed feature artifacts and
historical material MUST NOT independently choose next work or create a
delivery commitment. `docs/STATUS.md` remains the authority for verified
implementation claims. This single direction prevents plausible supporting
documents from becoming competing backlogs.

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
- The sole forward programme authority is
  `docs/product/MACHINE_DELIVERY_PLAN.md`. `docs/product/VISION.md` supplies
  durable intent; `docs/product/ROADMAP.md` and `docs/CORE_ROADMAP.md` are
  supporting capability/dependency catalogues; `docs/ARCHITECTURE.md` records
  current boundaries; and `docs/STATUS.md` records verified state. Files under
  `docs/Archive/` are historical input, not current specifications.
- Every specification MUST trace first to a named delivery-plan row or gate,
  then cite the supporting intent, architecture, status and technical context
  required by its strand.
- Compatibility, frame-rate, latency, or throughput targets MUST state how they
  will be measured. Visual presentation policy belongs in the host; machine
  timing remains a core concern.
- Releases MUST update `CHANGELOG.md` and follow `docs/RELEASING.md`. Breaking
  public contract or persisted-format changes require a major-version decision
  or an explicit migration and compatibility plan.

## Specification and Delivery Workflow

1. Start a bounded vertical slice with `/speckit-specify`; use
   `/speckit-clarify` when requirements contain material ambiguity.
2. Confirm the slice is named by a row or gate in
   `docs/product/MACHINE_DELIVERY_PLAN.md`, then classify it as product, core,
   or cross-strand and link its required supporting documents. If it is not
   named, amend the delivery plan before creating the feature. Record non-goals
   so supporting catalogues and historical documents cannot enter scope
   implicitly.
3. A specification MUST include an independently testable outcome, measurable
   success criteria, edge and failure cases, recovery behavior, evidence needed
   for fidelity claims, content/legal implications, and accessibility coverage
   for user-facing work (or a concrete `N/A` rationale).
   Every coding specification MUST also state which public contracts,
   private/internal named types and interfaces, non-obvious behavior, and
   conceptual guides change, or give a concrete
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
8. Completion MUST update affected status, architecture, delivery-plan,
   capability-catalogue, release, or user documentation according to each
   document's narrow authority. A feature is complete only when its acceptance
   evidence passes and no known limitation is presented as delivered behavior.

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

**Version**: 1.4.0 | **Ratified**: 2026-07-15 | **Last Amended**: 2026-07-19
