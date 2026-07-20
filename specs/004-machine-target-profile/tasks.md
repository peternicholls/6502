# Tasks: Machine Target Profile

**Input**: Design documents from `/specs/004-machine-target-profile/`

**Prerequisites**: `plan.md`, `spec.md`, `research.md`, `data-model.md`, `contracts/`, `quickstart.md`

**Evidence**: Every behavior task starts with a focused test committed after it
fails for the expected missing behavior. Implement the smallest correction,
pass the focused test and commit it before the next task. Product acceptance
also builds, launches and directly observes the maintained application; unit
tests and build success alone cannot close a story.

**Code Documentation**: Update affected public contracts, named internal types
and non-obvious invariants with the code task that introduces them. Do not defer
contract explanations to polish. Final validation generates Doxygen and DocC
and rejects increased documentation debt.

**Git**: Commit each task after its named verification in repository Lore
format. The final task in each phase must say that it closes the phase; do not
create an empty duplicate checkpoint. Stage only that task's files.

## Phase 1: Setup (Shared Evidence Infrastructure)

**Purpose**: Establish a focused, independently runnable target-profile gate
before behavior changes.

- [ ] T001 Create `Tests/TargetProfile/testlib.sh`, `Tests/TargetProfile/test-aggregate-runner.sh` and the `test-machine-target-profile` Make target in `Makefile`; verify the empty focused aggregate and existing tooling gates pass, then commit the setup checkpoint.

---

## Phase 2: Foundational Profile Value (Blocking Prerequisite)

**Purpose**: Define the bounded cross-language value shape used by every story,
without yet making any machine profile constructible.

**⚠️ CRITICAL**: No user-story implementation begins until this phase passes.

- [ ] T002 [P] Add a compile-time red contract for schema version 1, raw 32-bit identifiers, 16-bit component versions, sixteen expansion slots, canonical Model B and Model B+ 64K constructors and value equality in `Tests/TargetProfile/test-contract.sh`; observe the expected missing-header/symbol failure and commit the red test (FR-001, FR-002, FR-007, FR-011).
- [ ] T003 Implement and document the immutable component/profile values and canonical constructors in `Sources/BeebCore/include/beeb/profile.hpp` and `Sources/BeebCore/src/profile.cpp`; pass `Tests/TargetProfile/test-contract.sh`, `make format-check` and `git diff --check`, then commit the C++ value checkpoint (FR-001, FR-002, FR-007, FR-011).
- [ ] T004 [P] Add a C11-compatible red structural probe for fixed-width profile components, fixed expansion capacity, canonical constructors and caller-owned equality in `Tests/TargetProfile/test-boundaries.sh`; observe the expected missing-C-contract failure and commit the red test (FR-001, FR-002, FR-003).
- [ ] T005 Add and document the fixed C profile aggregates, constants and canonical value functions in `Sources/BeebCore/include/beeb_c.h` and `Sources/BeebCore/src/beeb_c.cpp`; pass the C structural probe and unchanged C1 boundary tests, then commit the C value checkpoint (FR-001, FR-002, FR-003, FR-012).
- [ ] T006 [P] Add red Swift value tests for owned `Sendable`/`Equatable` raw components, canonical Model B/Model B+ 64K values and unambiguous display names in `Tests/BeebKitTests/BeebMachineTests.swift`; observe the expected missing-type failure and commit the red test (FR-001, FR-003, FR-012).
- [ ] T007 Implement and DocC-document the immutable Swift profile/component values and C aggregate mapping in `Sources/BeebKit/BeebMachine.swift`; pass the focused Swift tests, `swift build` and `git diff --check`, then commit with a Lore message that explicitly closes the foundational phase (FR-001, FR-003, FR-012).

**Checkpoint**: One bounded identity value exists in C++, C and Swift; it does
not yet imply support or create a machine.

---

## Phase 3: User Story 1 — Select a Model B Target (Priority: P1) 🎯 MVP

**Goal**: Construct and query an explicit Model B profile consistently through
the core, runtime, C, Swift and maintained application.

**Independent Test**: Create a canonical Model B session through each public
boundary, query it repeatedly without advancing emulated state, then build and
launch the macOS application and observe matching requested/active Model B
labels through keyboard and VoiceOver.

