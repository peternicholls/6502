# Tasks: Running Model B Workflow

**Input**: `spec.md`, `plan.md`, `research.md`, `data-model.md`, `contracts/` and `quickstart.md`

**Evidence**: Each behavior task begins with focused red evidence and closes with its affected checks. The full audio-inclusive M1 gate remains outside this feature.

**Git**: Commit every verified task before beginning the next. A final task commit may close a phase only when its Lore message says so.

## Phase 1: Focused Workflow Harness

**Purpose**: Establish one small aggregate for this vertical slice.

- [ ] T001 Create `Tests/ModelBWorkflow/testlib.sh`, `Tests/ModelBWorkflow/test-aggregate-runner.sh` and the `test-model-b-workflow` target in `Makefile`; verify the empty aggregate and tooling gates, then commit the harness checkpoint.
- [ ] T002 Add `Tests/ModelBWorkflow/test-ci-contract.sh`, add `test-model-b-workflow` to the `apple-package` job in `.github/workflows/ci.yml`, and verify both source contract and focused hosted gate fail first; then commit the CI checkpoint.

**Checkpoint**: T001 passes before user-story implementation begins.

## Phase 2: Shared Host Workflow Foundation

**Purpose**: Define host-owned assignment, focus, diagnostic and output-epoch state without introducing a second runtime owner.

- [ ] T003 Add failing role-validation, failed-candidate-preservation and stale-output tests in `Tests/BeebKitTests/BeebMachineTests.swift`; observe failure and commit the red-boundary checkpoint.
- [ ] T004 Add failing application contract probes for firmware roles, accessible status, input focus and separate controls in `Tests/ModelBWorkflow/test-application-build.sh`; observe failure and commit the red-host checkpoint.
- [ ] T005 Implement documented host-owned firmware-assignment and diagnostic state in `Sources/BeebDemo/main.swift`, changing `Sources/BeebKit/BeebMachine.swift` only for a proved typed role/status gap; pass T003/T004 subsets and commit the foundation checkpoint.
- [ ] T006 Update changed public contract comments in `Sources/BeebKit/BeebMachine.swift`, `Sources/BeebKit/Documentation.docc/BeebKit.md` and `docs/code/host-boundary.md`; run focused documentation checks and `git diff --check`, then commit the foundation documentation checkpoint.

## Phase 3: User Story 1 — Import Firmware and Reach BASIC (Priority: P1)

**Goal**: A macOS user assigns remembered Model B OS/language ROMs and reaches BASIC with safe recovery.

**Independent Test**: A clean session accepts lawful fixture-equivalent role values, exposes active assignments and reaches BASIC-ready output; a rejected candidate preserves the working session.

- [ ] T007 [US1] Add red Model B OS/language role, size/bank and failure-atomic assignment fixtures in `Tests/BeebKitTests/BeebMachineTests.swift` and `Tests/test_main.cpp`; observe failure and commit the red firmware-boundary checkpoint.
- [ ] T008 [P] [US1] Add red import, remembered-assignment and reselect-recovery probes in `Tests/ModelBWorkflow/test-firmware-contract.sh`; use fixture-safe metadata, observe failure and commit the red host-firmware checkpoint.
- [ ] T009 [US1] Implement typed Model B firmware role assignment in `Sources/BeebKit/BeebMachine.swift` and only if a boundary gap is proven in `Sources/BeebCore/include/beeb_c.h` plus `Sources/BeebCore/src/beeb_c.cpp`; pass T007 and affected C/C++ checks, then commit the boundary checkpoint.
- [ ] T010 [US1] Implement read-only security-scoped bookmark persistence, stale refresh, balanced access and private byte loading in `Sources/BeebDemo/main.swift`; prove the current `Beeb6502.xcodeproj/project.pbxproj` configuration needs no change, pass T008 and the changed macOS build, then commit the host firmware checkpoint.
- [ ] T011 [US1] Implement accessible OS/language assignment presentation and BASIC-ready reset flow in `Sources/BeebDemo/main.swift`; pass the US1 aggregate subset and commit the checkpoint explicitly closing US1.

## Phase 4: User Story 2 — Type and Run a BASIC Program (Priority: P2)

**Goal**: A physical keyboard types/runs the documented program and the host presents continuous owned completed frames.

**Independent Test**: Assigned fixture-equivalent firmware plus the documented key sequence yields the expected result and two completed-frame identities without host-driven emulation.

