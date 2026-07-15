# Tasks: Core Baseline Evidence

**Input**: Design documents from `/specs/001-core-baseline-evidence/`

**Prerequisites**: `plan.md`, `spec.md`, `research.md`, `data-model.md`,
`contracts/`, and `quickstart.md`

**Evidence**: Behavior and quality-gate changes require their negative tests to
be written and observed failing before implementation. Approved references are
created only after ten-run determinism and provenance validation pass.

**Code Documentation**: C0 establishes the system. Each later coding task must
update affected public contracts and non-obvious rationale in the same logical
change. Generated output remains under `.build/docs/` and is validated rather
than committed.

**Organization**: Tasks are grouped by independently testable user story. Task
IDs encode the intended dependency order; `[P]` means different files and no
unfinished dependency.

**Git**: Commit every task after its focused verification succeeds and before
starting the next task. The final task commit in each phase should explicitly
record phase completion and serves as the phase checkpoint. If phase-level
artifacts remain after that task, commit them separately before the next phase;
do not create an empty duplicate commit.

## Phase 1: Setup and Baseline Record

**Purpose**: Record the foundation before C0 tooling changes and prepare only
shared paths that do not implement story behavior.

- [x] T001 Record the current command counts, version values, supported CI
  profiles, known limitations, source revision, and exact research-only
  clean-room signatures in `docs/CORE_BASELINE.md`; label them pre-C0 evidence,
  not approved references
- [x] T002 Add ignored generated C0 and documentation output conventions for
  `.build/c0/` and `.build/docs/` to `.gitignore` only if the existing `.build/`
  rule does not already cover them; prove no tracked reference path is ignored

**Checkpoint**: The pre-change evidence is reviewable and no C0 behavior has
been implemented.

---

## Phase 2: Foundational Test and Fixture Contracts

**Purpose**: Establish shared contract-test helpers and canonical workload
identity before story implementations. These tasks block all user stories.

- [x] T003 Add strict temporary-directory, command-capture, expected-failure,
  and no-mutation helpers to `Tests/C0/testlib.sh`; keep diagnostics plain-text
  and verify the helper cleans only its own temporary directory
- [x] T004 [P] Add `beeb-c0-evidence-v1` CLI usage/error contract tests to
  `Tests/C0/test-fixture-evidence.sh` for missing workload, invalid cycle count,
  unwritable output, and unknown output kind; run them and record the expected
  pre-implementation failures in the task notes
  - Red evidence: exits `1` with `missing C0 evidence executable` before T006.
- [x] T005 [P] Add named bitmap and Mode 7 workload contract coverage for
  `Tools/make-demo-rom/main.cpp` in `Tests/C0/test-demo-rom.sh` before generator
  changes; observe the missing-workload failure
  - Red evidence: named Mode 7 invocation exits `1` before T007.
- [x] T006 Implement the minimal deterministic workload/output interface in
  `Tools/beeb-evidence/main.cpp`, reusing public CPU-state and frame access only,
  and add its build rule to `Makefile`; make T004 pass without changing
  `Sources/BeebCore/`
- [x] T007 Extend `Tools/make-demo-rom/main.cpp` with the named redistributable
  bitmap and Mode 7 workloads defined by T005; make T005 pass and preserve the
  existing default demo-ROM behavior

**Checkpoint**: Named lawful workloads and a public-surface-only evidence tool
exist; each user story can now be implemented and tested independently.

---

## Phase 3: User Story 1 — Reproduce the Known-Good Baseline (Priority: P1) MVP

**Goal**: One command exhaustively reports behavioral, safety, version, public
boundary, fixture, reference, and documentation evidence.

**Independent Test**: Run a verifier test double where multiple groups pass,
fail, or are unavailable; confirm every applicable group runs, the final status
is derived correctly, detailed output is retained, and approved references do
not change.

### Tests for User Story 1 (write and observe failing first)

- [x] T008 [US1] Add `Tests/C0/test-baseline-verifier.sh` cases for all-pass,
  one failure followed by later execution, multiple failures, unexpected skip,
  declared profile N/A, interrupted command, stable summary ordering, and
  immutable `Tests/Fixtures/C0/`; record the expected missing-verifier failures
  - Red evidence: exits `1` with `missing C0 baseline verifier` before T011.
- [x] T009 [P] [US1] Extend C-boundary negative tests in `Tests/test_main.cpp`
  to cover every fallible function in `Sources/BeebCore/include/beeb_c.h`,
  including nullability, invalid lengths/indices, stable error reset, and no C++
  exception escape; observe any new failing contract before code changes
  - Characterization evidence: all new cases pass without a production change.
