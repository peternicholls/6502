# Feature Specification: Model B Acceptance Automation

**Feature Branch**: `007-model-b-acceptance-automation`

**Created**: 2026-08-01

**Status**: Ready

**Input**: User request to automate as much of the remaining Model B acceptance
as possible before human keyboard/UI observation.

**Strand**: cross-strand

**Delivery-Plan Trace**: [M1 acceptance closure and
`machine-model-b-workflow`](../../docs/product/MACHINE_DELIVERY_PLAN.md#critical-path-slices)

**Supporting Context**: [Current status](../../docs/STATUS.md),
[architecture](../../docs/ARCHITECTURE.md),
[implementation constraints](../../docs/IMPLEMENTATION_CONSTRAINTS.md), and
[desktop experience direction](../../docs/product/DESKTOP_EXPERIENCE.md).
Completed 005 artifacts are evidence only.

## Constitution Alignment

- **Outcome**: Automated production-path evidence separates machine/runtime
  correctness from the remaining human AppKit keyboard and visual observations.
- **Programme Authority**: The M1 `machine-model-b-workflow` acceptance-closure
  row explicitly names direct input and recoverable-failure evidence as NEXT.
- **Temporal Boundary**: Completed 005 artifacts are frozen evidence and are not
  edited or used to add new scope.
- **Boundaries and Non-Goals**: Tests exercise `MachineRuntime`, C ABI,
  `BeebMachine`, generated lawful fixtures and the existing headless host. This
  slice does not implement the AppKit host, interactive Terminal frontend,
  audio-device presentation or direct user keyboard observation.
- **Evidence**: Swift wrapper tests prove production firmware failure
  preservation, owner-serialized key input, completed frames and audio. C++ and
  headless checks retain portable CPU/device evidence. An evidence note
  distinguishes automated results from human-only gates.
- **Failure and Recovery**: Invalid ROM candidates leave the previously loaded
  machine usable; bounded runtime failures remain typed and recoverable. Test
  failures identify the missing automated contract rather than weaken it.
- **Content and Legal**: No proprietary ROM or user media is committed. Tests
  use synthetic clean-room fixtures and any user ROMs remain local and ignored.
- **Accessibility**: Automated checks do not claim keyboard/assistive
  acceptance. Direct AppKit observation remains explicitly assigned to the user.
- **Dependencies and Complexity**: Reuse existing BeebMachine tests,
  `testlib.sh`, `make test-model-b-workflow` and the C++ headless runner; add no
  dependencies or parallel emulation implementation.
- **Code Documentation**: New test helpers and acceptance scripts explain the
  production boundary and evidence limits. Run `make docs-check` and focused
  Swift/C++ tests.

## User Scenarios & Testing

### User Story 1 - Prove production input/output behavior (Priority: P1)

As a developer, I want a deterministic production-runtime sequence that submits
BBC matrix key events and observes output so that keyboard, video and audio
behavior is tested without a GUI.

**Why this priority**: Both the AppKit and Terminal hosts depend on this shared
machine path.

**Independent Test**: Run the focused Swift test against a generated ROM fixture;
it submits a press/release sequence, completes frames and audio through
`BeebMachine`, and verifies owned output and safe-point progress.

**Application Observation**: N/A for GUI acceptance. This is a production-path
automated observation; direct physical keyboard and accessibility observation
remain human gates.

**Acceptance Scenarios**:

1. **Given** a paused Model B with a valid synthetic OS fixture, **When** the
   test submits the documented matrix press/release sequence and bounded
   execution, **Then** the runtime reaches a later safe point and produces an
   owned completed frame and audio result.
2. **Given** the same sequence is repeated on a fresh runtime, **When** outputs
   are captured, **Then** frame identity, pixels, audio metadata and diagnostics
   match exactly.

### User Story 2 - Prove failure-atomic firmware recovery (Priority: P1)

As a developer, I want an automated invalid-firmware check so that future
Settings and interlock work cannot discard a working machine after a bad import.

**Why this priority**: The safety direction depends on preserving active state
when a candidate configuration is rejected.

**Independent Test**: Load a valid OS fixture, capture state, reject an invalid
candidate, then reset/run and verify the valid machine remains usable.

**Application Observation**: N/A for GUI acceptance. The user must still perform
one direct macOS recovery observation for T008.

**Acceptance Scenarios**:

1. **Given** a valid loaded Model B OS, **When** an invalid candidate is
   rejected, **Then** the previous runtime profile, safe point and firmware
   behavior remain usable.
2. **Given** an invalid candidate is rejected repeatedly, **When** the valid
   machine is reset and run, **Then** no partial candidate state is observed.

### User Story 3 - Produce a repeatable terminal-style evidence command (Priority: P2)

As a developer, I want one scriptable command to run the portable headless
machine and emit stable frame/state evidence so that Terminal mode can later
grow from an already useful regression surface.

**Why this priority**: It automates broad CPU/device/emulation checks without
pretending the interactive Terminal frontend already exists.

**Independent Test**: Run the focused shell contract against a generated
clean-room ROM and verify successful status, non-empty frame and stable
machine-state output.

**Application Observation**: N/A for AppKit. The command is a portable host
observation and does not claim terminal UI completion.

**Acceptance Scenarios**:

1. **Given** a clean-room generated ROM and portable headless executable,
   **When** the script runs a bounded workload, **Then** it exits successfully
   and writes a non-empty deterministic frame/state artifact.

## Edge Cases

- An invalid ROM must be rejected before it changes a valid machine.
- A runtime with no completed frame must report an explicit empty result rather
  than making the test pass on stale output.
- A repeated deterministic run must not depend on host wall-clock timing.
- Missing local proprietary ROMs must skip only the human ROM-backed step, not
  invalidate portable synthetic-fixture tests.

## Requirements

### Functional Requirements

- **FR-001**: The production Swift runtime test MUST submit bounded matrix key
  press/release events through `BeebMachine.setKey`.
- **FR-002**: The production runtime test MUST verify later safe-point progress
  and owned frame/audio observations without advancing from a host timer.
- **FR-003**: Repeated deterministic fixture runs MUST produce equal owned
  output digests and diagnostics.
- **FR-004**: Invalid firmware candidates MUST leave the previously valid
  machine usable and preserve its profile and safe-point behavior.
- **FR-005**: The portable evidence script MUST fail on missing, empty or stale
  frame/state artifacts and return a non-zero process status.
- **FR-006**: No test or fixture MUST add proprietary ROM bytes to Git.
- **FR-007**: Evidence output MUST label automated results separately from the
  remaining human keyboard, visual and assistive-technology observations.

### Key Entities

- **Production input sequence**: Ordered matrix key press/release events with
  bounded execution and release guarantees.
- **Owned output observation**: Frame identity/pixels, audio result, diagnostics
  and safe-point values copied out of one runtime.
- **Acceptance evidence record**: A dated record separating automated checks,
  required local-ROM checks and human-only observations.

## Success Criteria

### Measurable Outcomes

- **SC-001**: Focused production-runtime tests pass on a clean checkout without
  proprietary ROMs or GUI automation.
- **SC-002**: Two fresh deterministic fixture runs produce identical frame,
  audio-metadata and diagnostics observations.
- **SC-003**: A rejected firmware candidate produces no active-machine state
  change detectable through profile, safe point, reset or bounded execution.
- **SC-004**: The portable evidence script returns non-zero for missing/empty
  output and zero only when all required artifacts are valid.
- **SC-005**: Evidence explicitly leaves the direct human typed-program,
  visual, physical-key and assistive-technology gates open.

## Assumptions

- Synthetic clean-room ROM fixtures are sufficient for portable automated
  machine evidence; user-owned ROMs are required for final direct M1
  acceptance observation.
- The existing `BeebMachine`, C ABI, C++ headless runner and test fixtures are
  reused; no new dependency is needed.
- This slice does not claim an interactive terminal UI, AppKit migration,
  AVAudioEngine presentation or iOS behavior.
- The user will perform the remaining human physical-key and visual checks after
  automated evidence is complete.
