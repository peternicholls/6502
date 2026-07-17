# Feature Specification: Bounded Output Contracts

**Feature Branch**: `003-bounded-output-contracts`

**Created**: 2026-07-17

**Status**: Draft

**Input**: User description: "Prepare everything needed for Phase C2 in the core roadmap: bounded frame, audio and diagnostic contracts; resolve all planning gaps and elevate the Apple host to a committed Xcode project."

**Strand**: core

**Authoritative Context**: [Core roadmap](../../docs/CORE_ROADMAP.md#phase-c2--bounded-frame-audio-and-diagnostic-contracts), [architecture](../../docs/ARCHITECTURE.md), [status](../../docs/STATUS.md), [C1 runtime specification](../002-runtime-ownership/spec.md), and [constitution](../../.specify/memory/constitution.md)

## Constitution Alignment *(mandatory)*

- **Outcome**: Decoupled hosts can consume completed video frames, audio samples, and runtime diagnostics through bounded, deterministic, recoverable contracts without making host timing authoritative; Apple contributors can build and test those products through a committed Xcode project.
- **Boundaries and Non-Goals**: BeebCore owns production and bounded storage behind `MachineRuntime`; C transfers caller-owned frame values and copies audio into caller-provided storage; Swift returns independently owned values. The Xcode project is a host/build surface over the same local package and sources. Metal, AVAudioEngine, UI scheduling, CRT effects, host refresh policy, Xcode Cloud, signing, distribution, and replacement of the Makefile or Swift Package remain outside this slice.
- **Evidence**: C++ contract tests, C ABI boundary tests, Swift lifetime/error tests, empty/full/sustained-production tests, deterministic sequence checks, measured bounded-production runs with explicit duration and tolerances, and clean-checkout `xcodebuild` validation for shared macOS, iOS Simulator, and test schemes.
- **Failure and Recovery**: Full output queues apply the documented deterministic overflow policy; empty audio demand reports an underrun without corrupting state; public output is owned so expired borrowed views cannot occur; diagnostics remain recoverable values rather than exceptions.
- **Content and Legal**: N/A. The feature operates only on generated machine output and does not add ROMs, media, persistence, or external content.
- **Accessibility**: N/A for core-only contracts. Host presentation and assistive technology remain outside the core boundary, while failure diagnostics must remain available to host callers.
- **Dependencies and Complexity**: Reuse the C1 `MachineRuntime`, completed-instruction safe point, structured status taxonomy, existing rendering/audio buffers, C ABI, BeebKit mapping, and local Swift package. No third-party dependency or general scheduler is required; bounded queues and caller-owned results are needed because unbounded or borrowed producer storage would violate the roadmap’s safety and determinism goals. The Xcode project must reference existing sources/products rather than duplicate them.
- **Code Documentation**: Update affected C++, C, and Swift declaration contracts; document private/internal producer, queue, owned-result, and diagnostic abstractions; update runtime ownership, host-boundary, timing, and new output-contract guides; run generated documentation and zero-debt checks.

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Consume completed video frames safely (Priority: P1)

As a decoupled host consumer, I can obtain a completed frame with stable ownership and bounded storage so that presentation can lag or run independently without observing partially rendered or invalidated pixels.

**Why this priority**: Continuous video is the primary C2 capability and unlocks a usable sustained Machine host while directly exercising the ownership boundary.

**Independent Test**: Produce frames from a deterministic workload, consume them slower than production, and verify complete pixels, documented full-queue behavior, stable lifetime, and bounded memory at empty, full, and sustained-production boundaries.

**Acceptance Scenarios**:

1. **Given** a running runtime at a completed-instruction boundary, **When** a frame is completed, **Then** a consumer receives a complete frame identified by a monotonic emulated frame number and cannot observe producer mutation while retaining it.
2. **Given** the frame capacity is full, **When** another frame becomes available, **Then** the documented deterministic overflow policy is applied, the result is reported, and memory remains bounded.
3. **Given** no completed frame is available, **When** a consumer requests one, **Then** it receives a documented empty result without blocking the machine owner indefinitely or mutating machine state.

### User Story 2 - Produce and demand audio through a bounded contract (Priority: P2)

As a host audio consumer, I can request or drain bounded audio samples and learn how much production is needed so that a host ring buffer can remain supplied without allowing host callbacks to control emulated time.

**Why this priority**: Audio is the second continuous output and needs an explicit demand model to prevent either unbounded accumulation or hidden host-clock coupling.

**Independent Test**: Run a deterministic workload against empty, partially filled, full, and sustained audio demand, then verify sample ordering, bounded capacity, deterministic overrun/underrun reporting, and identical results for identical emulated input.

**Acceptance Scenarios**:

1. **Given** available audio samples, **When** a consumer requests up to its bounded capacity, **Then** it receives samples in deterministic order with an explicit count and ownership/lifetime guarantee.
2. **Given** fewer samples are available than requested, **When** a consumer asks for audio, **Then** the contract reports the shortfall as recoverable demand/underrun information and returns only valid samples.
3. **Given** audio production would exceed capacity, **When** the producer reaches the limit, **Then** the documented overrun policy is applied without unbounded memory growth or host wall-clock state transitions.

### User Story 3 - Observe actionable runtime diagnostics (Priority: P3)

As a host or test operator, I can inspect frame number, total emulated cycles, host-observed emulation rate, output demand, and recoverable overrun/underrun diagnostics so that I can respond to output pressure without inferring state from sentinels.

**Why this priority**: Diagnostics make bounded output behavior observable and recoverable across C and Swift instead of leaving hosts to guess why output is empty or discarded.

**Independent Test**: Drive known empty, full, and sustained-production cases and verify diagnostic values, sequencing, status categories, and typed Swift mappings through the public boundaries.

**Acceptance Scenarios**:

1. **Given** a runtime with known emulated progress and output queue state, **When** diagnostics are requested, **Then** the returned snapshot reports those values with an explicit consistency point and no host-clock-derived core state.
2. **Given** an output underrun or overrun, **When** a host reads diagnostics, **Then** it can distinguish the condition, determine whether recovery is possible, and continue using the runtime.

### User Story 4 - Build and test through an Xcode project (Priority: P4)

As an Apple-platform contributor, I can open a committed Xcode project and use shared schemes to build the macOS app, build the iOS Simulator app, and run package tests without manually reconstructing project settings.

**Why this priority**: The output contracts remain the C2 capability, while a real Xcode project makes that capability usable as an Apple application development surface and removes the current package-only IDE setup.

**Independent Test**: From a clean checkout, list the shared schemes and use `xcodebuild` to build macOS and iOS Simulator destinations and run the test scheme; then independently pass `swift build`, `swift test`, and the portable Makefile gates.

**Acceptance Scenarios**:

1. **Given** a clean checkout with supported Xcode installed, **When** a contributor opens `Beeb6502.xcodeproj`, **Then** shared macOS, iOS Simulator, and test schemes are immediately available.
2. **Given** the committed project, **When** its schemes build and test, **Then** they consume the repository’s existing package/core sources without copied source trees or generated absolute paths.
3. **Given** the Xcode project exists, **When** the portable and Swift Package gates run, **Then** they remain independent and continue to pass without requiring the project file.

### Edge Cases

- A consumer retains a frame or audio value while the producer continues; the owned value remains unchanged and does not expose concurrent mutation.
- A runtime is paused, reset, faulted, or shutting down while output is requested; the result is a structured lifecycle status and no partial output is presented as complete.
- Capacity values at zero, one, and configured maximum are validated and cannot cause arithmetic overflow or accidental unbounded allocation.
- Repeated production and consumption of the same deterministic workload yields the same frame identifiers, audio sequence, and diagnostic transitions.
- Concurrent C and Swift consumers contend with production; synchronization remains inside the runtime boundary and no callback runs while runtime synchronization is held.
- Output generation encounters an allocation or conversion failure; the error is contained, recoverable where specified, and never crosses the C ABI as a C++ exception.
- The Xcode project is opened from a different checkout path or on a clean machine; no user-specific workspace state, absolute source path, signing identity, or derived data is required.
- An unsupported or unavailable simulator destination is requested; validation selects an installed generic simulator destination and reports the toolchain limitation without weakening portable gates.

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: The completed-frame contract MUST use packed 8-bit RGBA, an immutable C++ owned value, a caller-owned C value released explicitly, and an independently owned Swift value; no returned frame aliases producer storage.
- **FR-002**: The system MUST make completed frames available only at the C1 completed-instruction/device-tick safe point and MUST never expose partially rendered storage.
- **FR-003**: The frame queue MUST have capacity three, dequeue the oldest retained frame, and on overflow discard exactly the oldest unconsumed frame before enqueuing the newest; reset MUST discard every retained pre-reset frame, and every discard increments a monotonic dropped-frame counter.
- **FR-004**: Audio production MUST use mono 32-bit floating-point samples at 48,000 Hz in a 4,096-sample ring; C++ and Swift consumers receive owned copies, while C copies into caller-provided storage.
- **FR-005**: The audio ring MUST preserve FIFO order, target a 2,048-sample fill, report `max(0, 2048 - available)` demand, discard the oldest samples on overflow while incrementing an exact overrun count, and return all available samples plus the exact shortfall on underrun. Reset MUST discard retained pre-reset samples, count them as overrun so conservation remains exact, and clear fractional audio timing without resetting runtime-lifetime sample identity.
- **FR-006**: Diagnostics MUST provide total emulated cycles, latest frame number, frame/audio depths and capacities, audio demand, dropped-frame/overrun/underrun counts, and data required to calculate host-observed emulation-rate ratio.
- **FR-007**: The system MUST preserve deterministic output ordering and diagnostic transitions for identical initial state, commands, and emulated execution.
- **FR-008**: The system MUST provide structured operation statuses for lifecycle, capacity, empty, invalid-input, and output-production failures, with no C++ exception crossing the C ABI.
- **FR-009**: The C ABI MUST document output-value lifetime, ownership, thread-safety, copying/release requirements, and failure behavior for every new or changed entry point.
- **FR-010**: The Swift wrapper MUST map output results and diagnostic statuses into Swift-owned values and typed errors without requiring a redundant host lock.
- **FR-011**: The system MUST keep output memory bounded under sustained production and MUST provide a reproducible measurement method for capacity, duration, and tolerance claims.
- **FR-012**: The implementation MUST update affected source and conceptual documentation and MUST pass generated documentation, link, markup, and documentation-debt validation.
- **FR-013**: Host-side C and Swift helpers MUST calculate emulation-rate ratio from the delta between two core emulated-cycle observations divided by an explicit positive host-observation interval; host time MUST NOT be stored in or drive BeebCore state.
- **FR-014**: The repository MUST contain `Beeb6502.xcodeproj` with shared schemes that build the macOS app, build the iOS Simulator app, and run package tests from a clean checkout without user-specific or absolute-path data.
- **FR-015**: The Xcode project MUST consume the existing local package/core sources without duplication, and `swift build`, `swift test`, and portable Makefile builds MUST remain independent required gates.

### Key Entities

- **CompletedFrame**: An immutable owned RGBA observation with frame number and dimensions; ownership transfers as an owned value and never aliases producer storage.
- **AudioChunk**: An owned ordered sequence of mono Float32 samples at 48,000 Hz with emulated position and explicit available/requested counts.
- **OutputQueue**: A finite producer/consumer buffer for completed frames or audio chunks with explicit empty/full and overflow policy.
- **OutputDiagnostics**: A consistent observation of emulated progress, frame/audio availability, demand, and recoverable pressure conditions.
- **OutputStatus**: Operation-scoped success, empty, overrun, underrun, lifecycle, and failure result information mapped across C++/C/Swift.
- **EmulationRateObservation**: Two emulated-cycle counters plus an explicit host-observation interval used outside the core to calculate an informational rate ratio.
- **XcodeProjectSurface**: Committed project metadata and shared schemes referencing existing package/core products without owning duplicate source.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: In the documented deterministic workload, every consumed frame is complete and unchanged for its documented lifetime across at least 10,000 produced/consumed frames, with zero ownership or mutation failures.
- **SC-002**: During at least 60 emulated seconds after a 10-second warm-up, frame depth never exceeds three, audio depth never exceeds 4,096 samples, produced output exactly equals consumed plus dropped plus retained output, and process resident memory grows by no more than 16 MiB.
- **SC-003**: Empty, full, and sustained-production contract tests pass deterministically across C++, C ABI, and Swift layers, including repeated runs with identical output identifiers and diagnostic transitions.
- **SC-004**: Audio demand and underrun/overrun diagnostics identify the required recovery action within one output operation and never require host wall-clock time to advance emulated state.
- **SC-005**: The public C and Swift contracts expose no ambiguous sentinel-only output result; all tested failure and lifecycle outcomes map to structured statuses or typed errors.
- **SC-006**: Documentation generation and link/markup/debt gates pass with no new recorded documentation debt, and the C2 quickstart reproduces the bounded-output evidence from a clean build.
- **SC-007**: Emulation-rate helpers return the exact expected ratio within 0.1% for synthetic observation intervals and never change core state.
- **SC-008**: A clean checkout passes the shared Xcode macOS build, generic iOS Simulator build, and test scheme, while the independent Swift Package and portable Makefile gates also pass.

## Assumptions

- C1’s `MachineRuntime`, capacity-64 command FIFO, structured status taxonomy, and completed-instruction/device-tick safe point are accepted as the only supported execution path.
- Output consumers may be slower or faster than production; the contract must remain non-blocking for ordinary polling/draining and must not make host scheduling authoritative.
- This phase does not persist output or machine state, alter the C1 ABI revision policy beyond required output additions, or add host presentation/audio framework integrations.
- The committed Xcode project is maintained source, while `xcuserdata`, derived data, signing credentials, and per-user schemes remain untracked build/user state.