- [x] T010 [P] [US1] Extend `Tests/BeebKitTests/BeebKitTests.swift` with matching
  Swift version/error translation and recovery cases; observe any new failing
  contract before code changes
  - Characterization evidence: all new cases pass without a production change.

### Implementation for User Story 1

- [x] T011 [US1] Implement exhaustive profile-aware orchestration and stable
  summary output in `scripts/verify-c0.sh` according to
  `contracts/baseline-verification.md`; make T008 pass while continuing after
  group failures
- [x] T012 [US1] Add `verify-c0` and `test-c0` targets to `Makefile`, ensuring
  normal verification can write only under `.build/c0/` and never update
  `Tests/Fixtures/C0/`
- [x] T013 [US1] Fix only contract defects exposed by T009/T010 in
  `Sources/BeebCore/src/beeb_c.cpp`, `Sources/BeebKit/BeebMachine.swift`, or
  `Sources/BeebKit/BeebVersion.swift`; if no defect exists, record that the
  expanded tests passed without production changes
  - Outcome: C++ 27/27 and Swift 7/7 pass; no production fix was required.
- [x] T014 [US1] Add Linux and macOS execution of
  `Tests/C0/test-baseline-verifier.sh` to `.github/workflows/ci.yml`, proving the
  orchestration contract without enabling the full aggregate job before later
  stories provide their required evidence groups
- [x] T015 [US1] Document the command, group matrix, failure recovery, profile
  differences, and non-mutation guarantee in `docs/CORE_BASELINE.md`

**Checkpoint**: US1 independently provides an exhaustive, non-destructive C0
result and remains useful even before approved visual references are populated.

---

## Phase 4: User Story 2 — Audit Redistributable Boot and Video Evidence (Priority: P2)

**Goal**: Exact state, bitmap, and Mode 7 evidence is lawful, reproducible,
reviewable, and immutable during normal verification.

**Independent Test**: Generate each named workload ten times, validate complete
provenance, compare exact outputs, inject a one-byte mismatch, and prove that
normal verification reports but never replaces it.

### Tests for User Story 2 (write and observe failing first)

- [x] T016 [US2] Extend `Tests/C0/test-fixture-evidence.sh` with ten-run identity,
  complete-manifest, orphan-reference, exact state/PPM comparison, one-byte
  mismatch, requested-versus-actual cycle, and approved-path immutability cases;
  observe failures while approved references are absent
  - Red evidence: exits `1` with `missing approved fixture directory` before T018/T019.
- [x] T017 [P] [US2] Add `Tests/C0/test-reference-update.sh` cases for missing
  reason, unknown reference, non-identical candidates, accidental CI use,
  single-reference scope, manifest refresh, and diff presentation; observe the
  missing explicit update flow
  - Red evidence: exits `1` with `missing explicit reference updater` before T020.

### Implementation for User Story 2

- [x] T018 [US2] Add lawful origin, redistribution basis, workload generation,
  coverage limits, and reference-review instructions to
  `Tests/Fixtures/C0/README.md`
- [x] T019 [US2] Generate ten identical candidates for each named workload into
  `.build/c0/candidate/`; only after T016's determinism/provenance portions pass,
  add `approved-state.txt`, `bitmap.ppm`, `mode7.ppm`, and complete derived fields
  in `Tests/Fixtures/C0/manifest.txt`
- [x] T020 [US2] Implement the separately invoked reviewed replacement workflow
  in `scripts/update-c0-reference.sh` according to
  `contracts/evidence-reference.md`; make T017 pass and do not expose it through
  `verify-c0`
- [x] T021 [US2] Add `verify-c0-references` and explicitly named
  `update-c0-reference` targets to `Makefile`; require the reference ID and
  rationale at the update boundary
- [x] T022 [US2] Wire fixture provenance, boot state, bitmap, and Mode 7 groups
  into `scripts/verify-c0.sh`; make all T016 cases pass and preserve later-group
  execution after a reference mismatch
- [x] T023 [US2] Record the approved identities, exact coverage, known visual
  limitations, and replacement history policy in `docs/CORE_BASELINE.md`

**Checkpoint**: US2 independently proves lawful deterministic boot and selected
video output, and its negative test proves references cannot bless themselves.

---

## Phase 5: User Story 3 — Establish a Reproducible Comparison Baseline (Priority: P3)

**Goal**: A named workload yields a valid, repeatable comparison record without
becoming a product performance promise.

**Independent Test**: Produce five complete samples and recompute their median
and range; then prove under-sampling, zero duration, missing environment, and
interruption produce an invalid result.

### Tests for User Story 3 (write and observe failing first)

