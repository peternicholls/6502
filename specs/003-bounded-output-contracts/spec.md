# Feature Specification: Bounded Output Contracts

**Feature Branch**: `003-bounded-output-contracts`

**Created**: 2026-07-17

**Status**: Draft

**Input**: User description: "Prepare everything needed for Phase C2 in the core roadmap: bounded frame, audio and diagnostic contracts."

**Strand**: core

**Authoritative Context**: [Core roadmap](../../docs/CORE_ROADMAP.md#phase-c2--bounded-frame-audio-and-diagnostic-contracts), [architecture](../../docs/ARCHITECTURE.md), [status](../../docs/STATUS.md), [C1 runtime specification](../002-runtime-ownership/spec.md), and [constitution](../../.specify/memory/constitution.md)

## Constitution Alignment *(mandatory)*

- **Outcome**: Decoupled hosts can consume completed video frames, audio samples, and runtime diagnostics through bounded, deterministic, recoverable contracts without making host timing authoritative.
- **Boundaries and Non-Goals**: BeebCore owns production and bounded storage behind `MachineRuntime`; C and Swift expose owned or explicitly borrowed views with documented lifetime. Metal, AVAudioEngine, UI scheduling, CRT effects, and host refresh policy remain outside this core slice.
- **Evidence**: C++ contract tests, C ABI boundary tests, Swift lifetime/error tests, empty/full/sustained-production tests, deterministic sequence checks, and measured bounded-production runs with explicit duration, capacity, and tolerance.
- **Failure and Recovery**: Full output queues apply the documented deterministic overflow policy; empty audio demand reports an underrun without corrupting state; invalid or expired views are rejected or impossible by contract; diagnostics remain recoverable values rather than exceptions.
- **Content and Legal**: N/A. The feature operates only on generated machine output and does not add ROMs, media, persistence, or external content.
- **Accessibility**: N/A for core-only contracts. Host presentation and assistive technology remain outside the core boundary, while failure diagnostics must remain available to host callers.
- **Dependencies and Complexity**: Reuse the C1 `MachineRuntime`, completed-instruction safe point, structured status taxonomy, existing rendering/audio buffers, C ABI, and BeebKit mapping. No third-party dependency or general scheduler is required; bounded queues and explicit views are needed because unbounded or host-owned storage would violate the roadmap’s safety and determinism goals.
- **Code Documentation**: Update affected C++, C, and Swift declaration contracts; document private/internal producer, queue, view, and diagnostic abstractions; update runtime ownership, host-boundary, timing, and new output-contract guides; run generated documentation and zero-debt checks.

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

As a host or test operator, I can inspect frame number, emulation progress, output demand, and recoverable overrun/underrun diagnostics so that I can respond to output pressure without inferring state from timing or sentinel values.

**Why this priority**: Diagnostics make bounded output behavior observable and recoverable across C and Swift instead of leaving hosts to guess why output is empty or discarded.

**Independent Test**: Drive known empty, full, and sustained-production cases and verify diagnostic values, sequencing, status categories, and typed Swift mappings through the public boundaries.

**Acceptance Scenarios**:

1. **Given** a runtime with known emulated progress and output queue state, **When** diagnostics are requested, **Then** the returned snapshot reports those values with an explicit consistency point and no host-clock-derived core state.
2. **Given** an output underrun or overrun, **When** a host reads diagnostics, **Then** it can distinguish the condition, determine whether recovery is possible, and continue using the runtime.

### Edge Cases

- A consumer retains a frame or audio view while the producer continues; the view remains valid for its documented lifetime and does not expose concurrent mutation.
- A runtime is paused, reset, faulted, or shutting down while output is requested; the result is a structured lifecycle status and no partial output is presented as complete.
- Capacity values at zero, one, and configured maximum are validated and cannot cause arithmetic overflow or accidental unbounded allocation.
- Repeated production and consumption of the same deterministic workload yields the same frame identifiers, audio sequence, and diagnostic transitions.
- Concurrent C and Swift consumers contend with production; synchronization remains inside the runtime boundary and no callback runs while runtime synchronization is held.
- Output generation encounters an allocation or conversion failure; the error is contained, recoverable where specified, and never crosses the C ABI as a C++ exception.

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: The system MUST define a completed-frame contract with explicit pixel format, dimensions, frame identity, ownership, lifetime, nullability, capacity, and overflow behavior.
- **FR-002**: The system MUST make completed frames available only at the C1 completed-instruction/device-tick safe point and MUST never expose partially rendered storage.
- **FR-003**: The system MUST enforce a finite frame capacity and deterministic behavior at empty, full, and sustained-production boundaries.
- **FR-004**: The system MUST define bounded audio production and consumption with explicit sample format, channel/rate metadata, ownership, lifetime, capacity, and overflow/underrun behavior.
- **FR-005**: The system MUST expose audio demand information sufficient for a host ring buffer without using host wall-clock time to advance core state.
- **FR-006**: The system MUST provide diagnostics for emulated progress, frame identity, output availability/demand, and recoverable overrun/underrun conditions.
- **FR-007**: The system MUST preserve deterministic output ordering and diagnostic transitions for identical initial state, commands, and emulated execution.
- **FR-008**: The system MUST provide structured operation statuses for lifecycle, capacity, empty, invalid-input, and output-production failures, with no C++ exception crossing the C ABI.
- **FR-009**: The C ABI MUST document output view lifetime, ownership, thread-safety, copying/release requirements, and failure behavior for every new or changed entry point.
- **FR-010**: The Swift wrapper MUST map output results and diagnostic statuses into Swift-owned values and typed errors without requiring a redundant host lock.
- **FR-011**: The system MUST keep output memory bounded under sustained production and MUST provide a reproducible measurement method for capacity, duration, and tolerance claims.
- **FR-012**: The implementation MUST update affected source and conceptual documentation and MUST pass generated documentation, link, markup, and documentation-debt validation.

### Key Entities

- **CompletedFrame**: An immutable completed video observation with frame number, dimensions, pixel format, bytes, and lifetime/ownership contract.
- **AudioChunk**: A bounded ordered sequence of samples with sample metadata, emulated position, count, and consumer ownership/lifetime.
- **OutputQueue**: A finite producer/consumer buffer for completed frames or audio chunks with explicit empty/full and overflow policy.
- **OutputDiagnostics**: A consistent observation of emulated progress, frame/audio availability, demand, and recoverable pressure conditions.
- **OutputStatus**: Operation-scoped success, empty, overrun, underrun, lifecycle, and failure result information mapped across C++/C/Swift.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: In the documented deterministic workload, every consumed frame is complete and unchanged for its documented lifetime across at least 10,000 produced/consumed frames, with zero invalid-view observations.
- **SC-002**: Frame and audio storage remain at or below their configured capacities during a sustained production run of at least 60 emulated seconds, with zero unbounded-allocation events.
- **SC-003**: Empty, full, and sustained-production contract tests pass deterministically across C++, C ABI, and Swift layers, including repeated runs with identical output identifiers and diagnostic transitions.
- **SC-004**: Audio demand and underrun/overrun diagnostics identify the required recovery action within one output operation and never require host wall-clock time to advance emulated state.
- **SC-005**: The public C and Swift contracts expose no ambiguous sentinel-only output result; all tested failure and lifecycle outcomes map to structured statuses or typed errors.
- **SC-006**: Documentation generation and link/markup/debt gates pass with no new recorded documentation debt, and the C2 quickstart reproduces the bounded-output evidence from a clean build.

## Assumptions

- C1’s `MachineRuntime`, capacity-64 command FIFO, structured status taxonomy, and completed-instruction/device-tick safe point are accepted as the only supported execution path.
- Initial C2 output capacities and exact frame/audio formats will be selected during research from existing renderer/audio behavior and stated as contract constants or configuration with measured rationale.
- Output consumers may be slower or faster than production; the contract must remain non-blocking for ordinary polling/draining and must not make host scheduling authoritative.
- This phase does not persist output or machine state, alter the C1 ABI revision policy beyond required output additions, or add host presentation/audio framework integrations.
