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
- [x] T027 Run the full clean-tree candidate gate and record exact evidence in `docs/STATUS.md`. Verify `make test`, `make sanitize`, `make thread-sanitize`, `swift test`, `swift build`, `make verify-c0`, `make test-c1`, and `make docs-check`.
- [x] T028 Synchronize the intentional pre-1.0 boundary release at `0.2.0` in `VERSION`, `Sources/BeebCore/include/beeb/version.h`, `Sources/BeebKit/BeebVersion.swift`, `CHANGELOG.md`, and release links. Verify `make check-version` and `git diff --check`.
- [x] T029 Mark C1 Complete and unblock C2/C3 only with exit evidence in `docs/CORE_ROADMAP.md`, `docs/STATUS.md`, `docs/ARCHITECTURE.md`, and applicable product trace. Verify links and roadmap/status consistency.
- [x] T030 Rerun every `quickstart.md` command on the completion candidate and record exact results only if the evidence record needs it. Verify all gates green and no generated files tracked; commit the complete verified Phase 6/C1 once, then confirm the committed revision is clean without creating an empty duplicate commit.

## Dependencies and Coverage

`T001 -> (T002, T003, T004) -> T005 -> T006 -> T007 -> T008 -> US1 -> US2 -> US3 -> exit`. T003/T004 may run in parallel after T001; governance is sequential. Each `[P]` task still receives its own verified commit and stages no other task.

- US1 / FR-001–010, FR-017: T002–T012.
- US2 / FR-005–012, FR-016–019: T013–T018.
- US3 / FR-012–016, FR-019–020: T019–T025.
- SC-001–006 and phase governance: T026–T030.

---

## Remediation Ledger: Code and Inline-Documentation Audits

**Inputs**: `code-audit-catalogue.md`,
`inline-documentation-audit-catalogue.md`, and the local execution plan
`.omx/plans/c1-code-review-remediation.md`.

**Status rule**: T031–T076 remain unchecked until the task's focused
verification succeeds and its Lore commit is created. Existing remediation
commits are implementation inputs, not automatic completion evidence. Preserve
the unrelated in-progress `scripts/run-klaus.sh` work and stage only the active
task.

**Execution rule**: Commit every task before beginning another task. The final
task in each phase records phase completion in its Lore message and serves as
the phase checkpoint; do not create an empty duplicate commit.

## Phase 7: Destructive-Safety Preconditions

**Purpose**: Make verification tooling safe and evidence inputs immutable before
the broader remediation repeatedly invokes those tools.

- [x] T031 [P] Add destructive-path negative fixtures for repository root, home, `/`, empty, symlink-escaped, and pre-existing unowned output directories in `Tests/C0/test-documentation.sh`, `Tests/C0/test-baseline-verifier.sh`, and `Tests/C0/test-measurement-record.sh`; prove sentinel files survive each expected red failure (AUD-002).
- [x] T032 Complete canonical path and tool-owned cleanup protection in `scripts/build-docs.sh`, `scripts/verify-c0.sh`, `scripts/verify-c0-references.sh`, and `scripts/measure-c0.sh`; verify T031, `bash -n` for every shell script, `make docs-check DOCS_PROFILE=portable`, and `make verify-c0 C0_PROFILE=portable` before the Lore commit (AUD-002).
- [x] T033 [P] Add offline checksum-mismatch, cleanup, and concurrent-invocation fixtures for Klaus evidence in `Tests/C0/test-fixture-evidence.sh`, using a local fake downloader/input so the negative tests require no network (AUD-017).
- [x] T034 Finish immutable Klaus handling in `scripts/run-klaus.sh` by pinning the reviewed revision and SHA-256, using a unique guarded temporary directory, and retaining no shared filename; verify T033 plus one network-backed success-trap run, `bash -n scripts/run-klaus.sh`, and `git diff --check`, then commit Phase 7 complete (AUD-017).