### Red tests for User Story 1

- [ ] T008 [US1] Add red C++ tests for Model B validation, `BBCMicro` profile retention, `MachineRuntime` profile-aware construction, owner-serialized repeated query and unchanged safe-point/digest state in `Tests/test_main.cpp`; observe the expected missing behavior and commit the red test (FR-003, FR-004, FR-011, SC-001, SC-006).
- [ ] T009 [US1] Add red C ABI tests for explicit Model B creation/query, null inputs, success-only output writes, unchanged legacy Model B creation and repeated owned round trips in `Tests/test_main.cpp`; observe the expected failure and commit the red test (FR-003, FR-004, FR-008, FR-011, SC-001).
- [ ] T010 [US1] Add red Swift tests for `BeebMachine(profile: .modelB)`, the explicit no-argument Model B convenience, repeated owned profile queries and unchanged runtime state in `Tests/BeebKitTests/BeebMachineTests.swift`; observe the expected failure and commit the red test (FR-003, FR-004, FR-011, SC-001).
- [ ] T011 [US1] Extend `Tests/TargetProfile/test-application-build.sh` with red source/build checks for a native labelled profile picker, stable accessibility identifiers and distinct requested/active profile state in `Sources/BeebDemo/main.swift`; observe the expected failure and commit the red host test (FR-005, FR-012, FR-014, SC-003).

### Implementation for User Story 1

- [ ] T012 [US1] Implement and document supported Model B validation, immutable `BBCMicro` retention and owner-serialized runtime profile query in `Sources/BeebCore/include/beeb/machine.hpp`, `Sources/BeebCore/include/beeb/runtime.hpp`, `Sources/BeebCore/src/machine.cpp` and `Sources/BeebCore/src/runtime.cpp`; pass T008 plus C1 replay/race tests and commit the core Model B checkpoint (FR-003, FR-004, FR-011, SC-001, SC-006).
- [ ] T013 [US1] Implement and document explicit Model B create/query translation while retaining `beeb_create()` as a deliberate Model B convenience in `Sources/BeebCore/include/beeb_c.h`, `Sources/BeebCore/src/beeb_c.cpp` and `docs/code/host-boundary.md`; pass T009 and the C1 public-boundary/lifetime gates, then commit the C Model B checkpoint (FR-003, FR-004, FR-008, FR-011, FR-013, SC-001).
- [ ] T014 [US1] Implement and DocC-document profile-aware Model B construction and owned runtime profile query in `Sources/BeebKit/BeebMachine.swift` and `Sources/BeebKit/Documentation.docc/BeebKit.md`; pass T010, `swift test` and `swift build`, then commit the Swift Model B checkpoint (FR-003, FR-004, FR-011, FR-013, SC-001).
- [ ] T015 [US1] Implement the native labelled picker plus documented requested/active profile invariants, accessibility labels/values/identifiers and Model B candidate installation in `Sources/BeebDemo/main.swift`; pass T011 and maintained macOS/iOS application builds, then commit the host Model B checkpoint (FR-005, FR-012, FR-014, SC-003).
- [ ] T016 [US1] Create `specs/004-machine-target-profile/evidence/macos-application-observation.md`, build and launch `BeebDemo-macOS`, time the journey, select Model B by keyboard, inspect it with VoiceOver and Accessibility Inspector, and record matching requested/active identity plus host/toolchain/commit evidence; verify the record against `contracts/host-profile-observation.md` and commit with a Lore message that closes User Story 1 (FR-005, FR-014, SC-003, SC-004).

**Checkpoint**: Model B identity works end to end and is independently
demonstrable; this is the implementation MVP for the feature.

---

## Phase 4: User Story 2 — Represent Model B+ 64K Separately (Priority: P2)

**Goal**: Carry and display Model B+ 64K as a recognised identity while typed
construction rejection proves that B+ emulation is not yet implemented.

**Independent Test**: Round-trip Model B+ 64K through value/validation
boundaries, attempt construction, and verify the typed recognised-unavailable
result never creates or labels Model B as B+; observe the same behavior in the
running application while its active Model B session remains intact.

### Red tests for User Story 2

