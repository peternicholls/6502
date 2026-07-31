# Feature Specification: Running Model B Workflow

**Feature Branch**: `005-model-b-workflow`

**Created**: 2026-07-31

**Status**: Ready for implementation

**Input**: User description: "Deliver the first running Model B application workflow."

**Strand**: cross-strand

**Delivery-Plan Trace**: [Critical-path slice 2 — `machine-model-b-workflow`](../../docs/product/MACHINE_DELIVERY_PLAN.md#critical-path-slices)

**Supporting Context**: [Product vision](../../docs/product/VISION.md), [current architecture](../../docs/ARCHITECTURE.md), [implementation constraints](../../docs/IMPLEMENTATION_CONSTRAINTS.md), and [verified status](../../docs/STATUS.md). [Completed Spec Kit runs](../completed/) are labelled evidence only; they do not add scope.

## Constitution Alignment *(mandatory)*

- **Outcome**: A macOS user can import and retain access to their own compatible Model B operating-system and language ROMs, boot Model B to BASIC, type and run one documented short BASIC program using an accessible physical keyboard, receive continuous video, and use run, pause, reset and BREAK with actionable status.
- **Programme Authority**: This is the **NEXT** `machine-model-b-workflow` row in the [Machine delivery plan](../../docs/product/MACHINE_DELIVERY_PLAN.md#critical-path-slices).
- **Temporal Boundary**: C0-C2 and target-profile artifacts are completed evidence only. This feature neither claims M1 completion nor alters verified status until its own acceptance evidence passes.
- **Boundaries and Non-Goals**: The feature joins the existing Model B core, runtime owner, C boundary, Swift wrapper and macOS application only where the boot/type/run/video/control journey requires them. It does not add AVAudioEngine output, snapshots or lifecycle restoration, iOS/iPadOS adaptation, disc/tape workflows, Model B+ behavior, timing refinement, inspection, editor transformation, bundled firmware, a second runtime owner or host-driven emulated time.
- **Evidence**: Begin each behavior with focused automated evidence. At feature closure run affected core, C, Swift, application-build and documentation checks, then build and launch the maintained macOS application to observe the changed journey with keyboard and assistive technology. The complete M1 audio-inclusive journey remains the later M1 gate.
- **Failure and Recovery**: Missing, inaccessible, malformed, incompatible or unassignable firmware reports an actionable result without changing a working session. Unsupported profile requests retain the active Model B session. Runtime command failures stop only the affected run state and leave the user able to recover by correcting firmware or using the documented controls.
- **Content and Legal**: Firmware remains user-owned, local and external to the repository. The application imports private copies for machine use, never bundles ROM bytes or silently changes source files, and remembers an assignment only while future authorised access remains available; otherwise it asks the user to locate the file again.
- **Accessibility**: The firmware flow, display, keyboard path, controls and diagnostics are keyboard-operable and expose useful labels, values and failures to assistive technology. No outcome depends solely on colour or motion.
- **Dependencies and Complexity**: Reuse `MachineRuntime`, `BeebMachine`, existing owned frame output, profile selection and typed status patterns. No new third-party dependency, second lifecycle owner or new core abstraction is justified.
- **Code Documentation**: Changed public contracts document ownership, accepted data, failures, threading and recovery. Changed named internal types document their responsibility and invariants. The existing conceptual boundary guides and generated documentation are updated only if their contract changes.

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Import firmware and reach BASIC (Priority: P1)

As a Model B user with lawful ROM files, I can select compatible operating-system and language ROMs, see which assignments are active, and boot to a BASIC-ready state without using shell commands.

**Why this priority**: Firmware onboarding is the first missing bridge between the verified core and a usable application.

**Independent Test**: On a clean application session, import lawful fixture-equivalent ROM data through the product path, assign it to Model B, reset the machine and prove a BASIC-ready observation without using a shell command during the observed journey.

**Application Observation**: Build and launch the maintained macOS application, import user-supplied OS and language ROM files through the application, confirm accessible active-assignment status, and observe the machine reaching BASIC-ready output.

**Acceptance Scenarios**:

1. **Given** a supported Model B session and compatible user-selected OS and language ROMs, **When** the user imports and assigns both, **Then** the application identifies the active assignments and the machine reaches BASIC-ready output after reset.
2. **Given** a previously accepted assignment whose authorised source access is still available, **When** the user starts a new application session, **Then** the application offers that assignment without requiring a shell command.
3. **Given** an OS or language ROM that cannot be accepted or accessed, **When** the user attempts assignment, **Then** the application explains what must be corrected and does not replace a working assignment or active session.

---

### User Story 2 - Type and run a BASIC program (Priority: P2)

As a user at the Model B BASIC prompt, I can use an accessible physical keyboard to enter the documented short BASIC program, run it and see its documented visual result continuously presented by the application.

**Why this priority**: The program journey is the first direct proof that firmware, runtime ownership, input and frame presentation form a usable machine rather than isolated subsystems.

**Independent Test**: With accepted firmware already assigned, replay the documented physical-key sequence through the supported input path and verify the expected program output plus successive owned video observations.

**Application Observation**: Launch the maintained macOS application with the assigned Model B firmware, place input focus on the machine, type and run the documented program using a physical keyboard, and record the visible program result and keyboard/assistive interaction.

**Acceptance Scenarios**:

1. **Given** a BASIC-ready Model B session, **When** the user types the documented program and its run command through the physical keyboard path, **Then** the emulated machine receives the corresponding input and produces the documented result.
2. **Given** a running program that produces display output, **When** the application presents completed frames, **Then** the displayed image updates without using host time to advance emulated execution.
3. **Given** the machine view has keyboard focus, **When** an assistive-technology or keyboard user needs to understand the input state, **Then** the application exposes an actionable focus and status description.

---

### User Story 3 - Control and recover a running session (Priority: P3)

As a user running Model B software, I can run, pause, reset or press BREAK, understand the resulting state, and recover from a failed operation without losing a valid configured session.

**Why this priority**: Controls and recovery make the boot/type/run journey usable during ordinary experimentation rather than a one-shot demonstration.

**Independent Test**: Exercise each control against a configured Model B session, verify owner-serialized state changes and output-epoch behavior, and inject recoverable failures without replacing the active machine or firmware assignment.

**Application Observation**: During the macOS boot/type/run journey, invoke run, pause, reset and BREAK using keyboard-operable controls, confirm their visible and assistive status, and intentionally attempt one recoverable firmware or runtime error.

**Acceptance Scenarios**:

1. **Given** a configured Model B session, **When** the user runs or pauses it, **Then** the application reports the resulting state and remains responsive.
2. **Given** a running or paused Model B session, **When** the user resets it or presses BREAK, **Then** the request completes at the documented safe boundary and stale output is not presented as current output.
3. **Given** an operation fails recoverably, **When** the user corrects the reported input or invokes a documented control, **Then** the configured Model B session remains available without silent fallback to another profile.

### Edge Cases

- Only Model B is constructible; a Model B+ 64K request remains recognised-unavailable and must not be enabled as an alternate firmware target.
- The user supplies an empty, unreadable, inaccessible, wrongly sized or otherwise incompatible ROM, or cancels import after selecting a file.
- The user selects a language ROM that cannot be assigned to an available Model B bank, or changes one assignment while the machine is running.
- A remembered assignment is missing or its prior access is no longer authorised.
- The user invokes run, pause, reset or BREAK repeatedly, while a frame update is pending, or after a recoverable runtime failure.
- Keyboard focus is absent, the application is inactive, a modifier key is held, or a key press/release pair arrives out of order.
- A completed frame is unavailable, stale after reset/BREAK, malformed for host presentation or arrives while the application is recovering.
- Diagnostics contain an unknown profile or status category; the application preserves a safe, actionable generic explanation rather than treating it as success.

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: The application MUST let a user import, identify and assign compatible user-owned Model B operating-system and language ROMs without shell commands or bundled proprietary bytes.
- **FR-002**: Firmware acceptance MUST be profile-aware, distinguish operating-system from language-ROM assignment, and provide an actionable typed result for missing, inaccessible, malformed, incompatible or unassignable input.
- **FR-003**: The application MUST remember a successful Model B firmware assignment only when it can later regain authorised user access; a missing or inaccessible remembered source MUST require user recovery and MUST NOT silently substitute other content.
- **FR-004**: A failed import or assignment MUST leave the active Model B session and its last working firmware configuration unchanged.
- **FR-005**: With accepted firmware, a user MUST be able to reset Model B and reach a documented BASIC-ready state.
- **FR-006**: The application MUST present the active profile and firmware-assignment state in visible, keyboard-accessible and assistive-technology-readable form.
- **FR-007**: The application MUST translate the documented physical-key sequence required for the accepted BASIC program into the machine input path without adding a second direct mutation path to core state.
- **FR-008**: The application MUST present completed video output continuously while the machine runs, and host presentation MUST NOT advance emulated time.
- **FR-009**: The application MUST provide separate, keyboard-operable run, pause, reset and BREAK controls with visible and assistive state feedback.
- **FR-010**: Control, firmware and presentation failures MUST be reported as actionable states; recovery MUST preserve the configured Model B identity and never fall back to Model B+ or another profile.
- **FR-011**: Runtime mutation and observation MUST continue through the existing owner and completed-instruction/device-tick boundary; no UI object may borrow or mutate live core state directly.
- **FR-012**: The feature MUST add no audio-device output, persistence/snapshot behavior, mobile adaptation, media workflow, Model B+ behavior, timing refinement, inspection/editor capability or new third-party dependency.
- **FR-013**: Acceptance evidence MUST include focused automated firmware, input, lifecycle and frame-presentation coverage; affected C/C++/Swift boundary checks; and a recorded macOS build, launch and direct observation of the three user stories with keyboard and assistive technology.

### Key Entities

- **Firmware Assignment**: The user-approved association of one externally supplied ROM with the active Model B operating-system or language role, including displayable identity, availability and recovery state.
- **Model B Session**: One configured supported machine identity, its assigned firmware and owner-serialized execution state.
- **Machine Input Focus**: The user-visible state that directs documented physical-key actions to the active machine without bypassing the runtime owner.
- **Workflow Diagnostic**: An actionable user-facing explanation of firmware, control, profile or presentation state that preserves the underlying typed result.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: On a named maintained macOS host, a user can complete the documented import, Model B boot and BASIC-ready journey entirely through the application; the acceptance record identifies the ROM roles, profile, inputs and observed result.
- **SC-002**: On the same host, a user can type `10 PRINT "BEEB6502"`, press Return, type `RUN`, press Return, through the physical-keyboard path, and the observed display includes `BEEB6502`.
- **SC-003**: During the recorded program run, the application presents at least two successive completed frames with distinct sequence identities or documented unchanged-image justification, while the machine advances only through its runtime owner.
- **SC-004**: The recorded macOS journey demonstrates run, pause, reset and BREAK plus one recoverable failure; each leaves the application responsive and the active profile correctly identified as Model B.
- **SC-005**: Automated tests cover accepted and rejected firmware assignment, keyboard input ordering, control recovery, stale-output handling and C/C++/Swift failure propagation relevant to changed boundaries.
- **SC-006**: Keyboard and assistive-technology observation confirms the firmware flow, machine input focus, controls and diagnostics expose actionable names, values and failures without relying on colour or motion alone.

## Assumptions

- The user supplies lawful OS and language ROM files compatible with the existing Model B foundation; the repository remains free of proprietary ROM bytes.
- Model B is the only constructible profile throughout this feature; Model B+ 64K remains a separately displayed but unavailable request.
- Compatibility is an observable Model B result, not file-type inference: the OS image is exactly 16 KiB, the selected language ROM is one through 16 KiB in an available bank, and the accepted pair must reset to the BASIC prompt and execute the documented program during direct observation. Automated tests use synthetic or clean-room fixtures and do not claim firmware authenticity.
- The documented acceptance program is `10 PRINT "BEEB6502"` followed by `RUN`; its expected result is a visible `BEEB6502` line. The maintained physical-key mapping documents the required key presses and releases without adding input scope.
- The maintained first acceptance platform is macOS with a physical keyboard. iPhone and iPad adaptation remain later committed work.
- Audio device presentation is the separately planned `machine-audio-output` slice; this feature may preserve the existing bounded audio producer but does not make audio a user-facing acceptance claim.