**Checkpoint**: Verification tooling cannot delete unowned data, and Klaus
evidence is immutable, collision-safe, and reproducible.

---

## Phase 8: User Story 1 Remediation — Coherent Sustained Execution (P1)

**Goal**: Restore one coherent last-completed whole-machine boundary for every
execution failure and prove deterministic sustained/replay behavior.

**Independent Test**: Bounded run, run-to-frame, and sustained execution each
perform a legal RAM/device mutation before illegal opcode `$02`; CPU, RAM,
devices, fault detail, ledger cycles, and replay digest agree on the retained
last-completed boundary across ten repetitions.

- [x] T035 [US1] Add failing late-fault regressions for bounded run, run-to-frame, and sustained execution plus a closed looping sustained fixture in `Tests/test_main.cpp`; assert CPU/RAM/device consistency, fault state, `actualCycles`, reset recovery, and 50 repeated lifecycle runs before changing implementation (AUD-001, AUD-004).
- [x] T036 [US1] Complete atomic illegal-opcode, trace-observer, and execution-fault restoration in `Sources/BeebCore/src/cpu6502.cpp`, `Sources/BeebCore/src/machine.cpp`, and `Sources/BeebCore/src/runtime.cpp`, extending the machine checkpoint to every mutable device state exercised by T035; verify T035, `make test`, and `make sanitize` before commit (AUD-001, AUD-013).
- [x] T037 [US1] Add a private test-only whole-machine digest covering relevant RAM and device state in `Sources/BeebCore/include/beeb/machine.hpp`, `Sources/BeebCore/src/machine.cpp`, and `Tests/test_main.cpp`, without adding a persisted format or C/Swift API (AUD-009).
- [x] T038 [US1] Extend `Tests/C1/test-runtime-replay.sh` and `Tests/test_main.cpp` so every named replay scenario compares CPU state, safe point, ledger, and the T037 machine digest for ten identical runs; verify the replay script and `git diff --check` (AUD-009, SC-002).
- [x] T039 [US1] Add a throwing-trace-observer regression in `Tests/test_main.cpp` and complete the failure/timing contract in `Sources/BeebCore/include/beeb/cpu6502.hpp`; verify unchanged functional tracing and pre-instruction CPU state after the throw (AUD-013).
- [x] T040 [US1] Synchronize last-completed-boundary, replay-digest, trace-failure, and sustained-fixture rationale in `docs/code/runtime-ownership.md`, `docs/code/timing-model.md`, and affected declaration comments; verify `Tests/C1/test-runtime-contract.sh`, `Tests/C1/test-runtime-replay.sh`, `make docs-check`, and `git diff --check`, then commit Phase 8 complete (AUD-001, AUD-004, AUD-009, AUD-013).

**Checkpoint**: US1 faults and replays expose one coherent deterministic machine
boundary with no C3 snapshot-format scope expansion.

---

## Phase 9: User Story 2 Remediation — Complete Transaction and Race Evidence (P2)

**Goal**: Prove every command/state cell and the full concurrent transaction
set through the same owner.

**Independent Test**: A table-driven matrix covers paused, running, faulted, and
shutting-down results for all 17 command kinds, while a 10,000-operation
sanitizer scenario covers start, pause, reset, copied media, input, query,
controlled failure/recovery, and shutdown without lost work or races.

