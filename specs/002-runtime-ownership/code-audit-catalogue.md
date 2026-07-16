# Code Audit Catalogue: Phase C1 Runtime Ownership

**Audit date**: 2026-07-16

**Branch**: `002-runtime-ownership`

**Verdict**: Request changes
**Scope**: All 53 repository code files (28 C/C++ headers or sources, 5 Swift
files, 20 shell scripts, and `Package.swift`) plus the active specification,
contracts, constitution, build rules, CI, and code-documentation standard.

This catalogue records issues; it does not authorize implementation. Items are
ordered by dependency and risk. P0 items are correctness or destructive-safety
defects. P1 items block the current C1 evidence claims. P2 items are required
standards, documentation, or governance repairs.

## P0: Correctness and destructive safety

- [ ] **AUD-001 Preserve a coherent whole-machine safe point after a late execution fault.**
  FR-012 and FR-014 require a safe fault boundary without partial mutation
  (`spec.md:206-212`). `BBCMicro::runFor()` can complete and tick several
  instructions before a later instruction throws (`Sources/BeebCore/src/machine.cpp:151-155`),
  but all three fault paths restore only `CPUState`
  (`Sources/BeebCore/src/runtime.cpp:592-608,611-628,631-650`). Add a regression
  in which legal instructions mutate RAM/devices before an illegal opcode, then
  retain one coherent last-completed whole-machine state for bounded run,
  run-to-frame, and sustained execution.

- [ ] **AUD-002 Prevent repository or user-data deletion through output-path options.**
  `scripts/build-docs.sh:77-82,265`, `scripts/verify-c0.sh:49-58`, and
  `scripts/verify-c0-references.sh:31,152` reject only an empty path or `/`
  before `rm -rf`; `scripts/measure-c0.sh:19,28-31` similarly deletes a derived
  fixed work directory. Canonicalize paths, reject repository/home/source roots
  and directories not created by the tool, prefer `mktemp -d` plus an EXIT
  trap, and add destructive-path negative tests.

- [ ] **AUD-003 Make allocation-failure recovery allocation-free.**
  `completeRequest()` and `executeRunningSlice()` are `noexcept`, but their
  exception handlers construct or assign `std::string` diagnostics
  (`Sources/BeebCore/src/runtime.cpp:419-434,631-650`). A second allocation
  failure escapes the handler and invokes `std::terminate`, contradicting the
  recoverable `resourceExhausted` contract. Use fixed-capacity or empty fallback
  diagnostics on low-memory paths and add injected allocation-failure tests for
  queue, ledger, frame/audio, bounded, and sustained execution paths.

## P1: C1 contract and evidence blockers

- [ ] **AUD-004 Make sustained-execution fixtures deterministic and non-faulting.**
  `makeNOPOSROM()` fills the ROM with NOPs but does not loop
  (`Tests/test_main.cpp:752-757`), while lifecycle tests start unbounded
  execution (`Tests/test_main.cpp:778-797`). Execution can reach the FC00-FEFF
  I/O overlay (`Sources/BeebCore/src/machine.cpp:72-76`) and decode device data
  as opcodes. The audit observed an intermittent lifecycle-matrix failure at
  `Tests/test_main.cpp:761`. Use the loop fixture already employed by Swift or
  another closed deterministic program, and add repeated regression coverage.

- [ ] **AUD-005 Run ThreadSanitizer and the complete C1 aggregate in supported CI.**
  The specification requires TSan (`spec.md:222-224`), and the evidence ledger
  says supported CI must execute it (`docs/STATUS.md:66-68`), but
  `.github/workflows/ci.yml:10-57` runs neither `make thread-sanitize` nor
  `make test-c1`. Add a supported Linux C1 lane that fails when its designated
  TSan profile cannot run and records both gates before C1 remains Complete.

- [ ] **AUD-006 Expand race evidence to the full FR-018 interaction set.**
  The 10,000-command stress at `Tests/test_main.cpp:1361-1394` omits ROM/disc
  loads, execution failure, and shutdown; the separate shutdown scenario at
  `Tests/test_main.cpp:1398-1451` still omits media and fault overlap. Exercise
  start, pause, reset, copied loads, input, query, deterministic failure, and
  shutdown together under the supported TSan profile.

- [ ] **AUD-007 Cover every state/command matrix cell.**
  SC-001 requires every lifecycle transition and matrix entry
  (`spec.md:251-252`; `contracts/runtime-state-machine.md:5-14`). Current fault
  tests (`Tests/test_main.cpp:804-833`) omit sideways/disc load, key, BREAK,
  run-to-frame, audio, safe-point, and several shutdown-state outcomes. Build a
  table-driven matrix asserting status, state transition, mutation, and output
  semantics for paused, running, faulted, and shutting-down states.

