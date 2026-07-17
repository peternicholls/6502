# Tasks: Phase C2 Bounded Output Contracts

**Input**: Design documents from `/specs/003-bounded-output-contracts/`
**Prerequisites**: plan.md, spec.md, research.md, data-model.md, contracts/, quickstart.md
**Evidence**: Behavior changes begin with focused red tests. Every task is verified and committed in Lore format before the next task; phase checkpoints are explicit.

## Phase 1: Setup (Shared Infrastructure)

**Purpose**: Establish C2 test and documentation surfaces.

- [ ] T001 [P] Add C2 test helper conventions and bounded-output fixture paths in `Tests/C2/testlib.sh`
- [ ] T002 [P] Add C2 Makefile targets and aggregate wiring in `Makefile` and `Tests/C1/test-aggregate-runner.sh`
- [ ] T003 [P] Add the bounded-output conceptual guide skeleton in `docs/code/bounded-output.md`

**Checkpoint**: C2 test, build, and documentation surfaces exist and validate.

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: Lock shared ownership, format, capacity, and status decisions.

- [ ] T004 [P] [US1] Add failing C++ assertions for frame ownership, safe-point publication, and capacity in `Tests/test_main.cpp`
- [ ] T005 [P] [US2] Add failing C++ assertions for audio metadata, demand, and pressure statuses in `Tests/test_main.cpp`
- [ ] T006 [P] [US3] Add failing C++ assertions for diagnostic consistency and emulated-counter sourcing in `Tests/test_main.cpp`
- [ ] T007 Define shared output status, format, capacity, and lifetime types in `Sources/BeebCore/include/beeb/output.hpp`
- [ ] T008 Define the owner-thread output producer/queue boundary and invariants in `Sources/BeebCore/include/beeb/runtime.hpp`
- [ ] T009 [P] Add failing C ABI shape checks for output views, release/copy, and pressure statuses in `Tests/C2/test-output-contract.sh`
- [ ] T010 [P] Add failing Swift tests for owned output values and typed errors in `Tests/BeebKitTests/BeebMachineTests.swift`

**Checkpoint**: Shared contract tests are red for expected missing C2 behavior and design contracts are documented.

## Phase 3: User Story 1 - Consume Completed Video Frames (Priority: P1) 🎯 MVP

**Goal**: Publish complete, bounded, stable-lifetime frames at the C1 safe point.
**Independent Test**: `bash Tests/C2/test-output-contract.sh` plus focused C++ frame tests pass for empty, full, sustained, retained-view, and lifecycle cases.

- [ ] T011 [P] [US1] Add frame empty/full/overflow and monotonic-number tests in `Tests/test_main.cpp`
- [ ] T012 [P] [US1] Add retained-frame and producer-concurrency lifetime tests in `Tests/C2/test-output-lifetime.sh`
- [ ] T013 [P] [US1] Add C ABI frame ownership, nullability, and release/copy tests in `Tests/C2/test-output-contract.sh`
- [ ] T014 [US1] Implement immutable completed-frame storage and deterministic overflow policy in `Sources/BeebCore/src/output.cpp`
- [ ] T015 [US1] Publish completed frames from the C1 safe point in `Sources/BeebCore/src/runtime.cpp`
- [ ] T016 [US1] Expose frame result/view and release/copy operations in `Sources/BeebCore/include/beeb_c.h` and `Sources/BeebCore/src/beeb_c.cpp`
- [ ] T017 [US1] Map frame results into Swift-owned values and typed errors in `Sources/BeebKit/BeebMachine.swift`
- [ ] T018 [US1] Document frame format, ownership, lifetime, overflow, and safe-point rationale in `Sources/BeebCore/include/beeb/output.hpp`, `Sources/BeebCore/include/beeb_c.h`, and `docs/code/bounded-output.md`

**Checkpoint**: US1 is independently demonstrable and its focused tests pass; commit before US2.

## Phase 4: User Story 2 - Produce and Demand Audio (Priority: P2)

**Goal**: Provide bounded ordered samples and explicit demand/pressure reporting without host-clock control.
**Independent Test**: Focused audio tests pass for empty, partial, full, sustained, deterministic ordering, and recovery cases.