- [x] T041 [US2] Add the complete table-driven lifecycle/command matrix in `Tests/test_main.cpp` and route it through `Tests/C1/test-runtime-contract.sh`; assert status, state transition, mutation, output presence, idempotence, and unavailable behavior for every applicable cell before correcting any exposed behavior (AUD-007, SC-001).
- [x] T042 [US2] Correct only matrix violations exposed by T041 in `Sources/BeebCore/src/runtime.cpp` and affected contracts under `specs/002-runtime-ownership/contracts/`; verify the full matrix, `make test`, `make sanitize`, and `git diff --check` (AUD-007).
- [x] T043 [US2] Expand `testC1RaceMixedCommands` and shutdown/fault scenarios in `Tests/test_main.cpp` to at least 10,000 accounted operations spanning start, pause, reset, OS/sideways/disc loads, key/BREAK, observations, deterministic failure, fault query, reset recovery, and shutdown; use latches/acceptance counters rather than correctness sleeps (AUD-006, FR-018).
- [x] T044 [US2] Strengthen `Tests/C1/test-runtime-races.sh` so T043 runs normally and under supported ThreadSanitizer with deadlock/time-out and accepted-completion accounting; verify ten consecutive normal runs, the local explicit N/A path where applicable, `make sanitize`, and `git diff --check`, then commit Phase 9 complete (AUD-005, AUD-006, SC-003).

**Checkpoint**: US2 has complete command-matrix and full-interaction race
evidence independent of public-language adapters.

---

## Phase 10: User Story 3 Remediation — Recoverable Public Boundaries (P3)

**Goal**: Make allocation recovery non-terminating and prove every applicable
C and Swift status/output contract.

**Independent Test**: Injected queue, ledger, frame/audio, bounded, and sustained
allocation failures return stable categories without termination or partial
mutation; every fallible C entry point and Swift category has applicable
success/failure/recovery evidence.

- [x] T045 [US3] Add failing allocation-injection and oversized-audio regressions for request/queue, ledger, frame/audio, bounded, and sustained paths in `Tests/test_main.cpp`; assert status category, diagnostic fallback, lifecycle, mutation boundary, waiter completion, and later query/reset behavior before implementation (AUD-003).
- [x] T046 [US3] Implement allocation-free failure fallback, ordered `std::bad_alloc`/size handling, and audio `max_size()` validation in `Sources/BeebCore/src/runtime.cpp` and private declarations in `Sources/BeebCore/include/beeb/runtime.hpp`; verify T045, `make test`, `make sanitize`, and no exception escape from `noexcept` (AUD-003).
- [x] T047 [US3] Build a declaration-driven C 0.2 matrix in `Tests/test_main.cpp` covering every fallible declaration in `Sources/BeebCore/include/beeb_c.h` with applicable live-handle success, invalid input/state, execution/resource/unavailable failure, stale diagnostic, and output-preservation cases; correct the sideways-ROM 1...16,384 contract and operation-specific return documentation in `beeb_c.h` (AUD-008, IDC-002, SC-004).
- [x] T048 [US3] Add direct and `@testable` Swift mapping/recovery cases for every `BeebStatusCategory` plus task-group fault/query/reset recovery and final-reference release in `Tests/BeebKitTests/BeebMachineTests.swift`; expose only the smallest internal mapper seam in `Sources/BeebKit/BeebMachine.swift` and add no unsafe public callback (AUD-008, AUD-016).
- [x] T049 [US3] Add a deterministic private C++ reentrant-submission producer test in `Tests/test_main.cpp` and document `BEEB_STATUS_REENTRANT_CALL`/Swift `.reentrantCall` as reserved in `Sources/BeebCore/include/beeb_c.h`, `Sources/BeebKit/BeebMachine.swift`, and boundary contracts (AUD-012).
- [x] T050 [US3] Complete DocC parameter and `- Throws:` contracts for every public throwing API, including key/BREAK bounds and FIFO semantics, in `Sources/BeebKit/BeebMachine.swift`; extend `Tests/C0/test-documentation.sh` to reject missing Swift throwing contracts (AUD-015, IDC-006).
- [ ] T051 [US3] Document registry lock order, stable-token admission, retained active calls, concurrent-destroy ownership, and rejection invariants near `HandleState`, `ActiveCall`, and `beeb_destroy` in `Sources/BeebCore/src/beeb_c.cpp`, linking `docs/code/host-boundary.md`; verify `Tests/C1/test-public-boundaries.sh`, Swift tests/build, `make docs-check`, and `git diff --check`, then commit Phase 10 complete (IDC-014).

