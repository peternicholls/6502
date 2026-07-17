# Tasks: Phase C2 Bounded Output Contracts and Xcode Project

**Input**: Design documents from `/specs/003-bounded-output-contracts/`
**Prerequisites**: plan.md, spec.md, research.md, data-model.md, contracts/, quickstart.md
**Evidence**: Behavior changes begin with focused red tests. Every task is verified and committed in Lore format before the next task; phase checkpoints are explicit.

## Phase 1: Setup (Shared Infrastructure)

**Purpose**: Establish independent C2 test, evidence, and documentation surfaces.

- [x] T001 [P] Add C2 test helper conventions and bounded-output fixture paths in `Tests/C2/testlib.sh`
- [x] T002 [P] Add a dedicated C2 aggregate failure-propagating runner in `Tests/C2/test-aggregate-runner.sh`
- [x] T003 Wire `test-c2` and C2 evidence targets without changing C1 aggregation in `Makefile`
- [x] T004 [P] Add the bounded-output conceptual guide skeleton in `docs/code/bounded-output.md`

**Checkpoint**: C2 has independent test/build/documentation surfaces and does not alter completed C1 evidence semantics.

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: Lock shared owned-value, capacity, format, status, and diagnostics shapes before story behavior.

- [x] T005 Add a failing public-shape contract for owned results, capacities 3/4,096, 48 kHz mono Float32, and diagnostic counters in `Tests/C2/test-public-boundaries.sh`
- [x] T006 Define documented shared output formats, statuses, capacities, counters, and owned result types in `Sources/BeebCore/include/beeb/output.hpp`
- [x] T007 Define the owner-thread producer/queue boundary and no-borrow/no-callback invariants in `Sources/BeebCore/include/beeb/runtime.hpp`

**Checkpoint**: Shared declarations compile only after the expected red boundary contract is satisfied; no producer storage is publicly borrowed.

## Phase 3: User Story 1 - Consume Completed Video Frames (Priority: P1) 🎯 MVP

**Goal**: Publish owned RGBA frames through a capacity-three FIFO at the C1 safe point.
**Independent Test**: Focused C++, C, Swift, and lifetime tests pass for empty, retained, full, drop-oldest, sustained, lifecycle, and exact accounting cases.

- [x] T008 [P] [US1] Add failing frame FIFO, oldest-first dequeue, drop-oldest, monotonic-number, and accounting tests in `Tests/test_main.cpp`
- [x] T009 [P] [US1] Add failing retained-frame and concurrent producer lifetime tests in `Tests/C2/test-output-lifetime.sh`
- [x] T010 [P] [US1] Add failing C caller-owned frame, nullability, release, and no-alias tests in `Tests/C2/test-output-contract.sh`
- [x] T011 [P] [US1] Add failing Swift independently owned frame and typed lifecycle/empty tests in `Tests/BeebKitTests/BeebMachineTests.swift`
- [x] T012 [US1] Implement immutable owned frame values and capacity-three drop-oldest FIFO in `Sources/BeebCore/src/output.cpp`
- [x] T013 [US1] Publish completed frames from the C1 instruction/device safe point in `Sources/BeebCore/src/runtime.cpp`
- [x] T014 [US1] Expose caller-owned frame dequeue/release operations in `Sources/BeebCore/include/beeb_c.h` and `Sources/BeebCore/src/beeb_c.cpp`
- [x] T015 [US1] Map dequeued C frames into independently owned Swift values in `Sources/BeebKit/BeebMachine.swift`
- [x] T016 [US1] Document RGBA format, ownership, capacity three, drop-oldest behavior, counters, and safe-point rationale in `Sources/BeebCore/include/beeb/output.hpp`, `Sources/BeebCore/include/beeb_c.h`, and `docs/code/bounded-output.md`

**Checkpoint**: US1 is independently demonstrable and verified in a Lore commit before US2.

## Phase 4: User Story 2 - Produce and Demand Audio (Priority: P2)

**Goal**: Produce mono Float32 at 48 kHz through a 4,096-sample FIFO with a 2,048 target and exact demand/pressure accounting.
**Independent Test**: Focused tests pass for FIFO ordering, empty, partial, full, drop-oldest, exact shortfall, sustained output, and owned-copy behavior.

