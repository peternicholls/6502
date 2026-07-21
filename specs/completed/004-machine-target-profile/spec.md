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
- **Evidence**: Contract and integration tests must cover both committed identities, round trips across every boundary, unassigned future-option fixtures and invalid input. The maintained macOS application must be built and launched, Model B and Model B+ 64K must both be selected, and the displayed identity plus Model B+ 64K recognised-unavailable rejection/recovery behavior must be recorded. Model B+ 64K is the one unsupported application selection required by this slice; unknown and malformed raw values remain automated boundary fixtures and are not exposed in the product picker. Unit tests alone do not close the feature.
- **Failure and Recovery**: Unknown, malformed, incompatible or recognised-but-unavailable identities must be rejected with an actionable result, must not fall back to Model B or another profile, and must leave the active machine identity and state unchanged. Delivery-plan future options have no assigned identity yet and therefore exercise the unknown path only through non-canonical fixtures.
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

**Application Observation**: Reuse the Model B+ 64K recognised-unavailable journey from User Story 2 to observe product-level rejection, retained Model B identity, keyboard recovery and assistive announcement. Unknown, malformed and unassigned future-option values are exercised through automated development boundaries because the product picker must not present them as supported or selectable profiles.

**Acceptance Scenarios**:

1. **Given** an active Model B session, **When** an unknown or malformed identity is requested, **Then** the request fails and the active session remains Model B with unchanged observable state.
2. **Given** a raw test fixture labelled as a future B+ 128K, Master-family, Tube, Econet or other expansion option but with no public identifier assigned by this feature, **When** it is submitted, **Then** it is rejected as unknown without publishing that fixture code as canonical and without selecting a fallback profile.

### Edge Cases

- Schema version 0 or component identifier/version 0 is malformed; a version newer than version 1 is structurally valid but unknown.
- Expansion count 16 is structurally valid; count 17 or any count beyond the fixed capacity is malformed and must not inspect storage outside the aggregate.
- Expansion entries are unsorted, duplicated, or followed by non-zero unused slots.
- A known base-machine identifier appears in an expansion slot, or another known component is used in the wrong role.
- A structurally valid raw identifier has no public assignment; it remains unknown even when a test labels it as a future B+ 128K, Master, Tube, Econet, ADFS, storage or peripheral option.
- One input has multiple defects; validation reports the deterministic precedence `malformed`, then `unknown`, then `incompatible`, then `recognised unavailable`, without mutation.
- Candidate construction fails after validation while another session is active; the candidate never replaces the active machine.
- The same supported identity is selected repeatedly or queried concurrently with shutdown.
- A caller-owned output contains canary data before a failing call, or a value crosses supported configuration boundaries and risks representation loss.
- Truncated bytes and trailing serialized data are outside this in-memory feature; the later snapshot envelope must specify them before persistence exists.

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: The system MUST define stable, distinct identities for Model B and Model B+ 64K.
- **FR-002**: A machine target MUST consist of a stable base-machine identity plus an explicitly versioned collection of expansion identities, allowing later additions without changing the meaning of existing identities.
- **FR-003**: The target identity MUST remain semantically unchanged across core configuration, the language-neutral boundary, the application wrapper and host configuration.
- **FR-004**: Callers MUST be able to retrieve the exact target identity associated with a successfully created or configured machine.
- **FR-005**: The maintained application MUST present the selected target identity in a form visible to users and available to assistive technology.
- **FR-006**: Model B+ 64K identity support MUST remain distinct from Model B and MUST NOT, by itself, claim or simulate completed Model B+ machine behavior.
- **FR-007**: The bounded base-plus-expansion shape MUST be able to represent later B+ 128K, Master-family, Tube, Econet, ADFS and other storage or peripheral identities once their stable codes are assigned; this feature MUST NOT assign public codes to, recognise, list or report those reserved options as supported.
- **FR-008**: Unknown, malformed, incompatible and recognised-but-unavailable identities MUST produce specific recoverable rejections using the deterministic precedence `malformed`, then `unknown`, then `incompatible`, then `recognised unavailable`.
- **FR-009**: A rejected target request MUST NOT fall back to Model B, Model B+ 64K or any other target.
- **FR-010**: A rejected target request MUST leave the active machine identity and all previously observable machine state unchanged.
- **FR-011**: Repeated selection of the same supported target MUST be deterministic and MUST NOT create a different identity representation.
- **FR-012**: Human-readable names MUST unambiguously distinguish Model B from Model B+ 64K; diagnostics for unassigned raw identifiers MUST use the raw value and `unknown` status rather than inventing a future-option name.
- **FR-013**: Public and developer-facing documentation MUST distinguish identity availability from emulation support and MUST state the rejection and non-fallback guarantees.
- **FR-014**: Acceptance evidence MUST include automated boundary coverage and a recorded build, launch and direct observation of the maintained macOS application for Model B plus Model B+ 64K; the Model B+ 64K recognised-unavailable result is the required unsupported application selection, while unknown/malformed/future-option fixtures remain automated-only.
- **FR-015**: The version-1 envelope MUST structurally admit exactly 0...16 canonically ordered expansion entries for classification, reject counts above 16 without reading beyond fixed storage, require zero reserved and unused fields, and use the edge-case classification precedence defined by FR-008.
- **FR-016**: The in-memory boundary value MUST NOT be treated as a serialized or persisted byte format; snapshot encoding, truncation and trailing-byte rules remain outside this feature.
- **FR-017**: Completion MUST publish the additive target-profile contract as development candidate version 0.4.0 across every user-visible and machine-readable project version surface, with change history that preserves the Model B+ support limitation.