- [ ] **AUD-008 Complete per-entry-point C ABI and Swift recovery evidence.**
  SC-004 requires each applicable fallible C operation to cover success,
  invalid input/state, execution failure, and stale diagnostics
  (`spec.md:259-261`). `Tests/test_main.cpp:344-438` lacks complete coverage for
  run-to-frame, sideways ROM, disc, safe-point, fault outputs, and lifecycle
  failures. Swift's seven tests do not cover every reachable category. Create a
  declaration-driven C 0.2 matrix and matching Swift mapping/recovery cases,
  including output preservation and shutdown unavailability.

- [ ] **AUD-009 Add a whole-machine/device signature to replay evidence.**
  SC-002 requires a final CPU/device signature (`spec.md:253-255`), but replay
  compares only `CPUState`, `SafePoint`, and ledger data
  (`Tests/test_main.cpp:853-1009`); `SafePoint` has no RAM or device digest
  (`Sources/BeebCore/include/beeb/runtime.hpp:72-80`). Add a test-only digest
  covering relevant RAM and device state to every ten-run replay comparison.

- [ ] **AUD-010 Validate documentation against the feature branch, not only uncommitted files.**
  `scripts/build-docs.sh:87-92,215-253` derives changed code from
  `git diff ... HEAD`. On the clean committed candidates required by T027/T030,
  that list is empty, so the complex-code rationale gate proves nothing about
  branch changes. Accept or derive a reviewed base revision and validate
  `base...HEAD`; make CI and release evidence pass the base explicitly.

- [ ] **AUD-011 Enforce the required private/internal documentation scope.**
  The constitution and FR-020 require private/internal named abstractions, and
  SC-006 claims generated coverage (`spec.md:227-232,264-267`). Doxygen excludes
  private symbols (`Doxyfile:14`), the generator covers only selected roots
  (`scripts/build-docs.sh:272-278,309-310`), and the debt inventory excludes
  private header sections (`Tests/Fixtures/C0/documentation-debt.txt:21`). Define
  an enforceable scope, cover all changed production roots/private sections or
  record reviewed debt, and add a negative fixture for an undocumented internal
  abstraction.

- [ ] **AUD-012 Reconcile the public reentrant status with reachable behavior.**
  `BEEB_STATUS_REENTRANT_CALL` is public (`Sources/BeebCore/include/beeb_c.h:35`),
  but its only producer is owner-thread submission
  (`Sources/BeebCore/src/runtime.cpp:200-205`), and no public owner callback can
  reach it. Either add a safe deterministic test seam and corresponding C/Swift
  evidence or narrow the public acceptance promise and document the category as
  internal/reserved.

- [ ] **AUD-013 Keep `CPU6502::step()` atomic when a trace observer throws.**
  `step()` fetches and advances PC before invoking the user callback
  (`Sources/BeebCore/src/cpu6502.cpp:231-240`), but the public declaration
  documents an atomic instruction/interrupt transition and only an illegal-opcode
  exception (`Sources/BeebCore/include/beeb/cpu6502.hpp:45-58`). Contain observer
  exceptions with complete pre-fetch restoration or make the callback contract
  non-throwing; document timing/threading/failure behavior and add a regression.

- [ ] **AUD-014 Reject out-of-range CLI values before narrowing.**
  `Tools/beeb-headless/main.cpp:216-220` narrows parsed `uint64_t` values to
  `uint16_t`/`unsigned` before validation, so values such as `--pc 0x10000` or a
  wrapping ROM bank can be accepted as another configuration. Validate the wide
  value first and add boundary/overflow CLI tests.

## P2: Standards, documentation, and governance

- [ ] **AUD-015 Document thrown errors on every public Swift throwing API.**
  Public throwing declarations in `Sources/BeebKit/BeebMachine.swift:147-160,222-328`
  mostly omit DocC `- Throws:` sections, contrary to
  `docs/CODE_DOCUMENTATION.md:16-25`. Name relevant `BeebError` and lifecycle
  categories and extend the documentation gate to reject undocumented throwing
  contracts.

- [ ] **AUD-016 Add concurrent Swift failure/recovery and final-release tests.**
  The only task-group case (`Tests/BeebKitTests/BeebMachineTests.swift:150-178`)
  covers lifecycle/input/query, while `contracts/swift-runtime-api.md:13-14`
  also promises concurrent failure recovery and release after tasks complete.
  Add task-group recovery/query/reset coverage, then release the final strong
  reference after the group joins and verify destruction completes safely.