- [x] T024 [US3] Add `Tests/C0/test-measurement-record.sh` cases for five-sample
  validity, recomputed median/min/max, four samples, zero/negative duration,
  incomplete workload, missing environment/toolchain data, interruption, dirty
  revision marking, and mandatory non-guarantee label; observe the missing
  measurement flow
  - Red evidence: exits `1` with `missing C0 measurement script` before T025.

### Implementation for User Story 3

- [x] T025 [US3] Implement release-build sampling, portable environment capture,
  strict validity derivation, and structured output in `scripts/measure-c0.sh`
  according to `contracts/measurement-record.md`; make T024 pass
- [x] T026 [US3] Add `measure-c0` and an internal measurement-record validation
  target to `Makefile`; keep variable performance outside `verify-c0` success
- [x] T027 [US3] Run at least five canonical workload samples on a recorded
  environment and add the valid comparison record plus interpretation limits to
  `docs/CORE_BASELINE.md`; retain raw generated records under `.build/c0/`

**Checkpoint**: US3 independently provides reproducible comparison evidence,
with no release threshold or compatibility claim.

---

## Phase 6: User Story 4 — Learn the Core from Browsable Code Documentation (Priority: P4)

**Goal**: Contributors can browse useful C/C++, C, Swift, architecture, timing,
boundary, and evidence documentation, while future changes are forced to keep
the knowledge current.

**Independent Test**: Generate the complete site from a clean checkout, follow
all representative links, then use temporary fixtures to prove missing changed
public docs, malformed markup, broken links, and debt growth each fail.

### Tests for User Story 4 (write and observe failing first)

- [x] T028 [US4] Add `Tests/C0/test-documentation.sh` temporary-fixture cases for
  clean Doxygen generation, clean DocC generation on macOS, unified landing
  links, invalid markup, unresolved link, missing changed public contract,
  changed complex code without rationale/N/A, debt addition/broadening, and
  ignored generated output; also assert that every coding feature template
  carries a documentation-impact decision and generated-doc validation task;
  observe the missing docs system failures
  - Red evidence: exits `1` with `missing code documentation builder` before T031.

### Implementation for User Story 4

- [x] T029 [US4] Add a reviewed exact compatible release of the official
  `swiftlang/swift-docc-plugin` as a documentation-only dependency in
  `Package.swift` and `Package.resolved`; verify normal `swift build` adds no
  runtime product dependency
- [x] T030 [P] [US4] Configure strict C/C++/C ABI generation in `Doxyfile` with
  `EXTRACT_ALL=NO`, public-surface and parameter/incomplete warnings, source
  links, Markdown guides, and warnings promoted to failure
- [x] T031 [US4] Implement host-profile detection, Doxygen/DocC static generation,
  unified `.build/docs/index.html`, link checking, and debt-ratchet validation in
  `scripts/build-docs.sh`; make the infrastructure portions of T028 pass
- [x] T032 [P] [US4] Document every supported public C ABI declaration in
  `Sources/BeebCore/include/beeb_c.h`, covering applicable ownership, lifetime,
  nullability, errors, threading, side effects, and frame-buffer validity
- [x] T033 [P] [US4] Document supported public C++ declarations in
  `Sources/BeebCore/include/beeb/*.hpp`, and add concise rationale links beside
  representative CPU, machine/device timing, state-transition, and buffer code
  in `Sources/BeebCore/src/*.cpp`; do not annotate self-evident helpers
- [x] T034 [P] [US4] Document the public Swift wrapper in
  `Sources/BeebKit/BeebMachine.swift` and `Sources/BeebKit/BeebVersion.swift`,
  including ownership, actor/thread expectations, errors, and C-buffer copying
- [x] T035 [P] [US4] Add the DocC module landing page to
  `Sources/BeebKit/Documentation.docc/BeebKit.md` and focused conceptual guides
  to `docs/code/architecture.md`, `docs/code/timing-model.md`,
  `docs/code/host-boundary.md`, and `docs/code/evidence-and-testing.md`
- [x] T036 [US4] Add the contributor standard, good/bad examples, required-detail
  matrix, local/CI commands, N/A rule, and debt-repayment workflow to
  `docs/CODE_DOCUMENTATION.md`
- [x] T037 [US4] Classify every public header surface as covered or internal-only
  and record only reviewed unchanged internal gaps with triggers in
  `Tests/Fixtures/C0/documentation-debt.txt`; make every T028 quality and ratchet
  case pass without expanding the baseline
- [x] T038 [US4] Add `docs` and `docs-check` targets to `Makefile`, wire C/C++
  docs into Linux CI and the complete Doxygen/DocC site into macOS CI in
  `.github/workflows/ci.yml`, and add both documentation groups to
  `scripts/verify-c0.sh`; make the Spec Kit template-enforcement cases in T028
  pass against `.specify/templates/`
