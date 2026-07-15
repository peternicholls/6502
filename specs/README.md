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

The governing quality and architecture rules are in
[the project constitution](../.specify/memory/constitution.md).
