# Tasks: Phase C1 Runtime Ownership

**Input**: Design documents from `/specs/002-runtime-ownership/`

**Evidence**: Every behavior task starts with a test observed failing for the expected reason. Each task is verified and committed in Lore format before the next task begins. The final task commit in each phase also records phase completion; no empty duplicate checkpoint is created.

## Phase 1: Contract Scaffolding

- [x] T001 Create reusable C1 test helpers and isolated build paths in `Tests/C1/testlib.sh` and `.gitignore`. Verify shell syntax and a self-test.
- [x] T002 Add failing state/command matrix and structured-status tests in `Tests/test_main.cpp` and `Tests/C1/test-runtime-contract.sh`. Verify failure is only missing C1 API.
- [x] T003 [P] Add failing deterministic ledger/replay acceptance in `Tests/C1/test-runtime-replay.sh` and `Tests/test_main.cpp`. Verify the focused missing-behavior failure.
- [x] T004 [P] Add failing mixed-command/shutdown/ThreadSanitizer acceptance in `Tests/C1/test-runtime-races.sh` plus Make targets `thread-sanitize` and `test-c1`. Verify focused failure; commit the complete verified Phase 1 once.

## Phase 2: Foundational Owner and Results

- [x] T005 Define `RuntimeState`, `RuntimeStatus`, safe-point, command/result values, and contracts in `Sources/BeebCore/include/beeb/runtime.hpp`. Verify declaration tests compile.
- [x] T006 Implement owner-thread skeleton, capacity-64 FIFO, sequencing, completion, re-entrancy rejection, and join in `Sources/BeebCore/src/runtime.cpp`. Verify queue lifecycle tests under normal/sanitizer builds.
- [x] T007 Move one `BBCMicro` behind `MachineRuntime`, retaining direct low-level construction only for unit tests, in `Sources/BeebCore/include/beeb/machine.hpp` and `Sources/BeebCore/src/runtime.cpp`. Verify ownership and existing core tests.
- [x] T008 Implement completed-instruction/device-tick `SafePoint` identity and fixed 2,048-cycle execution-slice ledger in `Sources/BeebCore/src/runtime.cpp`. Verify safe-point/ledger tests; commit the complete verified Phase 2 once.

## Phase 3: User Story 1 — Own Sustained Execution (P1)

**Independent Test**: Concurrent start/pause commands serialize through one owner and exact ledger replay matches ten times.

- [x] T009 [US1] Add/confirm failing create-paused, start, pause, idempotence, invalid-transition, and latency tests in `Tests/test_main.cpp`.
- [x] T010 [US1] Implement paused/running transitions and execution-loop arbitration in `Sources/BeebCore/src/runtime.cpp`. Verify US1 tests and `make sanitize`.
- [x] T011 [US1] Implement exact accepted-command/execution-slice replay test hooks in `Tests/test_main.cpp` and `.build/c1/`, with no production persisted format. Verify `Tests/C1/test-runtime-replay.sh`.
- [x] T012 [US1] Document public and private/internal C++ runtime types, lifecycle/safe-point contracts, responsibility boundaries, invariants, and state graph in `Sources/BeebCore/include/beeb/runtime.hpp`, `Sources/BeebCore/src/runtime.cpp`, and `docs/code/runtime-ownership.md`. Verify representative internal and public pages in `make docs-check`; commit the complete verified Phase 3 once.

## Phase 4: User Story 2 — Serialize Runtime Transactions (P2)

**Independent Test**: Reset, loads, inputs, and observations around run/pause complete FIFO at safe points with owned values and atomic failure.

- [x] T013 [US2] Add failing reset/load/input/query FIFO, atomic-invalid-input, and no-auto-resume tests in `Tests/test_main.cpp`.
- [x] T014 [US2] Route reset, OS/sideways ROM, and disc transactions with copied payloads through `Sources/BeebCore/src/runtime.cpp`. Verify focused and existing media tests.
- [x] T015 [US2] Route keyboard/BREAK mutations through the FIFO in `Sources/BeebCore/src/runtime.cpp`. Verify ordered input/reset tests and sanitizer.
- [x] T016 [US2] Route CPU state, frame, and audio observations through owner commands and return owned values in `Sources/BeebCore/include/beeb/runtime.hpp` and `Sources/BeebCore/src/runtime.cpp`. Verify consistency/lifetime and C0 frame tests.
- [x] T017 [US2] Complete capacity back-pressure, accepted-before-shutdown drain, new-call rejection, waiter wakeup, and join in `Sources/BeebCore/src/runtime.cpp`. Verify queue/overlap/timeout/10,000-command TSan stress.
- [x] T018 [US2] Update ownership, transaction matrix, shutdown, and future bus-cycle constraints in `docs/ARCHITECTURE.md`, `docs/code/runtime-ownership.md`, and `docs/code/timing-model.md`. Verify docs gates; commit the complete verified Phase 4 once.