- [ ] T017 [US2] Add red C++ and C tests for Model B+ 64K recognition, typed unavailable construction, null/output preservation, no handle registration and explicit no-fallback behavior in `Tests/test_main.cpp`; observe the expected failure and commit the red test (FR-006, FR-008, FR-009, FR-010, SC-002, SC-006).
- [ ] T018 [P] [US2] Add red Swift tests for Model B+ 64K support classification, typed localised rejection and absence of a fallback `BeebMachine` in `Tests/BeebKitTests/BeebMachineTests.swift`; observe the expected failure and commit the red test (FR-006, FR-008, FR-009, SC-002, SC-006).
- [ ] T019 [US2] Extend `Tests/TargetProfile/test-application-build.sh` with red checks for a Model B+ 64K picker choice, explicit recognised-but-unavailable copy and separate unchanged active-profile presentation in `Sources/BeebDemo/main.swift`; observe the expected failure and commit the red host test (FR-005, FR-006, FR-009, FR-010, SC-003).

### Implementation for User Story 2

- [ ] T020 [US2] Implement and document Model B+ 64K recognised-unavailable validation and typed C++/C construction rejection in `Sources/BeebCore/src/profile.cpp`, `Sources/BeebCore/include/beeb/runtime.hpp`, `Sources/BeebCore/include/beeb_c.h` and `Sources/BeebCore/src/beeb_c.cpp`; pass T017 and unchanged Model B replay, then commit the B+ boundary checkpoint (FR-006, FR-008, FR-009, FR-010, SC-002, SC-006).
- [ ] T021 [US2] Implement and DocC-document the Swift support classification and typed Model B+ 64K construction error in `Sources/BeebKit/BeebMachine.swift`; use `@unknown default` for imported support-state containment, pass T018 and `swift test`, then commit the Swift B+ checkpoint (FR-006, FR-008, FR-009, FR-013, SC-002).
- [ ] T022 [US2] Implement Model B+ 64K selection, actionable accessible rejection and failure-atomic retention of the active Model B runtime in `Sources/BeebDemo/main.swift`; pass T019 and maintained macOS/iOS builds, then commit the host B+ checkpoint (FR-005, FR-006, FR-009, FR-010, FR-012, SC-003).
- [ ] T023 [US2] Build and launch `BeebDemo-macOS`, select Model B+ 64K by keyboard, verify VoiceOver/Accessibility Inspector expose the request and rejection, confirm active Model B remains distinct, return to Model B, and append timed observations to `specs/004-machine-target-profile/evidence/macos-application-observation.md`; validate against the host contract and commit with a Lore message that closes User Story 2 (FR-005, FR-006, FR-009, FR-010, FR-014, SC-003, SC-004).

**Checkpoint**: Both committed identities cross the product boundary, and the
application cannot confuse identity availability with B+ emulation support.

---

## Phase 5: User Story 3 — Reject Unsupported Identities Safely (Priority: P3)

**Goal**: Reject malformed, unknown, duplicate, incompatible, reserved and
future-version values without fallback, partial outputs or active-machine
mutation.

**Independent Test**: Start from a known Model B runtime, submit the complete
invalid fixture matrix at each supported boundary, and prove specific owned
rejections while profile, safe point, digest and caller outputs remain exactly
unchanged.

### Red tests for User Story 3

- [ ] T024 [US3] Add the red C++/C invalid-profile matrix—zero/unknown identifiers, future schema/component versions, count overflow, non-zero reserved/unused slots, unsorted/duplicate expansions, recognised reserved values and incompatible combinations—with output-preservation and active Model B digest assertions in `Tests/test_main.cpp`; observe expected failures and commit the red test (FR-007, FR-008, FR-009, FR-010, SC-002).
- [ ] T025 [P] [US3] Add red Swift tests for owned raw unknown values, malformed/incompatible/unsupported error mapping and unchanged active Model B query after each failed candidate in `Tests/BeebKitTests/BeebMachineTests.swift`; observe expected failures and commit the red test (FR-007, FR-008, FR-009, FR-010, SC-002).
- [ ] T026 [US3] Extend `Tests/TargetProfile/test-boundaries.sh` with red concurrent query/destroy, C output-canary and C++/C/Swift representation-loss probes; observe expected failures and commit the red boundary test (FR-003, FR-008, FR-010, SC-001, SC-002).