**Checkpoint**: US3 satisfies SC-004 with stable allocation, C ABI, Swift
mapping, concurrency, and destruction evidence.

---

## Phase 11: Inline Documentation Remediation

**Purpose**: Resolve the reviewed inline-documentation catalogue with focused
contracts and rationale, then calibrate an enforceable private/internal gate.

- [ ] T052 [P] Add single-owner/threading and callback ownership/invocation/exception contracts to `Sources/BeebCore/include/beeb/crtc6845.hpp`, `disc_image.hpp`, `intel8271.hpp`, `sn76489.hpp`, `teletext_renderer.hpp`, `via6522.hpp`, and `video_ula.hpp`; verify generated declarations and `git diff --check` (IDC-003).
- [ ] T053 [P] Replace the CPU private TODO with grouped bus/addressing/stack/flag/arithmetic/finish invariants and document 8271 `Status`/`Transfer` responsibilities in `Sources/BeebCore/include/beeb/cpu6502.hpp` and `Sources/BeebCore/include/beeb/intel8271.hpp`; verify `make docs-check` (IDC-004, IDC-005).
- [ ] T054 [P] Document `PlatformImage`, `EmulatorModel`, `ContentView`, `BeebDemoApp`, and the Swift test-suite boundary in `Sources/BeebDemo/main.swift` and `Tests/BeebKitTests/BeebMachineTests.swift`; verify Swift DocC/test compilation and `git diff --check` (IDC-007, IDC-008).
- [ ] T055 Document `RAMBus`, `TestFailure`, the test registry/schema, and `C1ReplaySignature`/`C1CapturedReplay`/`C1ReplayOutcome` in `Tests/test_main.cpp`; explain evidence responsibilities without per-test boilerplate and verify `make docs-check` (IDC-009, IDC-010).
- [ ] T056 [P] Document `Output`, `Output::Kind`, `Options`, C-handle aliases, `FlatBus`, and `ROMBuilder` in `Tools/beeb-evidence/main.cpp`, `Tools/beeb-headless/main.cpp`, and `Tools/make-demo-rom/main.cpp`; verify strict builds and `make docs-check` (IDC-011, IDC-012, IDC-013).
- [ ] T057 [P] Add authoritative NMOS indirect-JMP, decimal-flag, transition, interrupt/reset, RMW, and opcode-timing rationale in `Sources/BeebCore/src/cpu6502.cpp` with primary-reference links; verify CPU tests and `git diff --check` (IDC-015, IDC-016).
- [ ] T058 [P] Add authoritative CRTC register-mask/readability and DFS geometry/interleave rationale in `Sources/BeebCore/src/crtc6845.cpp` and `Sources/BeebCore/src/disc_image.cpp`; verify focused CRTC/disc tests and `make docs-check` (IDC-017, IDC-018).
- [ ] T059 [P] Add focused 8271 protocol/timing and aggregate BBC wiring/address/rendering/keyboard rationale in `Sources/BeebCore/src/intel8271.cpp` and `Sources/BeebCore/src/machine.cpp`; verify 8271, memory-map, bitmap, and keyboard tests (IDC-019, IDC-020).
- [ ] T060 [P] Add authoritative protocol, approximation, and observable-consequence rationale in `Sources/BeebCore/src/sn76489.cpp`, `teletext_renderer.cpp`, `via6522.cpp`, and `video_ula.cpp`; verify sound, teletext, VIA, and video tests plus `make docs-check` (IDC-021, IDC-022, IDC-023, IDC-024).
- [ ] T061 Replace opcode/register syntax-translation comments with block-level hardware/evidence rationale and document C1 latch/deadline invariants in `Tests/test_main.cpp`; verify runtime, replay, race, hardware fixture, and documentation gates (IDC-025, IDC-026).
- [ ] T062 Document evidence serialization schemas and deterministic demo-ROM construction in `Tools/beeb-evidence/main.cpp` and `Tools/make-demo-rom/main.cpp`; verify exact evidence/reference tests and strict tool builds (IDC-027, IDC-028).
- [ ] T063 [P] Document temporary-file ownership and changed-complex-code gate rationale in `scripts/build-docs.sh`, linking `docs/CODE_DOCUMENTATION.md`; verify shell syntax and documentation negative fixtures (IDC-029, IDC-030).
- [ ] T064 [P] Document interrupted measurement persistence, locale-stable reference ordering, failure-retaining aggregate execution, and executable TSan probing in `scripts/measure-c0.sh`, `scripts/verify-c0-references.sh`, `scripts/verify-c0.sh`, and `Tests/C1/test-runtime-races.sh`; verify shell syntax and affected C0/C1 scripts (IDC-031, IDC-032, IDC-033, IDC-034).
- [ ] T065 Define and enforce branch-aware private/internal documentation scope in `scripts/build-docs.sh`, `Doxyfile`, `Tests/C0/test-documentation.sh`, `Tests/Fixtures/C0/documentation-debt.txt`, and `docs/CODE_DOCUMENTATION.md`; add an undocumented-internal negative fixture, separate public/internal debt and reviewed exclusions, verify `make docs-check` with explicit `develop...HEAD`, inspect representative generated pages, and commit Phase 11 complete (AUD-010, AUD-011, AUD-019, AUD-022, IDC-001).