- [x] T017 [P] [US2] Add failing 48 kHz mono Float32, 4,096-capacity, 2,048-target, FIFO, underrun, overrun, and accounting tests in `Tests/test_main.cpp`
- [x] T018 [P] [US2] Add failing deterministic sustained audio and bounded-depth replay tests in `Tests/C2/test-output-replay.sh`
- [x] T019 [P] [US2] Add failing C caller-buffer, copied-count, exact-shortfall, demand, and pressure tests in `Tests/C2/test-output-contract.sh`
- [x] T020 [P] [US2] Add failing Swift owned audio, demand, and typed pressure tests in `Tests/BeebKitTests/BeebMachineTests.swift`
- [x] T021 [US2] Implement the 4,096-sample FIFO, 2,048 target, FIFO drain, and drop-oldest pressure accounting in `Sources/BeebCore/src/output.cpp`
- [x] T022 [US2] Integrate deterministic 48 kHz audio production with owner slices and SN76489 state in `Sources/BeebCore/src/runtime.cpp`
- [x] T023 [US2] Expose caller-buffer audio drain, copied count, shortfall, demand, and pressure in `Sources/BeebCore/include/beeb_c.h` and `Sources/BeebCore/src/beeb_c.cpp`
- [x] T024 [US2] Map audio drains, demand, and recoverable pressure into Swift-owned values/errors in `Sources/BeebKit/BeebMachine.swift`
- [x] T025 [US2] Document sample format, capacity, target, FIFO/overflow/underrun rules, accounting, and host-clock exclusion in `Sources/BeebCore/include/beeb/output.hpp`, `Sources/BeebCore/include/beeb_c.h`, and `docs/code/bounded-output.md`

**Checkpoint**: US1 and US2 remain independently testable and verified in a Lore commit before US3.

## Phase 5: User Story 3 - Observe Runtime Diagnostics (Priority: P3)

**Goal**: Expose consistent progress, capacities, demand, pressure, and host-observed emulation-rate diagnostics.
**Independent Test**: C++, C, Swift, replay, and race tests pass with exact counters and rate ratio within 0.1%, without changing core state.

- [x] T026 [P] [US3] Add failing diagnostic consistency, depth/capacity, demand, pressure-counter, and cycle-observation tests in `Tests/test_main.cpp`
- [x] T027 [P] [US3] Add failing concurrent producer/consumer and shutdown/lifecycle diagnostic tests in `Tests/C2/test-output-races.sh`
- [x] T028 [P] [US3] Add failing C diagnostic snapshot and synthetic emulation-rate helper tests in `Tests/C2/test-output-contract.sh`
- [x] T029 [P] [US3] Add failing Swift diagnostic mapping, recovery, and 0.1%-tolerance rate tests in `Tests/BeebKitTests/BeebMachineTests.swift`
- [x] T030 [US3] Implement consistent diagnostic snapshots and exact pressure counters in `Sources/BeebCore/include/beeb/output.hpp` and `Sources/BeebCore/src/output.cpp`
- [x] T031 [US3] Expose total cycles, depths, capacities, demand, and counters through the C boundary in `Sources/BeebCore/include/beeb_c.h` and `Sources/BeebCore/src/beeb_c.cpp`
- [x] T032 [US3] Implement the pure C host-observation emulation-rate helper without core host-time state in `Sources/BeebCore/include/beeb_c.h` and `Sources/BeebCore/src/beeb_c.cpp`
- [x] T033 [US3] Map diagnostic snapshots and the pure rate calculation into Swift values in `Sources/BeebKit/BeebMachine.swift`
- [x] T034 [US3] Document consistency points, counter equations, rate units/tolerance, and recovery guidance in `Sources/BeebCore/include/beeb_c.h`, `Sources/BeebCore/include/beeb/output.hpp`, and `docs/code/bounded-output.md`

**Checkpoint**: All C2 output stories are independently testable and verified in a Lore commit before Xcode elevation.

## Phase 6: User Story 4 - Elevate to an Xcode Project (Priority: P4)

