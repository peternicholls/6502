<!--
Sync Impact Report
- Version: 1.5.0 -> 1.6.0
- Added principles: none
- Modified principles: none
- Modified section: Technical and Product Constraints
- Updated templates: none required; host technology is selected by the delivery
  plan and architecture sections already required by existing templates
- Updated guidance: docs/ARCHITECTURE.md,
  docs/IMPLEMENTATION_CONSTRAINTS.md, docs/product/DESKTOP_EXPERIENCE.md
- Migration:
  - the current SwiftUI root remains transitional
  - new macOS application structure is AppKit-first
  - terminal and later mobile hosts share the production runtime contracts
  - C++ remains independent of all Apple and terminal presentation frameworks
- Deferred items: none
-->

# Beeb Constitution

## Core Principles

### I. Product and Core Are Separate Strands

Every specification MUST be `product`, `core`, or `cross-strand`. Product work
defines user outcomes. Core work defines portable emulation behavior and
contracts. Cross-strand work MUST state the product, host, ABI and core
boundaries it crosses. Product intent MUST NOT be reported as implemented core
behavior, and core capability MUST NOT be treated as a complete application.
`docs/STATUS.md` is the sole authority for verified implementation. This keeps
long-term product intent useful without overstating the present system.

### II. The Core Is Deterministic and Portable

Emulated time MUST control core state transitions; host wall time MUST NOT drive
emulation. The dependency-light C++20 core MUST build and test independently of
UI, audio-device, file-picker, network and Apple frameworks. Host integration
belongs behind explicit C and Swift boundaries. Equal inputs, initial state and
emulated time MUST produce equal observable core results. This makes fidelity
reproducible and keeps the machine reusable across hosts.

### III. Fidelity Claims Require Evidence (NON-NEGOTIABLE)

Accuracy, compatibility, performance and timing claims MUST cite automated
tests, reproducible traces, measurements or primary references. Current limits
MUST appear in `docs/STATUS.md`. Detailed passed records MUST live under
`docs/completed/` or the completed feature run, not in current planning prose.
Planned, reserved and smoke-observed behavior MUST remain distinct from
verified behavior. Test ROMs and fixtures MUST have lawful, explicit
provenance. Evidence prevents confidence from outrunning what the project can
reproduce.

### IV. Delivery Is Test-First (NON-NEGOTIABLE)

Every behavior change MUST begin with a regression, contract or acceptance
test that fails for the expected reason. Implementation then follows
red-green-refactor: make the smallest correct change, pass the focused test and
run the relevant wider suite. Boundary changes MUST cover C++, C and Swift
where applicable. User-facing work MUST also build and launch the maintained
application, execute its documented acceptance journey and record the observed
visible, audible and interaction result on a named host/device or simulator;
unit tests alone are insufficient. Documentation/process changes do not require
synthetic unit tests, but MUST lock and validate links, formatting, schemas and
affected tooling. Tests and direct observation are delivery evidence.

### V. Boundaries Are Safe and Versioned

No C++ exception may cross the C ABI. Public contracts MUST define ownership,
lifetime, nullability, errors and threading. Cross-thread access MUST use the
runtime owner or immutable owned observations. Machine profiles, expansions,
persisted state and interchange formats MUST carry stable versioned identity.
Unknown or invalid identities MUST reject without partial mutation or fallback
to another machine. C and Swift MUST receive recoverable values they own.
Project releases follow Semantic Versioning, and C, Swift, CLI, `VERSION` and
`CHANGELOG.md` versions MUST agree.

### VI. User Content Remains User-Owned

The project MUST NOT bundle proprietary firmware, character ROMs, games or user
media. Imports MUST use host-approved access and MUST NOT silently change their
source. A mutation workflow is incomplete until it provides explicit export or
save behavior. The product MUST remain usable without downloaded native code or
a JIT requirement. These rules protect users, distribution options and legal
clarity.

### VII. Deliver Accessible Vertical Value Simply

Features MUST be the smallest independently demonstrable and testable slice
that produces user value or unlocks a named outcome. User-facing specifications
MUST cover keyboard access, assistive technology, relevant reduced-motion
behavior and recovery, or state a concrete `N/A`. Native hosts SHOULD follow
platform conventions because custom interaction adds accessibility and
maintenance risk. New dependencies, abstractions and layers require a current
need and MUST be rejected when an existing pattern or smaller design suffices.

### VIII. Code Documentation Is Maintained Knowledge

Public C++, C and Swift contracts MUST document purpose and applicable
parameters, results, ownership, lifetime, nullability, failures, threading,
side effects and invariants. Private/internal named types and interfaces MUST
document responsibility and important invariants. Non-obvious hardware,
timing, state and buffer decisions MUST explain rationale near the code or link
to one maintained conceptual guide. Comments MUST NOT narrate self-evident
syntax. Generated API documentation MUST be reproducible from tracked sources,
and changed public or complex code MUST NOT increase documentation debt.
Behavior and its explanation change in the same slice.

### IX. One Forward Programme Direction

`docs/product/MACHINE_DELIVERY_PLAN.md` MUST be the sole authority for delivery
order, next work, programme gates, committed profiles and promotion of future
options. Every feature MUST trace to a named row or gate before specification.
Unlisted work MUST stop for a reviewed plan amendment. `.specify/feature.json`
MUST name the exact current feature directory, and MUST be empty when no feature
is active. Branch names, directory numbers, modification times, completed runs
and archived plans MUST NOT infer active work. `docs/STATUS.md` alone may claim
verified implementation.