### Implementation for User Story 3

- [ ] T027 [US3] Implement canonical ordering/slot validation, duplicate, unknown, future-version, recognised-reserved and incompatible classification in `Sources/BeebCore/src/profile.cpp`; contain construction before mutation in `Sources/BeebCore/src/machine.cpp` and `Sources/BeebCore/src/runtime.cpp`, pass T024 and C1 replay/race gates, then commit the core rejection checkpoint (FR-007, FR-008, FR-009, FR-010, SC-002).
- [ ] T028 [US3] Implement C validation/status translation and success-only output writes in `Sources/BeebCore/src/beeb_c.cpp`, then implement owned Swift translation/localised errors in `Sources/BeebKit/BeebMachine.swift`; pass T025, T026, Swift tests and sanitizers, then commit the cross-language rejection checkpoint (FR-003, FR-007, FR-008, FR-009, FR-010, SC-001, SC-002).
- [ ] T029 [US3] Add the complete identity/support/rejection rationale and future-option non-claims to `docs/code/target-profile.md`, synchronize affected guidance in `docs/code/host-boundary.md`, and extend `Tests/TargetProfile/test-documentation.sh`; pass the focused documentation test and `DOCS_BASE=develop make docs-check`, then commit with a Lore message that closes User Story 3 (FR-007, FR-013, SC-005).

**Checkpoint**: Supported, recognised-unavailable, unknown, incompatible and
malformed inputs are distinguishable and failure-atomic across every boundary.

---

## Phase 6: Acceptance, Documentation and Feature Completion

**Purpose**: Prove all three stories together, record the limited result
honestly and retire the active planning run only after acceptance passes.

- [ ] T030 Run `make test-machine-target-profile`, `make test`, `make sanitize`, `swift test`, `swift build`, `make test-c1`, `make test-c2-portable`, `make test-c2-xcode`, `make format-check`, `DOCS_BASE=develop make docs-check` and `git diff --check`; record exact results and supported ThreadSanitizer status in `specs/004-machine-target-profile/evidence/verification.md`, then commit the aggregate evidence checkpoint (FR-014, SC-001, SC-002, SC-005, SC-006).
- [ ] T031 Rebuild and launch `BeebDemo-macOS` from the acceptance commit, repeat the complete timed Model B → Model B+ 64K rejection → Model B recovery journey with keyboard, VoiceOver and Accessibility Inspector, finalize `specs/004-machine-target-profile/evidence/macos-application-observation.md`, and commit the final observed-application acceptance checkpoint (FR-005, FR-006, FR-009, FR-010, FR-014, SC-003, SC-004).
- [ ] T032 Update only verified claims and ownership in `docs/STATUS.md`, `docs/product/MACHINE_DELIVERY_PLAN.md`, `docs/ARCHITECTURE.md`, `docs/IMPLEMENTATION_CONSTRAINTS.md` and `CHANGELOG.md`; verify live documents distinguish delivered identity transport from unimplemented B+ behavior, run targeted documentation/tooling gates and commit the completion-documentation checkpoint (FR-006, FR-013, SC-005, SC-006).
- [ ] T033 Clear `.specify/feature.json`, move `specs/004-machine-target-profile/` intact to `specs/completed/004-machine-target-profile/`, refresh the managed Spec Kit block in `AGENTS.md`, verify completed numbering, documentation links and `git diff --check`, and commit with a Lore message that explicitly closes the feature phase without an empty duplicate commit.

---

## Dependencies & Execution Order

### Phase Dependencies

- **Phase 1 — Setup**: Starts immediately.
- **Phase 2 — Foundational value**: Depends on T001 and blocks every story.
- **Phase 3 — User Story 1**: Depends on T002-T007; delivers the Model B MVP.
- **Phase 4 — User Story 2**: Depends on the foundational value and Model B active-session path from US1 so no-fallback retention is observable.
- **Phase 5 — User Story 3**: Depends on US1 construction/query and US2 support classification; expands rejection without changing either successful path.
- **Phase 6 — Acceptance/completion**: Depends on all three stories and their committed evidence.

### User Story Dependencies