## Phase 5: User Story 3 — Recover Across Public Boundaries (P3)

**Independent Test**: Every fallible C operation returns its own stable status and Swift preserves typed recovery, including fault/reset and shutdown overlap.

- [x] T019 [US3] Add failing C 0.2 status, out-parameter, nullability, stale-diagnostic, fault, and destroy-overlap tests in `Tests/test_main.cpp`.
- [x] T020 [US3] Replace 0.1 sentinel declarations with structured 0.2 contracts in `Sources/BeebCore/include/beeb_c.h`, including migration notes. Verify C11/C++ header compilation.
- [x] T021 [US3] Implement exception-safe C adapters and operation-scoped diagnostics over `MachineRuntime` in `Sources/BeebCore/src/beeb_c.cpp`. Verify C tests, `make test`, and sanitizer.
- [x] T022 [US3] Migrate `Tools/beeb-headless/main.cpp`, `Tools/beeb-evidence/main.cpp`, and examples to 0.2. Verify builds, C0 evidence, and no old API usage via `rg`.
- [x] T023 [US3] Add failing Swift state/start/pause/status-category/concurrency/recovery tests in `Tests/BeebKitTests/BeebMachineTests.swift`. Verify focused red failure.
- [x] T024 [US3] Migrate `BeebMachine` ownership and operations to structured results in `Sources/BeebKit/BeebMachine.swift`, removing redundant direct-state `NSLock` serialization. Verify Swift tests/build.
- [x] T025 [US3] Document typed errors, concurrency, lifecycle, ownership, recovery, and every changed private/internal Swift or C-boundary named abstraction in declarations, `Sources/BeebKit/Documentation.docc/BeebKit.md`, and `docs/code/host-boundary.md`. Verify representative internal and public pages in `make docs-check`; commit the complete verified Phase 5 once.

## Phase 6: C1 Exit and Governance

- [x] T026 [P] Finish aggregate C1 and negative documentation/runtime coverage in `Makefile` and `Tests/C1/*.sh`. Verify `make test-c1` runs all groups without masking later failures.
- [ ] T027 Run the full clean-tree candidate gate and record exact evidence in `docs/STATUS.md`. Verify `make test`, `make sanitize`, `make thread-sanitize`, `swift test`, `swift build`, `make verify-c0`, `make test-c1`, and `make docs-check`.
- [ ] T028 Synchronize the intentional pre-1.0 boundary release at `0.2.0` in `VERSION`, `Sources/BeebCore/include/beeb/version.h`, `Sources/BeebKit/BeebVersion.swift`, `CHANGELOG.md`, and release links. Verify `make check-version` and `git diff --check`.
- [ ] T029 Mark C1 Complete and unblock C2/C3 only with exit evidence in `docs/CORE_ROADMAP.md`, `docs/STATUS.md`, `docs/ARCHITECTURE.md`, and applicable product trace. Verify links and roadmap/status consistency.
- [ ] T030 Rerun every `quickstart.md` command on the completion candidate and record exact results only if the evidence record needs it. Verify all gates green and no generated files tracked; commit the complete verified Phase 6/C1 once, then confirm the committed revision is clean without creating an empty duplicate commit.

## Dependencies and Coverage

`T001 -> (T002, T003, T004) -> T005 -> T006 -> T007 -> T008 -> US1 -> US2 -> US3 -> exit`. T003/T004 may run in parallel after T001; governance is sequential. Each `[P]` task still receives its own verified commit and stages no other task.

- US1 / FR-001–010, FR-017: T002–T012.
- US2 / FR-005–012, FR-016–019: T013–T018.
- US3 / FR-012–016, FR-019–020: T019–T025.
- SC-001–006 and phase governance: T026–T030.