- [ ] **AUD-017 Make Klaus evidence immutable and collision-safe.**
  `scripts/run-klaus.sh:5-8` downloads from mutable `master`, verifies no digest,
  and writes a predictable shared temporary filename. Pin a reviewed commit,
  record and verify SHA-256, use `mktemp -d`, and clean it with a guarded EXIT
  trap.

- [ ] **AUD-018 Bring C/C++ files into declared formatting conformance and add a gate.**
  CONTRIBUTING requires `.clang-format` (`CONTRIBUTING.md:34-42`), but
  `clang-format --dry-run --Werror` rejects 26 of 28 C/C++ files, including all
  C1 runtime/C-boundary implementation files and `Tests/test_main.cpp`. Apply a
  separately reviewable mechanical formatting pass, then add a Make/CI check so
  conformance cannot drift again.

- [ ] **AUD-019 Replace TODOs and document uncovered internal named abstractions.**
  `Sources/BeebCore/include/beeb/cpu6502.hpp:89-100` has a documentation TODO in
  the private interface. Named abstractions also lack the required purpose and
  boundary documentation in `Tools/beeb-evidence/main.cpp:20-31`,
  `Sources/BeebDemo/main.swift:13,98,135`, and C1 replay helpers near
  `Tests/test_main.cpp:853,880,916`. Document them or record a narrow reviewed
  exclusion, then make the gate consume the inventory.

- [ ] **AUD-020 Synchronize stale and contradictory user/developer documentation.**
  Resolve the following as one contract pass: README recommends BBC-mode
  `--trace` (`README.md:83`) while the changelog removes it (`CHANGELOG.md:44-46`);
  the runtime guide says C 0.2 is still later work (`docs/code/runtime-ownership.md:8-10`);
  architecture calls C pointers borrowed (`docs/code/architecture.md:24-28`)
  while frames are owned (`docs/code/host-boundary.md:35-39`); CONTRIBUTING asks
  hosts to serialize access (`CONTRIBUTING.md:38`) although the runtime accepts
  concurrent calls (`docs/code/host-boundary.md:47-51`); and its required checks
  omit docs/C1/TSan gates (`CONTRIBUTING.md:10-19`). Add focused stale-text or
  link checks where practical.

- [ ] **AUD-021 Correct the feature story-count contradiction and rerun consistency analysis.**
  The specification defines three user stories (`spec.md:65,95,125`) but calls
  the package two stories in assumptions (`spec.md:271-272`). Correct the
  dependency description and rerun Spec Kit analysis before treating the
  planning stack as internally consistent.

- [ ] **AUD-022 Stop using zero recorded debt as proof of internal completeness.**
  `docs/STATUS.md:23,120-123` reports zero documentation debt while explicitly
  excluding private header sections. Reword the evidence to distinguish public
  generated coverage from excluded internal surfaces, then synchronize it with
  AUD-010's enforceable debt/scope decision.

- [ ] **AUD-023 Reconcile the 0.2.0 release record with the release procedure.**
  `CHANGELOG.md:9,81` presents a dated release/link, while
  `docs/RELEASING.md:18-27` requires an annotated tag and published notes and the
  repository currently has no local tags. Keep 0.2.0 as Unreleased until
  merge/tag or complete the release checklist after the blocking audit items.

## Verification snapshot

- `make test all`: failed once at the C1 lifecycle matrix (41/42), then 50
  repeated full-suite runs passed; AUD-003 records the fixture-dependent flake.
- `make sanitize`: 37/37 quick tests passed under UndefinedBehaviorSanitizer.
- `swift test`: 7/7 passed.
- `swift build`: passed, including a separate strict-concurrency build.
- `make docs-check`: passed, subject to the scope defects in AUD-009/AUD-010.
- `make test-c1`: all six groups passed.
- `make thread-sanitize`: reported N/A on this host; AUD-004 records the missing
  supported CI execution.
- `bash -n` passed for all shell scripts; `shellcheck` was unavailable.
- `git diff --check`: passed before this catalogue was added.

## Recommended execution order

`AUD-001/AUD-003 -> AUD-004 -> AUD-007/AUD-008 -> AUD-006/AUD-009 -> AUD-005`, then
`AUD-010 -> AUD-011 -> AUD-015/AUD-019/AUD-022`. AUD-002 and AUD-017 are
independent safety/reproducibility repairs. Complete AUD-020/AUD-021/AUD-023
only after the resulting implementation and evidence claims are settled.