- **US1 (P1)**: First independently usable slice after the foundation.
- **US2 (P2)**: Reuses US1's active Model B session solely to prove B+ rejection is not fallback; its identity/support contracts remain independently testable.
- **US3 (P3)**: Generalises the rejection matrix after supported and recognised-unavailable reference cases exist.

### Within Every Story

1. Commit each red test after verifying the expected missing behavior.
2. Implement core/value semantics before adapters that consume them.
3. Preserve C output values on all failure paths.
4. Copy values into Swift; do not add host locks or profile caches.
5. Update public/internal documentation with the relevant implementation task.
6. Build and directly observe the application where the story has a UI claim.
7. Make the final story-task Lore commit explicitly close that phase.

## Parallel Opportunities

- T002, T004 and T006 can be prepared in parallel because they own separate C++, C and Swift test files, but merge in task order before implementation.
- After T003, C aggregate declarations in T005 and Swift red value tests in T006 touch different files.
- T018 and the core/C red coverage in T017 are independent test files.
- T025 and T024 are independent Swift versus C++/C red-test work.
- Product observation tasks are deliberately serial because they validate the exact integrated commit produced by earlier tasks.

## Parallel Example: Foundational Value

```text
Task: "Add C++ profile value contract in Tests/TargetProfile/test-contract.sh"
Task: "Add C aggregate contract in Tests/TargetProfile/test-boundaries.sh"
Task: "Add Swift profile value tests in Tests/BeebKitTests/BeebMachineTests.swift"
```

## Parallel Example: Model B+ Rejection

```text
Task: "Add C++/C B+ recognised-unavailable tests in Tests/test_main.cpp"
Task: "Add Swift typed B+ rejection tests in Tests/BeebKitTests/BeebMachineTests.swift"
```

## Implementation Strategy

### MVP First

1. Complete setup and the foundational cross-language value.
2. Complete US1 through T016.
3. Stop and verify explicit Model B construction/query plus the observed macOS
   identity journey before adding B+ behavior.

### Incremental Delivery

1. **Foundation**: Bounded identity value, no support claim.
2. **US1**: Model B constructed, queried and observed end to end.
3. **US2**: B+ 64K recognised, displayed and rejected without fallback.
4. **US3**: Full malformed/unknown/reserved/incompatible rejection matrix.
5. **Completion**: Aggregate gates, repeated direct observation, honest live
   status and archival of the accepted feature run.

## Requirement Coverage Summary

| Requirement group | Primary tasks |
| --- | --- |
| FR-001-FR-003 identity/value transport | T002-T007, T012-T014, T026-T028 |
| FR-004 exact active query | T008-T014 |
| FR-005 accessible application identity | T011, T015-T016, T019, T022-T023, T031 |
| FR-006 B+ identity is not B+ emulation | T017-T023, T032 |
| FR-007 future identities remain representable/reserved | T002-T003, T024-T029 |
| FR-008-FR-010 typed failure/no fallback/no mutation | T009, T017-T29, T031 |
| FR-011 deterministic repeated identity | T002-T003, T008-T014 |
| FR-012 unambiguous human labels | T005-T007, T011, T015, T019, T022 |
| FR-013 maintained documentation | T013-T014, T021, T029, T032 |
| FR-014 automated and observed acceptance | T011, T016, T019, T023, T030-T031 |
| SC-001-SC-002 complete boundary/rejection fixtures | T008-T14, T017-T18, T020-T21, T024-T30 |
| SC-003-SC-004 observable/timed macOS journey | T011, T015-T16, T019, T022-T23, T031 |
| SC-005 documentation completeness | T003, T005, T007, T013-T14, T021, T029-T30, T032 |
| SC-006 Model B regression and honest B+ limit | T008, T012, T017, T020, T030, T032 |

## Notes

- All 33 tasks use the required checkbox, sequential ID, optional `[P]`, story
  label and exact file path format.
- `[P]` means file ownership is independent at that point; it does not waive
  the task-order merge, verification or commit rule.
- Local unsupported ThreadSanitizer is recorded as `N/A`, never as passed.
- No task implements Model B+ machine behavior, profile persistence, firmware
  onboarding, snapshots, media, timing refinement or reserved-profile product
  selection.