**Checkpoint**: Every IDC-001–IDC-034 item is documented or represented by a
narrow reviewed exclusion consumed by the branch-aware gate.

---

## Phase 12: Toolchain, CLI, CI, and Contract Reconciliation

**Purpose**: Close remaining cross-cutting audit blockers and make them
regression-proof in maintained gates.

- [ ] T066 Add failing CLI boundary/overflow cases for `--pc`, `--rom BANK`, and other narrowed numeric options in a focused shell contract under `Tests/C0/`; assert exact maximum acceptance and rejection of one-past/wrapping inputs before implementation (AUD-014).
- [ ] T067 Validate parsed wide integers before narrowing in `Tools/beeb-headless/main.cpp`, preserve valid functional/BBC behavior, and verify T066, strict tool builds, `make test`, and `git diff --check` (AUD-014).
- [ ] T068 Reconcile tracing, C 0.2 completion, owned-frame terminology, concurrent-host guidance, and required verification commands across `README.md`, `CHANGELOG.md`, `CONTRIBUTING.md`, `docs/code/runtime-ownership.md`, `docs/code/architecture.md`, and `docs/code/host-boundary.md`; add stale-text/link assertions to `Tests/C1/test-documentation.sh` (AUD-020).
- [ ] T069 Verify the committed mechanical formatting pass covers every declared C/C++ file, correct remaining drift with formatting-only changes, and keep semantic edits out of the diff; run `clang-format --dry-run --Werror` and `git diff --check` (AUD-018).
- [ ] T070 Confirm `make format-check` and its dedicated `.github/workflows/ci.yml` step fail on an injected formatting fixture and pass the clean tree; verify `make test` and `make sanitize` before commit (AUD-018).
- [ ] T071 Make the supported Linux C1 lane in `.github/workflows/ci.yml` run both `make thread-sanitize` and `make test-c1`, with a strict environment mode in `Tests/C1/test-runtime-races.sh` that fails unexpected TSan N/A while preserving honest local unsupported reporting; validate workflow/shell syntax (AUD-005).
- [ ] T072 Correct any remaining three-story contradiction in `specs/002-runtime-ownership/spec.md` and run Spec Kit cross-artifact analysis against `plan.md`, `tasks.md`, data model, and contracts; resolve every critical/high inconsistency without rewriting T001–T030 history (AUD-021).
- [ ] T073 Reconcile public generated coverage versus excluded/reviewed internal surfaces in `docs/STATUS.md` and synchronize the wording with the T065 debt model and actual generated pages (AUD-022).
- [ ] T074 Verify remote tag/release-note state for 0.2.0, then either keep the release pending/Unreleased in `CHANGELOG.md` and `docs/STATUS.md` or record existing verifiable release evidence consistent with `docs/RELEASING.md`; do not create or publish a tag/release without explicit user authorization (AUD-023).
- [ ] T075 Update `specs/002-runtime-ownership/code-audit-catalogue.md` and `inline-documentation-audit-catalogue.md` only from task commits and exact verification evidence, checking AUD-001–AUD-023 and IDC-001–IDC-034 individually with links to their closing tasks; run sequential-ID/coverage validation and `git diff --check`.
- [ ] T076 Run the clean committed remediation candidate through `make test`, `make sanitize`, `make thread-sanitize`, `swift test`, `swift build`, `make verify-c0`, `make test-c1`, `make docs-check`, `make format-check`, `git diff --check`, `git status --short`, and `git ls-files .build`; record exact honest evidence in `docs/STATUS.md`, update `docs/CORE_ROADMAP.md` only if every blocker is closed, obtain a final code review with no CRITICAL/HIGH finding, and commit Phase 12/C1 remediation complete without an empty duplicate checkpoint.