### Key Entities

- **Machine Target Profile**: The complete stable identity requested for a machine session, comprising one base machine, its identity version and zero or more explicit expansions.
- **Base Machine Identity**: A stable identifier for a BBC machine family and revision, including committed Model B and Model B+ 64K identities.
- **Expansion Identity**: A versioned identifier for an optional processor, network, memory, storage or peripheral capability. No expansion identifier is assigned in this feature; non-zero fixture values are unknown until later work allocates a stable code.
- **Support Status**: The result of validating a target: malformed, unknown, incompatible, recognised but unavailable, or supported, using FR-008 precedence.
- **Target Rejection**: An owned, actionable failure that identifies why a request was refused without changing the active machine.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: All acceptance fixtures for Model B and Model B+ 64K retain the same identity through 100% of the supported configuration boundaries.
- **SC-002**: 100% of unknown, malformed, unassigned future-option and incompatible fixture identities are rejected with the FR-008 category precedence, without fallback or any measured change to the active machine identity, pre-existing caller output or observable state.
- **SC-003**: In the maintained macOS application, an observer can distinguish Model B from Model B+ 64K and identify an unsupported target result on the first attempt in each documented acceptance journey.
- **SC-004**: The complete application observation—build, launch, Model B selection, Model B+ 64K recognised-unavailable selection and recovery to Model B—can be completed and recorded in no more than 10 minutes on the named validation host.
- **SC-005**: Every public target-profile contract and named internal identity abstraction passes the repository documentation checks with its stability, support and failure semantics documented.
- **SC-006**: Existing Model B regression evidence remains passing, and no acceptance result represents Model B+ identity transport as completed Model B+ emulation.
- **SC-007**: Boundary fixtures demonstrate that 16 canonically ordered expansion entries are structurally accepted for classification, 17 is rejected before out-of-bounds access, and every multi-defect fixture returns the same category at every supported configuration boundary.
- **SC-008**: Every project version surface reports 0.4.0 and the corresponding change record describes profile identity, safe rejection and the absence of Model B+ emulation support.

## Assumptions

- Model B remains the only machine profile with verified emulation behavior at the start of this feature.
- Model B+ 64K becomes a committed identity in this slice, while its machine implementation remains in the later reference-led workstream.
- The maintained macOS application is the observation surface for this cross-strand slice; iPhone and iPad interaction is deferred to the delivery plan's adaptation row.
- Development configuration may expose target choices before the later polished profile-selection workflow, provided the observed application makes identity and unsupported behavior explicit.
- Model B+ 64K is the sole recognised-but-unavailable identity and the sole unsupported picker choice in this slice; the picker does not expose raw unknown or reserved future options.
- B+ 128K, Master-family, Tube, Econet, ADFS and other future options are names in the delivery plan, not stable profile identifiers assigned by this feature.
- A profile identity is local metadata and does not require firmware, media, networking or a third-party service.
- Snapshot persistence consumes this identity in a later feature and is outside this slice, but the identity defined here must be suitable for that later use without reinterpretation.
