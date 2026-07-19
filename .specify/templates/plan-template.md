# Implementation Plan: [FEATURE]

**Branch**: `[###-feature-name]` | **Date**: [DATE] | **Spec**: [link]

**Input**: Feature specification from `/specs/[###-feature-name]/spec.md`

**Note**: This template is filled in by the `/speckit-plan` command. See `.specify/templates/plan-template.md` for the execution workflow.

## Summary

[Extract from feature spec: primary requirement + technical approach from research]

## Technical Context

<!--
  ACTION REQUIRED: Replace the content in this section with the technical details
  for the project. The structure here is presented in advisory capacity to guide
  the iteration process.
-->

**Language/Version**: [e.g., Python 3.11, Swift 5.9, Rust 1.75 or NEEDS CLARIFICATION]

**Documentation Strand**: [product | core | cross-strand]

**Delivery-Plan Trace**: [link to the selecting row or gate in
docs/product/MACHINE_DELIVERY_PLAN.md]

**Supporting Context**: [links to the required intent, capability, core,
architecture and status documents]

**Primary Dependencies**: [e.g., FastAPI, UIKit, LLVM or NEEDS CLARIFICATION]

**Storage**: [if applicable, e.g., PostgreSQL, CoreData, files or N/A]

**Testing**: [e.g., pytest, XCTest, cargo test or NEEDS CLARIFICATION]

**Code Documentation**: [language-appropriate browsable generators; affected
public contracts and private/internal named types or interfaces; source
comments/guides; documentation debt impact; validation command]

**Git Checkpoints**: [How every verified task is committed before the next task
and how the final task commit or a separate phase-closing commit records each
phase boundary]

**Target Platform**: [e.g., Linux server, iOS 15+, WASM or NEEDS CLARIFICATION]

**Project Type**: [e.g., library/cli/web-service/mobile-app/compiler/desktop-app or NEEDS CLARIFICATION]

**Performance Goals**: [domain-specific, e.g., 1000 req/s, 10k lines/sec, 60 fps or NEEDS CLARIFICATION]

**Constraints**: [domain-specific, e.g., <200ms p95, <100MB memory, offline-capable or NEEDS CLARIFICATION]

**Scale/Scope**: [domain-specific, e.g., 10k users, 1M LOC, 50 screens or NEEDS CLARIFICATION]

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

- [ ] The feature is named by a row or gate in the sole forward programme
      authority, `docs/product/MACHINE_DELIVERY_PLAN.md`; any unlisted proposal
      has stopped for a reviewed delivery-plan amendment.
- [ ] The feature is classified as product, core, or cross-strand and links only
      the supporting intent, capability, dependency, architecture and status
      documents required by that slice.
- [ ] The outcome is a bounded vertical slice that can be demonstrated and
      tested independently; non-goals are explicit.
- [ ] Core determinism and portability are preserved; host wall-clock timing or
      host frameworks do not become core dependencies.
- [ ] Test-first evidence is identified across C++, C, Swift, and product
      acceptance boundaries as applicable.
- [ ] Fidelity, compatibility, and performance claims name their test, trace,
      measurement, or primary reference, and known limits remain explicit.
- [ ] Ownership, lifetime, errors, threading, ABI compatibility, and persisted
      format versioning are defined for every affected boundary.
- [ ] Public contracts, private/internal named types and interfaces, non-obvious
      implementation behavior, conceptual-guide impact, and browsable-
      documentation validation are defined, or a concrete documentation `N/A`
      rationale is recorded.
- [ ] User-content provenance, import/export behavior, and legal constraints are
      addressed where relevant.
- [ ] Accessibility and failure recovery are specified for user-facing work, or
      a concrete `N/A` rationale is recorded.
- [ ] Every new dependency, abstraction, or constitution exception is justified
      below with the simpler alternative that was rejected.
- [ ] New or changed code does not increase recorded documentation debt, and
      generated documentation remains a reproducible build artifact.
- [ ] Every task has a focused verification and commit checkpoint before the
      next task, and every phase boundary will be committed without requiring
      empty duplicate commits.

## Project Structure

### Documentation (this feature)

```text
specs/[###-feature]/
├── plan.md              # This file (/speckit-plan command output)
├── research.md          # Phase 0 output (/speckit-plan command)
├── data-model.md        # Phase 1 output (/speckit-plan command)
├── quickstart.md        # Phase 1 output (/speckit-plan command)
├── contracts/           # Phase 1 output (/speckit-plan command)
└── tasks.md             # Phase 2 output (/speckit-tasks command - NOT created by /speckit-plan)
```

### Source Code (repository root)
<!--
  ACTION REQUIRED: Replace the placeholder tree below with the concrete layout
  for this feature. Delete unused options and expand the chosen structure with
  real paths (e.g., apps/admin, packages/something). The delivered plan must
  not include Option labels.
-->

```text
# [REMOVE IF UNUSED] Option 1: Single project (DEFAULT)
src/
├── models/
├── services/
├── cli/
└── lib/

tests/
├── contract/
├── integration/
└── unit/

# [REMOVE IF UNUSED] Option 2: Web application (when "frontend" + "backend" detected)
backend/
├── src/
│   ├── models/
│   ├── services/
│   └── api/
└── tests/

frontend/
├── src/
│   ├── components/
│   ├── pages/
│   └── services/
└── tests/

# [REMOVE IF UNUSED] Option 3: Mobile + API (when "iOS/Android" detected)
api/
└── [same as backend above]

ios/ or android/
└── [platform-specific structure: feature modules, UI flows, platform tests]
```

**Structure Decision**: [Document the selected structure and reference the real
directories captured above]

## Complexity Tracking

> **Fill ONLY if Constitution Check has violations that must be justified**

| Violation | Why Needed | Simpler Alternative Rejected Because |
|-----------|------------|-------------------------------------|
| [e.g., 4th project] | [current need] | [why 3 projects insufficient] |
| [e.g., Repository pattern] | [specific problem] | [why direct DB access insufficient] |
