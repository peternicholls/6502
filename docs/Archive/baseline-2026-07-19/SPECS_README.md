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

Every product, core and cross-strand feature starts with the sole forward
programme authority: the
[Machine delivery plan](../docs/product/MACHINE_DELIVERY_PLAN.md). The selected
feature must trace to a named row or gate there. If it does not, amend and review
the delivery plan before creating the feature.

Then use only the supporting context required by that selected slice:

- [product vision](../docs/product/VISION.md) for durable intent;
- [product capability catalogue](../docs/product/ROADMAP.md) for user-facing
  scope without priority;
- [core phase catalogue](../docs/CORE_ROADMAP.md) for technical dependencies and
  invariants;
- [architecture](../docs/ARCHITECTURE.md) for current boundaries; and
- [implementation status](../docs/STATUS.md) for verified behavior and gaps.
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
checklists. Keep slice-local decisions in the feature artifacts. Update the
delivery plan only when programme direction or a gate changes; update status,
architecture, capability catalogues and changelog only when their narrower
concerns change.

### Feature lifecycle

- `Draft` means requirements are still being shaped and are not approved for
  implementation.
- `Ready` means the complete spec/plan/tasks stack has passed analysis and its
  dependencies are satisfied.
- `Active` means implementation has begun on the selected feature branch.
- `Complete` means acceptance evidence passes, `STATUS.md` records verified
  behavior where applicable, and the delivery plan records gate/slice closure.

`.specify/feature.json` names only the currently selected feature. It is empty
when no feature is active; completed feature directories remain as decision and
evidence history and never become forward authority. Before running
`/speckit-plan`, `/speckit-tasks` or
`/speckit-implement`, create or select a fresh bounded feature and verify that
the pointer names it. Never leave the pointer on a completed feature merely to
make prerequisite scripts pass.

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