**Checkpoint**: All AUD and IDC findings are closed with evidence, supported CI
gates are mandatory, release/status claims are honest, and the clean candidate
is merge-ready.

## Remediation Dependencies and Coverage

```text
T031 -> T032
T033 -> T034
(T032 + T034) -> T035 -> T036 -> T037 -> T038 -> T039 -> T040
T040 -> T041 -> T042 -> T043 -> T044
T044 -> T045 -> T046 -> T047 -> T048 -> T049 -> T050 -> T051
T051 -> (T052, T053, T054, T056, T057, T058, T059, T060, T063, T064)
(T052 + T053 + T054) -> T055
(T055 + T056 + T057 + T058 + T059 + T060) -> T061 -> T062
(T061 + T062 + T063 + T064) -> T065
T065 -> T066 -> T067 -> T068 -> T069 -> T070 -> T071 -> T072 -> T073 -> T074 -> T075 -> T076
```

- US1 / AUD-001, AUD-004, AUD-009, AUD-013: T035–T040.
- US2 / AUD-005, AUD-006, AUD-007: T041–T044 and T071.
- US3 / AUD-003, AUD-008, AUD-012, AUD-015, AUD-016: T045–T051.
- Safety/provenance / AUD-002, AUD-017: T031–T034.
- Inline documentation / AUD-010, AUD-011, AUD-019, AUD-022 and IDC-001–IDC-034: T052–T065 and T073.
- CLI/toolchain/docs/governance / AUD-005, AUD-014, AUD-018, AUD-020–AUD-023: T066–T076.

## Parallel Execution Examples

- After T051, independent worktrees may execute T052 (device declarations),
  T054 (Swift declarations), T056 (tool abstractions), T057–T060 (separate
  production source groups), and T063–T064 (separate shell tooling) in parallel.
- T031 and T033 are independent safety-test tasks but each must receive its own
  verified Lore commit before T032 or T034 begins.
- US1, US2, and US3 remain sequential because each depends on the corrected
  owner/safe-point behavior of the preceding story.

## Remediation Delivery Strategy

1. **Safety MVP**: T031–T040 closes destructive tooling and P0 runtime faults;
   do not proceed to broad evidence reruns before this slice is green.
2. **Contract completion**: T041–T051 closes SC-001 through SC-004 behavior and
   public-boundary evidence.
3. **Documentation truth**: T052–T065 resolves the complete inline catalogue
   before strengthening the private/internal gate.
4. **Merge readiness**: T066–T076 closes CLI, formatting, CI, consistency,
   release, catalogue, and final-candidate governance.