- [x] T039 [US4] Run `make docs-check`, inspect representative CPU, machine, C
  ABI, Swift, timing, and evidence pages from `.build/docs/index.html`, and
  record the review and generator versions in `docs/CORE_BASELINE.md`

**Checkpoint**: US4 independently produces navigable, useful documentation and
enforces the same standard for every later Spec Kit coding feature.

---

## Phase 7: Integration, Governance, and C0 Exit

**Purpose**: Reconcile all stories, current project authorities, and full exit
evidence without claiming undelivered fidelity.

- [ ] T040 [P] Update `docs/ARCHITECTURE.md` with the evidence-tool boundary,
  generated-documentation architecture, comment/guide ownership, and the rule
  that neither tooling path enters the runtime core
- [ ] T041 [P] Update `docs/STATUS.md` with only verified C0 commands, exact
  approved fixture/reference coverage, documentation coverage/debt, measurement
  context, and remaining hardware limitations
- [ ] T042 [P] Move C0 to Complete and C1 to Ready in `docs/CORE_ROADMAP.md` only
  after every C0 exit item has a passing evidence link; otherwise leave status
  unchanged and record the missing item
- [ ] T043 [P] Add the delivered aggregate verifier, lawful references,
  comparison baseline, and browsable documentation system to `CHANGELOG.md`
  without changing the release version
- [ ] T044 Validate every command and expected failure in
  `specs/001-core-baseline-evidence/quickstart.md`; correct the guide or
  implementation wherever actual behavior differs
- [ ] T045 Run `make test`, `make sanitize`, `swift test`, `swift build`,
  `make verify-c0`, `make docs-check`, all `Tests/C0/test-*.sh`, and
  `git diff --check`; preserve concise logs and exact exit status as C0 evidence
- [ ] T046 Add Linux portable-profile and macOS complete-profile
  `make verify-c0` jobs to `.github/workflows/ci.yml` now that every required
  group exists; retain the direct test, sanitizer, Swift, and docs jobs so the
  aggregate result cannot hide detailed failures
- [ ] T047 Confirm generated `.build/c0/` and `.build/docs/` outputs are
  untracked, approved references changed only through the reviewed flow, no
  proprietary bytes exist, no unresolved documentation warnings or critical
  Spec Kit findings remain, and the implementation matches all four contracts

---

## Dependencies and Execution Order

### Phase dependencies

- Phase 1 has no dependency.
- Phase 2 depends on Phase 1 and blocks all stories.
- US1 through US4 depend on Phase 2. They are independently testable; the
  priority order is the default delivery order.
- Phase 7 depends on every story selected for C0 exit. C0 cannot be marked
  Complete unless all four stories and all exit evidence pass.

### User-story dependencies

- **US1 (P1)**: uses the shared evidence tool but does not require approved
  reference or measurement implementation to prove orchestration semantics.
- **US2 (P2)**: uses the shared named workloads; its references are independently
  verifiable and immutable.
- **US3 (P3)**: uses the named clean-room workload but not US1 orchestration or
  US2 approved files.
- **US4 (P4)**: documents existing and C0-added surfaces and can build before it
  is added to US1's group matrix. T038 is its only integration with US1.

### Test-first ordering

- T008 precedes T011–T015.
- T009/T010 precede any boundary fix in T013.
- T016/T017 precede T018–T023.
- T024 precedes T025–T027.
- T028 precedes T029–T039.
- T019 cannot approve a reference until ten identical candidates and complete
  provenance pass.
- Documentation comments and guides in T032–T036 ship with the relevant
  surfaces, not as optional polish.

### Parallel opportunities

- T004 and T005 can run in parallel after T003.
- T009 and T010 can run in parallel after T008 is authored.
- T017 can run beside T016.
- T030 and T032–T035 touch independent files after T028.
- T040–T043 touch independent project documents after story verification.

## Implementation Strategy

1. Preserve and record the starting state.
2. Define failing contracts for shared evidence and named workloads.
3. Deliver US1 as the first useful vertical slice: one honest baseline result.
4. Add lawful exact evidence (US2), then descriptive measurement (US3).
5. Establish and populate the browsable documentation system (US4), including
   durable Spec Kit and CI enforcement.
6. Re-run the entire quickstart and only then update phase status.

## Completion Standard

Every checked task must point to its test, generated evidence, or validated
documentation result. “Command exists” is insufficient: deliberate failures
must fail for the named reason, ordinary verification must leave references
unchanged, generated docs must be navigable, and no later feature may omit its
documentation-impact decision.