- [ ] T019 [P] [US2] Add audio format/order/capacity/underrun/overrun tests in `Tests/test_main.cpp`
- [ ] T020 [P] [US2] Add deterministic sustained audio and bounded-memory measurement in `Tests/C2/test-output-replay.sh`
- [ ] T021 [P] [US2] Add C ABI audio demand, partial-result, and pressure tests in `Tests/C2/test-output-contract.sh`
- [ ] T022 [P] [US2] Add Swift audio ownership, demand, and typed pressure-error tests in `Tests/BeebKitTests/BeebMachineTests.swift`
- [ ] T023 [US2] Implement bounded audio chunk production, consumption, and pressure policy in `Sources/BeebCore/src/output.cpp`
- [ ] T024 [US2] Integrate audio production with the owner runtime and SN76489/audio state in `Sources/BeebCore/src/runtime.cpp`
- [ ] T025 [US2] Expose audio metadata, demand, partial results, and release/copy behavior in `Sources/BeebCore/include/beeb_c.h` and `Sources/BeebCore/src/beeb_c.cpp`
- [ ] T026 [US2] Map audio chunks, demand, and recoverable pressure into Swift values/errors in `Sources/BeebKit/BeebMachine.swift`
- [ ] T027 [US2] Document sample format, demand semantics, bounded policy, and host-clock exclusion in `Sources/BeebCore/include/beeb/output.hpp`, `Sources/BeebCore/include/beeb_c.h`, and `docs/code/bounded-output.md`

**Checkpoint**: US1 and US2 remain independently testable and pass; commit before US3.

## Phase 5: User Story 3 - Observe Runtime Diagnostics (Priority: P3)

**Goal**: Make progress, availability, demand, and recoverable pressure observable across boundaries.
**Independent Test**: Replay, race, C++, and Swift diagnostic tests pass with identical transitions.

- [ ] T028 [P] [US3] Add diagnostic snapshot consistency and counter-transition tests in `Tests/test_main.cpp`
- [ ] T029 [P] [US3] Add concurrent producer/consumer and shutdown/lifecycle tests in `Tests/C2/test-output-races.sh`
- [ ] T030 [P] [US3] Add Swift diagnostic mapping and recovery tests in `Tests/BeebKitTests/BeebMachineTests.swift`
- [ ] T031 [US3] Implement diagnostic snapshots and pressure counters in `Sources/BeebCore/include/beeb/output.hpp` and `Sources/BeebCore/src/output.cpp`
- [ ] T032 [US3] Expose diagnostics through the structured C boundary in `Sources/BeebCore/include/beeb_c.h` and `Sources/BeebCore/src/beeb_c.cpp`
- [ ] T033 [US3] Map diagnostics and recoverable statuses into Swift values/errors in `Sources/BeebKit/BeebMachine.swift`
- [ ] T034 [US3] Document diagnostic consistency and recovery guidance in `Sources/BeebCore/include/beeb_c.h`, `Sources/BeebCore/include/beeb/output.hpp`, and `docs/code/bounded-output.md`

**Checkpoint**: All C2 stories are independently testable and diagnostics are reproducible; commit before polish.

## Phase 6: Polish & Cross-Cutting Concerns

- [ ] T035 [P] Add C2 aggregate failure propagation and documentation-negative coverage in `Tests/C2/test-documentation.sh` and `Tests/C2/test-output-contract.sh`
- [ ] T036 [P] Update runtime ownership, host boundary, and timing guides in `docs/code/runtime-ownership.md`, `docs/code/host-boundary.md`, and `docs/code/timing-model.md`
- [ ] T037 [P] Update verified C2 status and roadmap evidence in `docs/STATUS.md` and `docs/CORE_ROADMAP.md` after measurements exist
- [ ] T038 Update release notes/version sources for any public contract change in `CHANGELOG.md`, `VERSION`, and `Sources/BeebCore/include/beeb/version.h`
- [ ] T039 Run the reproducible C2 quickstart and record bounded-production evidence under `.build/c2/` using `specs/003-bounded-output-contracts/quickstart.md`
- [ ] T040 Run full validation with `make test`, `make sanitize`, `swift test`, `swift build`, `make docs-check`, `bash Tests/C2/test-documentation.sh`, and `git diff --check`
- [ ] T041 Generate browsable documentation and validate changed public/internal documentation debt with `scripts/build-docs.sh` and `Tests/C0/test-documentation.sh`

**Checkpoint**: C2 completion is committed with all claims backed by evidence; unsupported local TSan remains N/A if applicable.

## Dependencies & Execution Order

- Setup (Phase 1) precedes Foundational (Phase 2), which blocks all stories.
- US1 is the P1 MVP and depends only on Foundational.
- US2 depends on Foundational and shared output types, while preserving US1.
- US3 consumes the statuses/counters from US1/US2 and then remains independently testable.
- Polish depends on all selected stories.
- T001–T003, T004–T006, and T009–T010 are parallel opportunities; tests marked `[P]` can be authored in parallel before implementation.

## Implementation Strategy

Deliver Foundational → US1 frame MVP → US2 audio → US3 diagnostics → measured/documented full validation. Commit each verified task and phase checkpoint in Lore format.
