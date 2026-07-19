# Feature Specification: Machine Target Profile

**Feature Branch**: `004-machine-target-profile`

**Created**: 2026-07-19

**Status**: Draft

**Input**: User description: "Set up the next phase of work selected by the canonical delivery plan."

**Strand**: cross-strand

**Delivery-Plan Trace**: [Specification sequence row 1 — `machine-target-profile`](../../docs/product/MACHINE_DELIVERY_PLAN.md#specification-sequence)

**Supporting Context**: [Product vision](../../docs/product/VISION.md), [current architecture](../../docs/ARCHITECTURE.md), [implementation constraints](../../docs/IMPLEMENTATION_CONSTRAINTS.md#target-profile), and [verified status](../../docs/STATUS.md). [Completed Spec Kit runs](../completed/) are evidence only and add no scope.

## Constitution Alignment *(mandatory)*

- **Outcome**: A user or host can identify a machine target consistently as Model B or Model B+ 64K, observe that identity in the maintained application, and receive a safe rejection for any unsupported identity.
- **Programme Authority**: This feature is exactly the `machine-target-profile` slice selected as **NEXT** by specification-sequence row 1 of the machine delivery plan.
- **Temporal Boundary**: C0-C2 artifacts are completed evidence only. Archived plans and the archived PRD provide historical context only. Neither adds requirements or completion claims. This feature does not change the status ledger until its acceptance evidence passes.
- **Boundaries and Non-Goals**: The machine identity must remain consistent across core configuration, the language-neutral boundary, the application wrapper and host configuration. This slice defines and transports identity; it does not implement Model B+ memory, firmware, display, storage or timing behavior, firmware onboarding, snapshots, or a complete profile-selection experience.
- **Evidence**: Contract and integration tests must cover both committed identities, round trips across every boundary, reserved identifiers and invalid input. The maintained macOS application must be built and launched, each committed profile identity must be selected through the available development configuration, and the displayed identity and rejection/recovery behavior must be recorded. Unit tests alone do not close the feature.
- **Failure and Recovery**: Unknown, malformed, reserved or unsupported identities must be rejected with an actionable result, must not fall back to Model B or another profile, and must leave the active machine identity and state unchanged.
- **Content and Legal**: No firmware or other proprietary bytes are introduced, bundled or inferred. Profile identity is metadata only and does not assert possession or compatibility of firmware.
- **Accessibility**: Any profile identity or rejection shown by the maintained application must be available to keyboard navigation and assistive technology and must not rely on colour or motion alone. Broader profile-selection interaction is deferred to its later product slice.
- **Dependencies and Complexity**: Reuse existing machine configuration, typed failure and boundary-value patterns. The identity model must permit later versioned expansions without a closed two-value design. No new third-party dependency is justified.
- **Code Documentation**: Changed public contracts must document identity meaning, stability, ownership, supported and reserved values, rejection behavior and non-fallback guarantees. Named internal identity types must document their responsibility and invariants. Generated documentation and repository documentation gates must pass.

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Select a Model B Target (Priority: P1)

As a user starting a machine session, I can select Model B and see that exact target identity carried consistently by the running application so later machine workflows have an unambiguous configuration.

**Why this priority**: Model B is the verified baseline and the first working-application milestone depends on a reliable target identity.

**Independent Test**: Start with no active session, select Model B through the maintained development configuration, create the machine, and confirm every observable boundary and the application report the same stable identity.

**Application Observation**: Build and launch the maintained macOS application from its checked-in project, select the Model B development configuration, start a session and record that the visible and assistive description identifies Model B without requiring a shell command during the observed journey.

**Acceptance Scenarios**:

1. **Given** no active machine and a Model B target request, **When** a session is created, **Then** the session is identified as Model B at every observable boundary.
2. **Given** a running Model B session, **When** its target identity is queried repeatedly, **Then** the same stable Model B identity is returned without changing machine state.

---

### User Story 2 - Represent Model B+ 64K Separately (Priority: P2)

As a developer preparing later Model B+ delivery, I can select and transport a distinct Model B+ 64K identity without treating it as Model B or claiming that Model B+ behavior is implemented.

**Why this priority**: The post-C6 developer preview requires a separately researched Model B+ profile, and that work cannot be safely layered on an ambiguous or Model-B-only identity.

**Independent Test**: Select Model B+ 64K, pass the identity through every supported boundary, and confirm it remains distinct from Model B while the product continues to label Model B+ emulation behavior as unavailable where it is not yet implemented.

**Application Observation**: Build and launch the maintained macOS application, select the Model B+ 64K development configuration and record that the application identifies Model B+ 64K distinctly and communicates any unavailable machine behavior without silently starting Model B.

**Acceptance Scenarios**:

1. **Given** a Model B+ 64K target request, **When** the identity crosses all supported boundaries, **Then** each boundary returns the same Model B+ 64K identity and never reports Model B.
2. **Given** Model B+ 64K identity support but no completed Model B+ machine implementation, **When** a user attempts behavior not yet supported, **Then** the application reports the limitation and does not substitute Model B behavior.

---

### User Story 3 - Reject Unsupported Identities Safely (Priority: P3)

As a user or caller, I receive a clear failure when a target identity is unknown, malformed, reserved or unsupported, and my active machine remains unchanged.

**Why this priority**: Extensibility is unsafe if future or corrupt identities are guessed, silently downgraded or partially applied.

**Independent Test**: Begin with a known Model B session, submit each class of invalid or unsupported identity, and verify a specific rejection while the original session identity and observable state remain unchanged.

**Application Observation**: With the maintained macOS application running a Model B development session, attempt an unsupported target through the available development configuration and record the actionable rejection, retained Model B identity, keyboard recovery path and assistive announcement.

**Acceptance Scenarios**:

1. **Given** an active Model B session, **When** an unknown or malformed identity is requested, **Then** the request fails and the active session remains Model B with unchanged observable state.
2. **Given** a reserved B+ 128K, Master-family, Tube, Econet or other expansion identity, **When** it is requested before support is delivered, **Then** the request is rejected as unsupported and no fallback profile is selected.

### Edge Cases

- A syntactically valid identity has a version newer than the application understands.
- A known base machine is combined with an unknown, duplicate or incompatible expansion identifier.
- An identity is empty, truncated, contains out-of-range values or has trailing unrecognised data.
- A reserved identity is confused with a supported identity because their names share a prefix.
- A target change fails after validation begins while another session is active.
- The same supported identity is selected repeatedly.
- A boundary receives a value that it cannot represent without loss.

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: The system MUST define stable, distinct identities for Model B and Model B+ 64K.
- **FR-002**: A machine target MUST consist of a stable base-machine identity plus an explicitly versioned collection of expansion identities, allowing later additions without changing the meaning of existing identities.
- **FR-003**: The target identity MUST remain semantically unchanged across core configuration, the language-neutral boundary, the application wrapper and host configuration.
- **FR-004**: Callers MUST be able to retrieve the exact target identity associated with a successfully created or configured machine.
- **FR-005**: The maintained application MUST present the selected target identity in a form visible to users and available to assistive technology.
- **FR-006**: Model B+ 64K identity support MUST remain distinct from Model B and MUST NOT, by itself, claim or simulate completed Model B+ machine behavior.
- **FR-007**: B+ 128K, Master-family revisions, Tube, Econet, ADFS and other storage or peripheral expansions MUST remain representable as future versioned identities without being reported as supported by this feature.
- **FR-008**: Unknown, malformed, incompatible, reserved or unsupported identities MUST produce a specific recoverable rejection.
- **FR-009**: A rejected target request MUST NOT fall back to Model B, Model B+ 64K or any other target.
- **FR-010**: A rejected target request MUST leave the active machine identity and all previously observable machine state unchanged.
- **FR-011**: Repeated selection of the same supported target MUST be deterministic and MUST NOT create a different identity representation.
- **FR-012**: Human-readable target names MUST unambiguously distinguish Model B, Model B+ 64K and unsupported future options.
- **FR-013**: Public and developer-facing documentation MUST distinguish identity availability from emulation support and MUST state the rejection and non-fallback guarantees.
- **FR-014**: Acceptance evidence MUST include automated boundary coverage and a recorded build, launch and direct observation of the maintained macOS application for both committed identities and one unsupported identity.

### Key Entities

- **Machine Target Profile**: The complete stable identity requested for a machine session, comprising one base machine, its identity version and zero or more explicit expansions.
- **Base Machine Identity**: A stable identifier for a BBC machine family and revision, including committed Model B and Model B+ 64K identities.
- **Expansion Identity**: A versioned identifier for an optional processor, network, memory, storage or peripheral capability; it may be supported, reserved or unknown.
- **Support Status**: The result of validating a target against the current application: supported identity, recognised but unavailable, incompatible or unknown.
- **Target Rejection**: An owned, actionable failure that identifies why a request was refused without changing the active machine.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: All acceptance fixtures for Model B and Model B+ 64K retain the same identity through 100% of the supported configuration boundaries.
- **SC-002**: 100% of unknown, malformed, reserved and incompatible fixture identities are rejected without fallback or any measured change to the active machine identity and observable state.
- **SC-003**: In the maintained macOS application, an observer can distinguish Model B from Model B+ 64K and identify an unsupported target result on the first attempt in each documented acceptance journey.
- **SC-004**: The complete application observation—build, launch, two committed identity selections, one unsupported selection and recovery—can be completed and recorded in no more than 10 minutes on the named validation host.
- **SC-005**: Every public target-profile contract and named internal identity abstraction passes the repository documentation checks with its stability, support and failure semantics documented.
- **SC-006**: Existing Model B regression evidence remains passing, and no acceptance result represents Model B+ identity transport as completed Model B+ emulation.

## Assumptions

- Model B remains the only machine profile with verified emulation behavior at the start of this feature.
- Model B+ 64K becomes a committed identity in this slice, while its machine implementation remains in the later reference-led workstream.
- The maintained macOS application is the observation surface for this cross-strand slice; iPhone and iPad interaction is deferred to the delivery plan's adaptation row.
- Development configuration may expose target choices before the later polished profile-selection workflow, provided the observed application makes identity and unsupported behavior explicit.
- A profile identity is local metadata and does not require firmware, media, networking or a third-party service.
- Snapshot persistence consumes this identity in a later feature and is outside this slice, but the identity defined here must be suitable for that later use without reinterpretation.