### X. Current, Completed, and Archived Are Disjoint

Current documents MUST state current intent, direction, boundaries, constraints
or verified state only. Passed ledgers and finished Spec Kit runs MUST move to
`docs/completed/` and `specs/completed/`. Superseded or abandoned material MUST
move intact to `docs/Archive/` instead of being rewritten as current guidance.
Completed and archived content MUST NOT select work or be silently edited to
match a new baseline. Live documents MUST link to one owner instead of copying
its requirements or evidence. This storage boundary makes temporal status
visible from the path and prevents old prose from contaminating current work.

## Technical and Product Constraints

- The supported foundation is a dependency-light C++20 core, stable C ABI and
  Swift `BeebKit` wrapper with host-specific frontends. The primary macOS
  application is AppKit-first; terminal and later iPhone/iPad hosts use the
  same production machine contracts. SwiftUI may be embedded selectively but
  is not the required macOS application architecture. Foundation, AppKit,
  UIKit, SwiftUI, Metal, AVAudio, TTY and ANSI services stay on the host side.
- The portable Make build and test path MUST remain maintained. A new
  third-party dependency requires explicit specification and plan rationale,
  including the rejected existing or platform alternative.
- `docs/product/MACHINE_DELIVERY_PLAN.md` owns direction;
  `docs/product/VISION.md` owns durable intent; `docs/ARCHITECTURE.md` owns
  current boundaries; `docs/IMPLEMENTATION_CONSTRAINTS.md` owns technical
  constraints for unfinished slices;
  `docs/product/DESKTOP_EXPERIENCE.md` owns desktop interaction direction; and
  `docs/STATUS.md` owns verified state.
- `docs/code/` contains maintained conceptual contracts. Generated output is a
  disposable build artifact and MUST NOT enter the runtime dependency graph.
- `docs/completed/`, `specs/completed/` and `docs/Archive/` are non-forward
  evidence or history. They MAY support research but MUST NOT add scope.
- Numeric fidelity, frame-rate, latency and throughput targets MUST define the
  fixture, host/toolchain, observation interval and tolerance.
- Releases MUST update `CHANGELOG.md` and follow `docs/RELEASING.md`. Breaking
  public or persisted-format changes require a major-version decision or an
  explicit migration and compatibility plan.

## Specification and Delivery Workflow

1. Select a named delivery-plan row or gate, then create one bounded feature
   with `/speckit-specify`. If no row exists, amend the plan first.
2. Classify the feature as product, core or cross-strand. Link only the current
   vision, architecture, status and implementation constraints the slice needs.
   Archived/completed material may be cited only as labelled evidence or
   research. State non-goals explicitly.
3. A specification MUST define an independently testable outcome, measurable
   success criteria, edge/failure cases, recovery, evidence, content/legal
   implications, accessibility and documentation impact, with concrete `N/A`
   rationales where allowed. User-facing work MUST define how the built
   application is launched, exercised and observed on each maintained platform
   it claims.
4. `/speckit-plan` MUST pass every Constitution Check before research and after
   design. Exceptions require Complexity Tracking and a rejected simpler
   alternative.
5. `/speckit-tasks` MUST put failing tests or required process evidence before
   implementation, preserve independently demonstrable slices and pair code
   changes with their documentation. Use `/speckit-analyze` before
   implementation when artifacts or strands interact.
6. Every task MUST be verified and committed before the next task starts. Every
   phase boundary MUST be committed. A final task commit MAY close its phase;
   empty checkpoint commits MUST NOT be created. Commits preserve unrelated
   changes and use the repository Lore format.
7. Implementation MUST use focused checks while iterating, then run affected
   Make, sanitizer, Swift, Xcode and generated-documentation gates. Applicable
   user-facing work MUST build, launch and directly observe the documented
   application journey; the evidence records platform, inputs and observed
   result. Every change MUST pass `git diff --check`.
8. Completion MUST update only the live owners affected by verified behavior or
   changed direction. After acceptance passes, clear `.specify/feature.json`
   and move the finished feature directory to `specs/completed/` in the same
   phase-closing change. Feature numbering MUST include completed directories
   so an identifier is never reused.

Repository work also follows `AGENTS.md`, including cleanup, verification and
Lore-commit requirements.

## Governance

This constitution is the highest project authority for specifications, plans
and tasks. Conflicting feature artifacts MUST be corrected before work
continues. `AGENTS.md` may add execution rules but MUST NOT weaken these
principles.

Amendments require rationale, a Sync Impact Report, migration guidance and
synchronized dependent templates. Versions use Semantic Versioning: MAJOR for
incompatible governance changes or removed principles, MINOR for a new
principle or materially expanded obligation, and PATCH for clarification
without changed intent. Every plan, implementation and completion review MUST
verify compliance; unresolved violations block implementation or release.

Commit and storage rules apply prospectively. Historical artifacts are not
rewritten merely to match a new constitution; they are moved intact and
labelled. Necessary corrections require a new adjacent note or an explicit
superseding artifact.

**Version**: 1.6.0 | **Ratified**: 2026-07-15 | **Last Amended**: 2026-08-01