- [ ] T012 [US2] Add red keyboard mapping, press/release ordering and owner-serialization coverage in `Tests/BeebKitTests/BeebMachineTests.swift` and `Tests/test_main.cpp`; observe failure and commit the red input-boundary checkpoint.
- [ ] T013 [P] [US2] Add red physical-key focus, documented-program and continuous-frame probes in `Tests/ModelBWorkflow/test-application-build.sh`; observe failure and commit the red input-presentation checkpoint.
- [ ] T014 [US2] Implement documented Model B physical-key mapping and focus lifecycle in `Sources/BeebDemo/main.swift` using only `BeebMachine.setKey`; pass T012/T013 subsets and commit the input checkpoint.
- [ ] T015 [US2] Implement epoch-aware completed-frame consumption in `Sources/BeebDemo/main.swift`, adding `Sources/BeebKit/BeebMachine.swift` support only if an owned observation is missing; pass focused frame/replay checks and commit the presentation checkpoint.
- [ ] T016 [US2] Record the physical key press/release mapping for `10 PRINT "BEEB6502"`, Return, `RUN`, Return in `specs/005-model-b-workflow/contracts/machine-workflow.md` and `specs/005-model-b-workflow/quickstart.md`; run focused documentation/build probes and commit the checkpoint explicitly closing US2.

## Phase 5: User Story 3 — Control and Recover a Running Session (Priority: P3)

**Goal**: A user runs, pauses, resets and presses BREAK with accessible feedback and preserved Model B configuration after recovery.

**Independent Test**: Controls cross the owner, reset/BREAK cannot present stale frames and one recoverable failure retains profile and firmware state.

- [ ] T017 [US3] Add red lifecycle, reset/BREAK epoch and repeated-control recovery tests in `Tests/BeebKitTests/BeebMachineTests.swift` and `Tests/C2/test-output-contract.sh`; observe failure and commit the red control-boundary checkpoint.
- [ ] T018 [P] [US3] Add red separate-control, keyboard-operability, accessibility-status and failure-recovery probes in `Tests/ModelBWorkflow/test-application-build.sh`; observe failure and commit the red control-host checkpoint.
- [ ] T019 [US3] Implement independent run, pause, reset and BREAK actions plus diagnostic mapping in `Sources/BeebDemo/main.swift`; use existing serialized `BeebMachine` operations, pass T017/T018 subsets and commit the controls checkpoint.
- [ ] T020 [US3] Preserve configured Model B firmware and identity through recoverable host failures in `Sources/BeebDemo/main.swift` and document changed recovery guarantees in `Sources/BeebKit/BeebMachine.swift`; pass focused failure-atomicity checks and commit the checkpoint explicitly closing US3.

## Phase 6: Slice Acceptance and Completion

**Purpose**: Prove the visual/control slice without claiming the later audio-inclusive M1 gate.

- [ ] T021 Finish focused groups in `Tests/ModelBWorkflow/` and `Makefile`; run `make test-model-b-workflow`, affected target-profile/C1/C2 regressions, `swift test`, the changed macOS Xcode build, `make format-check`, `DOCS_BASE=HEAD make docs-check` and `git diff --check`, then commit automated acceptance.
- [ ] T022 Build and launch `BeebDemo-macOS`; perform the OS/language import, BASIC boot, physical-key program, two-frame, run/pause/reset/BREAK and recoverable-failure journey; record keyboard and assistive observation in `specs/005-model-b-workflow/evidence/macos-application-observation.md`, then commit observed acceptance.
- [ ] T023 Update verified owners in `docs/STATUS.md`, `docs/ARCHITECTURE.md`, `docs/IMPLEMENTATION_CONSTRAINTS.md`, `docs/code/host-boundary.md` and `CHANGELOG.md` only where accepted behavior changed; do not close M1, then commit live documentation.
- [ ] T024 Clear `.specify/feature.json`, move `specs/005-model-b-workflow/` to `specs/completed/005-model-b-workflow/`, refresh `AGENTS.md`, verify numbering, links and `git diff --check`, then commit feature completion.

## Dependencies and Execution Order

```text
T001 → T003/T004 → T005 → T006
T005 → T007/T008 → T009/T010 → T011
T011 → T012/T013 → T014 → T015 → T016
T016 → T017/T018 → T019 → T020 → T021 → T022 → T023 → T024
```

T002 runs after T001 and must complete before user-story implementation begins so the focused aggregate is enforced on the maintained Apple CI path. T007/T008, T012/T013 and T017/T018 are parallel opportunities after their foundation, but their dependent tasks remain sequential.

## Implementation Strategy

Deliver user value in order: firmware/BASIC readiness, then physical keyboard plus video, then controls/recovery. Do not start the audio feature or M1 gate on this branch. The focused aggregate keeps iteration fast; Phase 6 supplies the required wider regression and direct application evidence.
