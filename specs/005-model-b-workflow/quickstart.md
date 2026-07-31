# Quickstart: Running Model B Workflow

## Planning prerequisites

```bash
./.specify/scripts/bash/check-prerequisites.sh --json --require-tasks --include-tasks
```

Before implementation, the command must resolve this feature and list `spec.md`,
`plan.md`, `research.md`, `data-model.md`, `contracts/`, `quickstart.md` and
`tasks.md`.

## Focused implementation loop

1. Add the smallest focused failing test for the task.
2. Implement only enough behavior to pass it.
3. Run directly affected boundary checks and commit the verified task.

The task document will name the exact workflow aggregate. Do not run the
audio-inclusive M1 acceptance until `machine-audio-output` is complete.

## Feature closure

Run the focused workflow aggregate, affected C1/C2 and target-profile checks,
Swift tests, the changed macOS build, `DOCS_BASE=HEAD make docs-check` and
`git diff --check`. Then build and launch `BeebDemo-macOS` on a named host and
record the firmware, BASIC, frame, control, failure and accessibility evidence.

## Content rule

Do not add proprietary OS, BASIC or language-ROM bytes to the repository. Use
lawful user-provided material for direct observation and synthetic or clean-room
fixtures for automated tests.