**Goal**: Commit a clean-checkout Xcode project with shared macOS, iOS Simulator, and test schemes over existing sources/products.
**Independent Test**: `Tests/C2/test-xcode-project.sh` lists all shared schemes, builds macOS and generic iOS Simulator destinations, runs tests, rejects user/absolute-path state, and then proves Swift Package and Makefile independence.

- [x] T035 [US4] Add a failing clean-checkout Xcode project/scheme/source-duplication contract in `Tests/C2/test-xcode-project.sh`
- [x] T036 [US4] Create the source-relative project and macOS/iOS app target metadata in `Beeb6502.xcodeproj/project.pbxproj`
- [ ] T037 [US4] Add shared macOS app, iOS Simulator app, and package-test schemes in `Beeb6502.xcodeproj/xcshareddata/xcschemes/BeebDemo-macOS.xcscheme`, `Beeb6502.xcodeproj/xcshareddata/xcschemes/BeebDemo-iOS.xcscheme`, and `Beeb6502.xcodeproj/xcshareddata/xcschemes/Beeb6502-Tests.xcscheme`
- [ ] T038 [US4] Exclude user-specific Xcode state and derived products while retaining shared project metadata in `.gitignore`
- [ ] T039 [US4] Add macOS build, generic iOS Simulator build, and test-scheme gates to `.github/workflows/ci.yml`
- [ ] T040 [US4] Replace package-only Apple setup with Xcode-project-first and independent Swift Package guidance in `README.md` and `CONTRIBUTING.md`

**Checkpoint**: The Xcode project is a verified maintained delivery surface and the Makefile/Swift Package paths remain independently green.

## Phase 7: Polish & Cross-Cutting Concerns

- [ ] T041 [P] Add C2 aggregate failure propagation, documentation-negative, and Xcode-metadata hygiene coverage in `Tests/C2/test-aggregate-runner.sh`, `Tests/C2/test-documentation.sh`, and `Tests/C2/test-xcode-project.sh`
- [ ] T042 [P] Update runtime ownership, host boundary, timing, and architecture guides in `docs/code/runtime-ownership.md`, `docs/code/host-boundary.md`, `docs/code/timing-model.md`, and `docs/ARCHITECTURE.md`
- [ ] T043 [P] Update verified C2 and Xcode-project evidence only after measurements pass in `docs/STATUS.md`, `docs/CORE_ROADMAP.md`, and `docs/product/LEGACY_DECISIONS.md`
- [ ] T044 Update release notes and synchronized version sources for public contract changes in `CHANGELOG.md`, `VERSION`, and `Sources/BeebCore/include/beeb/version.h`
- [ ] T045 Run 60 emulated seconds after warm-up plus 10,000-item stress, exact accounting, 16 MiB RSS tolerance, and 0.1% rate tolerance in `Tests/C2/test-output-measurement.sh` using `specs/003-bounded-output-contracts/quickstart.md`
- [ ] T046 Generate and validate browsable public/internal documentation with `scripts/build-docs.sh`, `Tests/C0/test-documentation.sh`, and `Tests/C2/test-documentation.sh`
- [ ] T047 Run final validation with `make test`, `make sanitize`, `make thread-sanitize`, `make format-check`, `make test-c1`, `make test-c2`, `swift test`, `swift build`, all `xcodebuild` commands in `specs/003-bounded-output-contracts/quickstart.md`, `make docs-check`, and `git diff --check`

**Checkpoint**: C2 completion is committed with exact evidence; unsupported local TSan is N/A but supported CI execution is required.

## Dependencies & Execution Order

- Setup precedes Foundational; Foundational blocks all story implementation.
- US1 is the P1 MVP and depends only on Foundational.
- US2 depends on shared output types but not on US1 presentation behavior.
- US3 consumes the exact queue/status counters from US1 and US2.
- US4 depends on stable package products and may begin after Foundational, but its final build evidence runs after US1–US3 integration.
- Polish depends on all four stories.
- T001, T002, and T004 are parallel; tests marked `[P]` within each story use different files and can be authored before implementation.

## Implementation Strategy

Deliver Foundational → US1 frame MVP → US2 audio → US3 diagnostics → US4 Xcode project → measured/documented full validation. Commit every verified task and every phase checkpoint in Lore format.
