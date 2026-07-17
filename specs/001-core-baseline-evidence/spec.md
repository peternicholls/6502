# Feature Specification: Core Baseline Evidence

**Feature Branch**: `001-core-baseline-evidence`

**Created**: 2026-07-15

**Status**: Complete

**Input**: Execute the full planning phase for C0 baseline evidence from the
canonical emulator core roadmap.

**Strand**: core

**Authoritative Context**:
[Core roadmap C0](../../docs/CORE_ROADMAP.md#phase-c0--baseline-evidence),
[architecture](../../docs/ARCHITECTURE.md), and
[implementation status](../../docs/STATUS.md)

## Constitution Alignment *(mandatory)*

- **Outcome**: Maintainers can reproduce and audit the known-good emulator
  foundation before runtime ownership or buffer-lifetime work begins.
- **Boundaries and Non-Goals**: This slice consolidates evidence for existing
  behavior. It does not increase hardware fidelity, restructure execution,
  introduce new host behavior, or define performance promises.
- **Evidence**: Existing behavioral and public-boundary suites, deterministic
  clean-room boot evidence, provenance-recorded bitmap and Mode 7 references,
  sanitizer results, and repeated comparison measurements.
- **Failure and Recovery**: A mismatch must identify the failing evidence and
  expected versus observed result, exit unsuccessfully, and leave approved
  references unchanged. Regeneration is a separate, explicit maintainer action.
- **Content and Legal**: Every fixture must be redistributable with recorded
  provenance. No proprietary firmware, character generator, game, or user
  media may be required or committed.
- **Accessibility**: This slice adds no product UI. Evidence summaries and
  diagnostics must remain readable as text and must not rely on colour alone.
- **Dependencies and Complexity**: Reuse the existing build, test, clean-room
  fixture, frame-export, and host-boundary surfaces. Documentation tooling may
  be added where it produces maintainable browsable output for the existing
  languages; runtime dependencies and generalized frameworks remain outside
  scope.
- **Code Documentation**: Establish the project-wide documentation standard,
  browsable output, initial public-surface coverage, and a ratchet that requires
  every later coding slice to document changed public contracts and non-obvious
  behavior without restating self-evident code.

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Reproduce the Known-Good Baseline (Priority: P1)

As a maintainer preparing foundational core work, I can run one documented
verification flow and receive an unambiguous result for the current behavioral,
sanitizer, release-version, and public host-boundary evidence.

**Why this priority**: Later phases must distinguish regressions from existing
limitations before changing ownership and timing internals.

**Independent Test**: From a clean supported environment, run the baseline
verification flow and confirm that every required evidence group is attempted,
reported, and reflected in the final result.

**Acceptance Scenarios**:

1. **Given** a clean supported environment and unchanged known-good sources,
   **When** the maintainer runs baseline verification, **Then** every required
   evidence group passes and the final result identifies the verified scope.
2. **Given** one deliberately failing behavioral or public-boundary check,
   **When** baseline verification runs, **Then** the final result fails and
   identifies that check without masking it behind later results.
3. **Given** inconsistent release-version sources, **When** baseline
   verification runs, **Then** it fails before presenting the foundation as
   reproducible.

---

### User Story 2 - Audit Redistributable Boot and Video Evidence (Priority: P2)

As a reviewer, I can verify that a lawful clean-room machine fixture reaches a
named deterministic state and produces representative bitmap and Mode 7 visual
evidence without relying on private firmware.

**Why this priority**: Runtime refactoring needs stable end-to-end machine
evidence beyond isolated component tests, and that evidence must be safe to
distribute and run in continuous verification.

**Independent Test**: Generate the redistributable fixture twice from clean
inputs, run the named workload, and verify identical boot and visual signatures
plus complete provenance records.

**Acceptance Scenarios**:

1. **Given** only redistributable repository inputs, **When** the fixture is
   generated and run, **Then** it reaches the named state with the approved
   exact cycle count or state signature.
2. **Given** representative bitmap and Mode 7 output, **When** it is compared
   with approved evidence, **Then** every selected output matches exactly.
3. **Given** an intentionally altered output or expected signature, **When**
   verification runs, **Then** it reports the mismatch and does not replace the
   approved reference.
4. **Given** any fixture or reference, **When** a reviewer inspects its evidence
   record, **Then** the origin, redistribution basis, generation procedure, and
   intended coverage are present.

---

### User Story 3 - Establish a Reproducible Comparison Baseline (Priority: P3)

As a maintainer, I can capture repeatable throughput measurements for a named
workload so later architectural changes can quantify their impact without
turning the initial measurement into a product guarantee.

**Why this priority**: The bus-cycle and runtime phases need a stable comparison
point, but correctness and determinism remain more important than an arbitrary
speed target.

**Independent Test**: Run the documented workload repeatedly in one recorded
environment and confirm that the result contains the workload identity,
environment, individual samples, median, and range.

**Acceptance Scenarios**:

1. **Given** a supported environment, **When** the maintainer records five
   consecutive measurements of the named workload, **Then** all five samples,
   their median, their range, and the relevant environment details are reported.
2. **Given** an incomplete run or fewer than five valid samples, **When** the
   result is summarized, **Then** it is marked invalid rather than becoming the
   comparison baseline.
3. **Given** a valid baseline record, **When** it is presented in project
   documentation, **Then** it is labelled as descriptive comparison evidence
   rather than a compatibility, latency, or product-performance promise.

---

### User Story 4 - Learn the Core from Browsable Code Documentation (Priority: P4)

As a contributor, I can move from a project documentation landing page to the
C/C++ core, C host boundary, Swift wrapper, and conceptual guides, and learn
what each important surface does, how it works, and which invariants constrain
changes at the level appropriate to this emulator.

**Why this priority**: The baseline must be maintainable before deeper runtime
and timing work begins. Documentation is valuable only if it remains close to
the code, can be browsed as a coherent body of knowledge, and is enforced for
future changes.

**Independent Test**: Generate the documentation from a clean checkout, follow
the landing-page links to representative CPU, machine, C boundary, and Swift
symbols plus a timing or architecture guide, and confirm that an intentionally
undocumented public change is rejected by the documentation quality gate.

**Acceptance Scenarios**:

1. **Given** a clean supported documentation environment, **When** a
   contributor runs the documented generation flow, **Then** it produces a
   browsable landing page linking all language-specific and conceptual output.
2. **Given** a representative public symbol, **When** a contributor opens its
   generated page, **Then** the purpose, contract, ownership or lifetime,
   errors, threading, and relevant invariants are stated where applicable.
3. **Given** non-obvious emulation or timing code, **When** a contributor reads
   its nearby documentation, **Then** the hardware rationale, observable
   behavior, and invariants are explained or linked without narrating obvious
   statements.
4. **Given** a later coding slice, **When** its specification, plan, and tasks
   are reviewed, **Then** documentation impact is either covered by explicit
   work and validation or marked not applicable with a concrete rationale.
5. **Given** a new or changed public contract without the required
   documentation, **When** the documentation quality gate runs, **Then** it
   fails with the affected surface identified.

### Edge Cases

- A supported verification tool is unavailable or reports an unsupported
  environment.
- A full suite passes while a sanitizer or public-boundary suite fails.
- Fixture generation succeeds but produces a different byte sequence or state
  signature from the approved reference.
- A visual mismatch occurs outside the intended representative region or mode.
- Existing evidence depends accidentally on a generated build artifact that is
  absent from a clean checkout.
- A measurement is interrupted, produces a zero duration, or lacks enough
  environment metadata to compare later.
- An approved reference genuinely needs replacement after an intentional
  behavior change; ordinary verification must not perform that replacement.
- Documentation generation is attempted where one language's generator is
  unavailable, or a generated cross-link targets output that was not built.
- A symbol is technically public in a header but is not intended as a supported
  host contract, or an internal helper contains complex hardware behavior that
  needs explanation despite not being public.
- A comment becomes inaccurate after a contract, timing rule, or ownership
  invariant changes while its syntax remains valid.

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: The project MUST provide one documented verification flow that
  covers every evidence group required by C0 and returns a single successful or
  unsuccessful result.
- **FR-002**: The verification flow MUST preserve the detailed failure result
  for each attempted evidence group and MUST NOT report overall success when a
  required group fails or is skipped unexpectedly.
- **FR-003**: The evidence set MUST cover current behavioral correctness,
  memory and undefined-behavior safety, synchronized release-version reporting,
  and recoverable failures at every supported public host boundary.
- **FR-004**: The project MUST provide a redistributable clean-room fixture that
  reaches a named deterministic machine state with an exact approved signature.
- **FR-005**: The project MUST maintain exact visual evidence for at least one
  representative bitmap output and one representative Mode 7 output.
- **FR-006**: Every fixture and approved reference MUST record its origin,
  redistribution basis, generation procedure, intended coverage, and expected
  signature.
- **FR-007**: Normal verification MUST treat approved references as immutable;
  updating a reference MUST require a distinct, explicit maintainer action.
- **FR-008**: Evidence mismatches MUST report the evidence identity and expected
  versus observed result in text and MUST cause an unsuccessful final result.
- **FR-009**: The project MUST define one named comparison workload and record
  at least five valid samples, their median, their range, and sufficient
  environment details for later comparison.
- **FR-010**: A measurement record MUST be rejected as a baseline if the
  workload is incomplete, fewer than five samples are valid, or required
  environment details are absent.
- **FR-011**: Performance evidence MUST be labelled as a descriptive comparison
  baseline and MUST NOT be used as a product guarantee or as evidence of
  compatibility.
- **FR-012**: C0 verification MUST require no proprietary firmware, character
  generator, game, or user media.
- **FR-013**: The final evidence summary MUST distinguish verified current
  behavior from known limitations and planned work.
- **FR-014**: All evidence generation and verification procedures MUST work
  from a clean checkout without relying on untracked build artifacts.
- **FR-015**: The project MUST define a code-documentation strategy that
  distinguishes public contract documentation, conceptual architecture and
  timing guidance, rationale for non-obvious implementation behavior, and code
  that is sufficiently clear without commentary.
- **FR-016**: The project MUST generate browsable documentation for the C/C++
  core and C boundary, the Swift wrapper, and conceptual guides from one
  documented entry point with a unified landing page.
- **FR-017**: Every supported public C, C++, and Swift symbol MUST document its
  purpose and, where applicable, parameters, results, ownership, lifetime,
  errors, threading expectations, side effects, and invariants.
- **FR-018**: Non-obvious hardware, timing, state-transition, and buffer
  behavior MUST document its rationale, observable effect, and invariants close
  to the implementation or link to an authoritative conceptual guide.
- **FR-019**: Documentation MUST avoid comments that merely restate names,
  types, control flow, or otherwise self-evident implementation details.
- **FR-020**: Documentation generation MUST diagnose invalid markup, broken
  internal links, and missing required public-contract documentation, and MUST
  fail the quality gate when a new or changed surface violates the standard.
- **FR-021**: C0 MUST record existing documentation debt separately from its
  required initial coverage, and future changes MUST NOT increase that debt.
- **FR-022**: Every later coding specification, plan, and task set MUST address
  code-documentation impact or provide a concrete not-applicable rationale.
- **FR-023**: Generated documentation output MUST be reproducible from tracked
  sources and MUST NOT be treated as an authoritative tracked source.

### Key Entities

- **Evidence Group**: A required category of proof, its verification action,
  result, and relationship to C0 exit evidence.
- **Redistributable Fixture**: Lawfully distributable input or generator with a
  stable identity, provenance record, and named deterministic workload.
- **Approved Reference**: Immutable expected state or visual signature with its
  generation procedure, scope, and review history.
- **Baseline Run**: One execution of the C0 verification flow containing the
  environment, evidence-group results, failures, and overall outcome.
- **Measurement Record**: The named workload, environment description,
  individual samples, median, range, validity, and non-guarantee label.
- **Documentation Surface**: A public contract, complex implementation area,
  or conceptual topic with its source, audience, required detail, generated
  destination, and validation status.
- **Documentation Debt Record**: A bounded inventory of existing uncovered
  surfaces, its rationale and priority, and the rule preventing new or changed
  code from increasing the inventory.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: From a clean checkout on each supported environment, 100% of the
  required C0 evidence groups are attempted and reflected in one final result.
- **SC-002**: Ten consecutive clean-room fixture executions produce the same
  approved boot state signature.
- **SC-003**: 100% of the selected bitmap and Mode 7 references match exactly
  across ten consecutive fixture executions, and a deliberate one-unit mismatch
  is detected.
- **SC-004**: 100% of fixtures and approved references have complete origin,
  redistribution, generation, coverage, and signature records.
- **SC-005**: A comparison baseline contains at least five valid samples plus
  the median, range, workload identity, and required environment details.
- **SC-006**: Deliberate failures in behavior, safety, release-version,
  public-boundary, fixture, and reference evidence each cause an unsuccessful
  final result with the failing evidence identified.
- **SC-007**: No C0 verification path requires or creates proprietary firmware,
  character generator, game, or user-media content.
- **SC-008**: A reviewer can reproduce the documented C0 evidence from a clean
  checkout without using any pre-existing untracked artifact.
- **SC-009**: One documented command produces a browsable landing page from a
  clean checkout, with valid links to representative CPU, machine, C boundary,
  Swift wrapper, architecture, and timing documentation.
- **SC-010**: 100% of the supported public C boundary, public Swift wrapper,
  and public C++ header surfaces have the required contract documentation or an
  explicit internal-only classification recorded by C0.
- **SC-011**: Documentation validation reports zero invalid-markup and broken
  internal-link errors, and a deliberately undocumented changed public symbol
  causes an unsuccessful result identifying that surface.
- **SC-012**: 100% of later generated feature specifications, plans, and task
  sets contain a documentation-impact decision, explicit work where needed,
  and a documentation-generation validation task when code is changed.

## Assumptions

- C0 establishes evidence for existing behavior; any discovered emulator defect
  is recorded separately rather than silently fixed inside this feature.
- The existing clean-room demonstration fixture is the starting point for the
  redistributable boot evidence unless planning shows it cannot reach a stable,
  meaningful state.
- Selected bitmap and Mode 7 evidence represents current deterministic output;
  broader visual coverage remains pull-based and compatibility-led.
- Performance measurements are descriptive comparison data. C0 does not set a
  release threshold or user-facing performance claim.
- Supported environments are the environments already exercised by current
  project verification and continuous integration.
- Ordinary evidence verification is non-destructive; approved-reference updates
  are reviewed separately with an explanation of the intended behavior change.
- Build-time documentation generators are acceptable C0 dependencies because
  the repository contains C/C++ and Swift public surfaces that require native,
  cross-referenced API documentation; they must not become runtime dependencies.
- C0 documents all supported public contracts and representative complex core
  behavior. A reviewed debt inventory may defer unchanged internal surfaces,
  but any later change to a deferred surface must document it before delivery.
