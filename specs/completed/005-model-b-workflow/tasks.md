# Tasks: Running Model B Workflow

**Input**: `spec.md`, `plan.md`, `research.md`, `data-model.md`, `contracts/` and `quickstart.md`

**Evidence**: Each behavior task starts with a focused failing check, implements the smallest change, and closes with its affected checks. The full audio-inclusive M1 gate remains outside this feature.

**Git**: Commit each verified task before beginning the next. A task may close a phase when its Lore message records that phase checkpoint.

## Phase 1: Focused Workflow Harness

**Purpose**: Establish and enforce one small aggregate for this vertical slice.

- [X] T001 Create `Tests/ModelBWorkflow/testlib.sh`, `Tests/ModelBWorkflow/test-aggregate-runner.sh`, `Tests/ModelBWorkflow/test-ci-contract.sh` and the `test-model-b-workflow` target in `Makefile`; add `make test-model-b-workflow` to the `apple-package` job in `.github/workflows/ci.yml`; verify the empty aggregate and source contract, then commit the harness/CI checkpoint.

**Checkpoint**: T001 passes before user-story work begins.

## Phase 2: User Story 1 — Import Firmware and Reach BASIC (Priority: P1)

**Goal**: A macOS user assigns remembered Model B OS/language ROMs and reaches BASIC with safe recovery.

**Independent Test**: A clean session accepts lawful fixture-equivalent role values, assigns the language ROM to bank 12, exposes active assignments and reaches BASIC-ready output; a rejected candidate preserves the working session.

- [X] T002 [US1] Add failing role-validation, fixed-bank-12, failed-candidate-preservation and stale-output checks in `Tests/BeebKitTests/BeebMachineTests.swift` and the focused aggregate; implement typed role assignment in `Sources/BeebKit/BeebMachine.swift`, adding `Sources/BeebCore/include/beeb_c.h`, `Sources/BeebCore/src/beeb_c.cpp` or `Tests/test_main.cpp` only if a changed boundary requires it; pass the focused checks and commit the firmware-boundary checkpoint.
- [X] T003 [US1] Add failing import, remembered-assignment, reselect-recovery and accessible-status probes; implement `Sources/BeebDemo/main.swift` file import, private byte loading, balanced read-only security-scoped bookmark access, fixed bank-12 installation, assignment presentation and BASIC-ready reset; resolve the sandbox/signing decision using the exact `Beeb6502.xcodeproj/project.pbxproj` target and add the smallest required entitlement/configuration if needed; build the maintained macOS target and commit the checkpoint closing US1.

## Phase 3: User Story 2 — Type and Run a BASIC Program (Priority: P2)

**Goal**: A physical keyboard types/runs the documented program and the host presents continuous owned completed frames.

**Independent Test**: Assigned firmware plus `10 PRINT "BEEB6502"`, Return, `RUN`, Return yields the expected result and two completed-frame identities without host-driven emulation.

- [X] T004 [US2] Add failing physical-key focus, press/release ordering and documented-program probes; implement `Sources/BeebDemo/main.swift` mapping and focus lifecycle using only `BeebMachine.setKey`, adding core/C/Swift boundary coverage only if the implementation changes that boundary; pass the focused input checks and commit the input checkpoint.
- [X] T005 [US2] Add failing frame/epoch probes; implement epoch-aware completed-frame consumption in `Sources/BeebDemo/main.swift` using existing owned output, record the physical key mapping and acceptance program in `contracts/machine-workflow.md` and `quickstart.md`, then pass focused frame/replay checks and commit the checkpoint closing US2.

## Phase 4: User Story 3 — Control and Recover a Running Session (Priority: P3)

**Goal**: A user runs, pauses, resets and presses BREAK with accessible feedback and preserved Model B configuration after recovery.

**Independent Test**: Controls cross the owner, reset/BREAK cannot present stale frames and one recoverable failure retains profile and firmware state.

- [X] T006 [US3] Add failing separate-control, lifecycle, reset/BREAK epoch and recovery probes in `Tests/ModelBWorkflow/test-application-build.sh`, `Tests/BeebKitTests/BeebMachineTests.swift` and the existing `Tests/C2/test-output-contract.sh` reset contract; implement independent run, pause, reset and BREAK actions plus diagnostic mapping and failure-atomic preservation in `Sources/BeebDemo/main.swift`; pass the focused checks and commit the checkpoint closing US3.

## Phase 5: Slice Acceptance and Completion

**Purpose**: Prove the visual/control slice without claiming the later audio-inclusive M1 gate.

- [X] T007 Run `make test-model-b-workflow`, `make test-machine-target-profile`, `make test-c1`, `make test-c2-xcode`, `swift test`, `swift build`, `xcodebuild -project Beeb6502.xcodeproj -scheme BeebDemo-macOS -configuration Debug build`, `make format-check`, `DOCS_BASE=HEAD make docs-check` and `git diff --check`; commit automated acceptance.
- [ ] T008 Build and launch `BeebDemo-macOS`; perform OS/language import, bank-12 BASIC boot, the physical-key program, two-frame observation, run/pause/reset/BREAK and one recoverable failure; record keyboard and assistive observation in `specs/005-model-b-workflow/evidence/macos-application-observation.md`; commit observed acceptance. Direct boot, frame, control, profile and accessibility observations passed on 2026-08-01; the typed-program result and recoverable import failure still require direct observation.
- [X] T009 Update verified owners in `docs/STATUS.md`, `docs/ARCHITECTURE.md`, `docs/IMPLEMENTATION_CONSTRAINTS.md`, `docs/code/host-boundary.md` and `CHANGELOG.md` only where accepted behavior changed; do not close M1; commit live documentation.
- [ ] T010 After acceptance passes, clear `.specify/feature.json`, move `specs/005-model-b-workflow/` to `specs/completed/005-model-b-workflow/`, refresh `AGENTS.md`, verify numbering, links and `git diff --check`, then commit feature completion.

## Dependencies and Execution Order

T008 and T010 are intentionally gated: they require lawful ROM-backed macOS
observation and must not be closed from automated build evidence alone.

```text
T001 → T002 → T003 → T004 → T005 → T006 → T007 → T008 → T009 → T010
```

The sequence is deliberately linear at the slice level so each user-visible
journey is proven before the next one. Tests and implementation stay in the same
task, preserving red-green delivery without creating red-only checkpoint work.

## Implementation Strategy

Deliver user value in order: firmware/BASIC readiness, then physical keyboard plus
video, then controls/recovery. Reuse existing runtime, C, Swift and C2 contracts;
do not start audio, snapshots, mobile adaptation, B+ behavior, timing, media or
editor work on this branch.
