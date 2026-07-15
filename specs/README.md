# Feature Specifications

This directory is the working area for current Spec Kit features. It starts
without an active feature specification intentionally: a feature directory is
created only when a bounded vertical slice has been selected.

## Before Starting a Feature

Classify the work as one of:

- `product`: a user journey or outcome in the Machine, Media, or Editor vision;
- `core`: a portable emulator capability or contract; or
- `cross-strand`: a product slice that requires a named core capability and an
  explicit host/core boundary.

Use the current documents as sources of truth:

- Product work starts with [the product vision](../docs/product/VISION.md) and
  [product roadmap](../docs/product/ROADMAP.md).
- Core work starts with [the core roadmap](../docs/CORE_ROADMAP.md),
  [architecture](../docs/ARCHITECTURE.md), and
  [implementation status](../docs/STATUS.md).
- Historical files under `docs/Archive/` are research input, not current
  specifications.

Do not create a single umbrella specification for the whole product. Prefer a
small slice that can be implemented, demonstrated, and verified independently.

## Artifact Flow

For a new slice, run the Spec Kit workflow in order:

1. `/speckit-specify` creates `specs/NNN-feature-name/spec.md`.
2. `/speckit-clarify` resolves material ambiguity when needed.
3. `/speckit-plan` creates the technical plan and design artifacts.
4. `/speckit-tasks` creates test-first, dependency-ordered implementation work.
5. `/speckit-analyze` checks consistency across the artifacts.
6. `/speckit-implement` executes the approved tasks and verification gates.

Each feature directory may contain `spec.md`, `plan.md`, `research.md`,
`data-model.md`, `contracts/`, `quickstart.md`, `tasks.md`, and focused
checklists. Keep decisions in the feature artifacts; update the project-level
status, architecture, roadmaps, and changelog when delivery changes them.

Every coding feature must treat code documentation as part of the changed
contract. Its specification states the affected public surfaces,
private/internal named types and interfaces, non-obvious behavior and
conceptual guides (or a concrete `N/A`); its plan selects the
language-appropriate browsable output and debt impact; and its tasks update the
documentation with the code and validate the generated result. Internal
documentation should explain responsibility, invariants, ownership, lifetime,
threading, and collaboration boundaries where applicable. Do not add comments
that only repeat what the code already says.

Implementation uses committed checkpoints as part of task completion. Verify
and commit each task before beginning another task, and commit phase-completion
changes before entering the next phase. A phase's final task commit may satisfy
both requirements when its Lore message explicitly records the phase boundary;
do not manufacture an empty second commit.

The governing quality and architecture rules are in
[the project constitution](../.specify/memory/constitution.md).
