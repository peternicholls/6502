# Tasks: Model B Acceptance Automation

**Input**: Design documents from `specs/007-model-b-acceptance-automation/`

**Prerequisites**: `plan.md`, `spec.md`, `research.md`, `data-model.md`,
`contracts/`, `quickstart.md`

**Git**: Verify and commit each task before starting the next. Preserve the
user's dirty `Sources/BeebDemo/main.swift` and untracked `BBC Micro ROMS/`.

## Phase 1: Foundational verification

- [X] T001 Confirm the active feature pointer, current branch and focused baseline with `./.specify/scripts/bash/check-prerequisites.sh --json --require-tasks --include-tasks`, `swift test --filter BeebMachineTests`, and `make test-model-b-workflow`; record that human T008/T010 gates remain open in `specs/007-model-b-acceptance-automation/quickstart.md`.

## Phase 2: User Story 1 — production input/output evidence (P1)

**Independent test**: Focused Swift tests submit matrix events through
`BeebMachine`, produce owned frame/audio observations and repeat deterministically.

- [X] T002 [US1] Add focused production-runtime input/frame/audio regression tests to `Tests/BeebKitTests/BeebMachineTests.swift`, using existing generated ROM helpers and asserting safe-point progress, owned output and deterministic replay.
- [X] T003 [US1] Run `swift test --filter BeebMachineTests/testProduction` and `swift test --filter BeebMachineTests/testDeterministic` (or the exact generated test filters), then commit the verified test evidence with a Lore message.

## Phase 3: User Story 2 — firmware failure recovery (P1)

**Independent test**: Reject invalid firmware after a valid load and prove the
valid profile, safe point and bounded execution remain usable.

- [X] T004 [US2] Add invalid-candidate preservation coverage to `Tests/BeebKitTests/BeebMachineTests.swift`, checking typed rejection, unchanged profile/safe-point state and successful reset/run afterward.
- [X] T005 [US2] Run the focused recovery tests plus `swift test --filter BeebMachineTests`, inspect the failure semantics, and commit the verified recovery task with a Lore message.

## Phase 4: User Story 3 — portable terminal-style evidence (P2)

**Independent test**: One workflow command runs the existing portable headless
host with a clean-room fixture and rejects empty or missing output.

- [X] T006 [US3] Add `Tests/ModelBWorkflow/test-runtime-acceptance.sh` to run the production Swift tests and portable headless fixture, checking successful status and non-empty deterministic evidence without proprietary ROM bytes.
- [X] T007 [US3] Add a direct `test-runtime-acceptance` Make target in `Makefile`, include the script in the Model B workflow aggregate, update `specs/007-model-b-acceptance-automation/quickstart.md`, and commit the verified automation task.

## Phase 5: Evidence and completion

- [X] T008 Update `specs/007-model-b-acceptance-automation/contracts/production-runtime-evidence.md` and add the dated automated-results section to `specs/007-model-b-acceptance-automation/evidence/automated-results.md`, explicitly leaving human keyboard, visual and assistive gates open.
- [X] T009 Run `make test`, `swift test`, `make test-model-b-workflow`, `make test-runtime-acceptance`, `make docs-check`, `make format-check`, and `git diff --check`; commit the phase checkpoint without staging unrelated user files.
- [X] T010 If and only if all required automated checks pass, mark completed automation tasks `[X]`, refresh `docs/STATUS.md` only for the verified automation claim, and leave T008/T010 from completed 005 open for the user's direct observations.

## Dependencies and execution order

`T001 → T002 → T003 → T004 → T005 → T006 → T007 → T008 → T009 → T010`.
T002 and T004 share `BeebMachineTests.swift`, so they are sequential. T006 and
T007 share the workflow aggregate/Makefile and are sequential. No task changes
the AppKit UI or user-owned dirty files.

## Completion boundary

This feature automates machine/runtime evidence only. It does not close the
direct user-owned-ROM BASIC typing, physical keyboard, visual CRT, accessibility
or rejected-import application observations required by the historical 005
acceptance record.
