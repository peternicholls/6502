# Feature Specification: Phase C1 Runtime Ownership

**Feature Branch**: `002-runtime-ownership`

**Created**: 2026-07-15

**Status**: Draft

**Input**: User description: "Setup full planning for phase C1."

**Strand**: core

**Authoritative Context**: [Core roadmap C1](../../docs/CORE_ROADMAP.md#phase-c1--runtime-ownership-and-recoverable-boundaries), [architecture](../../docs/ARCHITECTURE.md), and [verified implementation status](../../docs/STATUS.md)

## Constitution Alignment *(mandatory)*

- **Outcome**: One runtime owner serializes every machine command, completes
  execution and host transactions only at a documented quiescent safe point,
  and returns recoverable failures consistently through C++, C, and Swift.
- **Boundaries and Non-Goals**: This slice owns execution state, command
  ordering, safe-point semantics, and error transport through the supported
  host boundary. It does not add frame/audio queues, snapshots, editor
  mutation, watchpoints, source debugging, Metal presentation, audio-device
  integration, or bus-cycle timing.
- **Evidence**: Deterministic C++ state-machine and ordering tests, C ABI
  contract tests, Swift concurrency/error tests, repeated command-sequence
  comparisons, race-focused stress tests, supported sanitizer runs, and the
  unchanged C0 aggregate evidence prove the behavior. No new hardware-fidelity
  or performance claim is made.
- **Failure and Recovery**: Invalid commands and inputs return stable error
  codes plus diagnostics without mutating machine state. Execution faults stop
  at a completed-instruction boundary, preserve a queryable failure, and allow
  reset or destruction; no C++ exception crosses the C ABI and Swift callers
  receive typed errors.
- **Content and Legal**: C1 does not add bundled content or file access. ROM and
  disc inputs remain caller-supplied, are copied at the command boundary, and
  writable media remains a private in-memory copy with no implicit export.
- **Accessibility**: N/A for this core-only capability. It introduces no user
  interface, animation, input mapping, or assistive-technology surface.
- **Dependencies and Complexity**: Reuse the C++20 standard library, the
  existing opaque C handle, and the existing Swift ownership wrapper. Add no
  dependency, host framework to the core, general scheduler, or callback
  system; the smallest sufficient abstraction is one machine-scoped command
  queue and owner.
- **Code Documentation**: Update public C++ runtime types, every affected C ABI
  contract, Swift lifecycle/error APIs, and all new or changed private/internal
  named types and interfaces, plus `docs/ARCHITECTURE.md`,
  `docs/code/host-boundary.md`, and `docs/code/timing-model.md`. Document the
  state graph, command matrix, ownership/lifetime, safe point, failure recovery,
  and future bus-cycle constraint; `make docs-check` must reproduce browsable
  Doxygen/DocC output with zero documentation-debt growth.

## Clarifications

### Session 2026-07-15

- No high-impact unresolved ambiguity remained after applying the C1 roadmap,
  architecture, constitution, and existing host-boundary constraints. The
  command policy below selects deterministic FIFO serialization, synchronous
  completion for state-changing host calls, and explicit typed failures as the
  smallest defaults consistent with those authorities.

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Own Sustained Execution (Priority: P1)

As a host integrator, I can start and pause sustained machine execution from
any host thread while one runtime owner exclusively advances the core, so the
host never races direct machine access or observes a half-completed transition.

**Why this priority**: Every later C1 boundary and all C2/C3 work depend on one
legal execution owner and a stable point at which commands can take effect.

**Independent Test**: Drive start, pause, and state-query commands concurrently
against a clean-room fixture; prove all accepted commands have a deterministic
total order, pause completes at a safe point, and the same ordered input twice
produces the same state and cycle signature.

**Acceptance Scenarios**:

1. **Given** a newly created paused runtime, **When** the host requests start,
   **Then** exactly one owner advances whole instructions and reports running.
2. **Given** a running runtime, **When** one or more threads request pause,
   **Then** all accepted requests complete after the same or a later ordered
   quiescent safe point and the runtime reports paused.
3. **Given** repeated start or pause requests for the already requested state,
   **When** they reach the owner, **Then** they succeed idempotently without an
   extra reset or an unbounded queue.
4. **Given** the same initial fixture and accepted command order, **When** the
   sequence is replayed, **Then** final state, completed cycles, and safe-point
   identity match exactly.

---

### User Story 2 - Serialize Runtime Transactions (Priority: P2)

As a host integrator, I can reset, load media, change keyboard/BREAK state, and
inspect machine values through the same command path, so every transaction has
a defined relationship to execution rather than racing it.

**Why this priority**: Sustained execution is not safely usable until every
existing mutation and observation obeys the owner established by User Story 1.

**Independent Test**: Submit reset, ROM/disc load, key/BREAK, CPU-state, and
frame-read operations around run/pause commands from multiple threads; verify
each completes according to the command matrix at a completed-instruction safe
point and no machine state is accessed outside the owner.

**Acceptance Scenarios**:

1. **Given** a running runtime, **When** a reset or media-load transaction is
   submitted, **Then** it queues FIFO, executes synchronously at the next
   quiescent safe point, and preserves the runtime's ordered run/pause intent.
2. **Given** a running runtime, **When** key, BREAK, state, or frame operations
   are submitted, **Then** each is serialized by the same owner and returns a
   value or failure from one documented safe point.
3. **Given** pause ordered before a mutation, **When** both complete, **Then**
   the mutation occurs while paused; **given** start ordered after it, **Then**
   execution resumes only after the mutation completes.
4. **Given** a command whose input is rejected, **When** it completes, **Then**
   no partial machine mutation occurs and later valid commands still run.

---

### User Story 3 - Recover Across Public Boundaries (Priority: P3)

As a C or Swift host integrator, I receive stable typed outcomes for every
fallible runtime operation and can recover without process termination,
undefined sentinels, stale diagnostics, or a C++ exception crossing the ABI.

**Why this priority**: Typed recovery completes C1's public promise after the
owner and transaction rules establish which operation produced each outcome.

**Independent Test**: Trigger every defined validation, state, queue, execution,
allocation, and shutdown failure through the C ABI and Swift wrapper; verify
stable codes, operation-scoped diagnostics, no exception escape, no stale error,
and documented reset/destruction recovery.

**Acceptance Scenarios**:

1. **Given** an invalid pointer, input, or command/state combination, **When** a
   fallible C operation is called, **Then** it returns a documented status code
   and never requires an ambiguous sentinel to distinguish failure.
2. **Given** an execution fault, **When** it reaches the runtime owner, **Then**
   the runtime becomes faulted at a safe point, exposes one stable diagnostic,
   rejects execution commands, and accepts reset or destruction.
3. **Given** a C failure, **When** the same operation is called through Swift,
   **Then** Swift throws the corresponding typed `BeebError` without losing the
   stable status category or diagnostic.
4. **Given** recovery by reset, **When** reset succeeds, **Then** the runtime is
   paused, its prior fault is cleared, and subsequent commands behave normally.

### Edge Cases

- Zero-cycle execution requests complete successfully without advancing the
  machine or creating a sustained-running ambiguity.
- Start, pause, and reset requests arriving concurrently are ordered by the
  owner; tests assert the observed order rather than thread scheduling order.
- Destruction first stops command acceptance, resolves already accepted work by
  the documented shutdown policy, joins the owner, and only then releases state.
- Commands submitted after shutdown begins fail immediately with a stable
  unavailable status and do not access the handle's machine state.
- An execution fault in an unsupported opcode is contained after device time
  for the completed instruction boundary has been accounted consistently.
- Diagnostics are operation-scoped: a successful operation cannot expose an
  older failure, and concurrent callers cannot read another command's message.
- A frame read before any completed frame returns an explicit no-value success,
  not a failure; its output lifetime remains documented.
- Allocation failure for a command payload or copied media returns without
  partial mutation and without terminating the owner thread.
- Queue growth is bounded by synchronous completion/back-pressure; commands are
  not silently dropped or accumulated without a limit.
- The later bus-cycle sequencer may add internal micro-steps but must retain the
  same externally observable completed-instruction safe point.

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: The system MUST give each machine runtime exactly one execution
  owner; no other thread may directly read or mutate emulated machine state.
- **FR-002**: The runtime MUST expose at least paused, running, faulted, and
  shutting-down lifecycle states with a documented legal-transition graph.
- **FR-003**: A newly created runtime and a successfully reset runtime MUST be
  paused at a quiescent safe point.
- **FR-004**: A quiescent safe point MUST be the boundary after one complete CPU
  instruction or interrupt and after every device has advanced through that
  transition's aggregate CPU cycles.
- **FR-005**: Start, pause, reset, ROM/disc load, key/BREAK mutation, CPU-state
  read, frame read, and shutdown MUST enter one machine-scoped FIFO command path.
- **FR-006**: Accepted state-changing commands MUST complete synchronously from
  the caller's perspective only after their effect or failure is determined at
  a safe point; state queries MUST represent one safe point.
- **FR-007**: Start while running and pause while paused MUST be idempotent
  successes; other illegal state/command pairs MUST return a typed failure.
- **FR-008**: Reset and media/input transactions submitted while running MUST
  execute at an ordered safe point without permitting concurrent machine access;
  the surrounding FIFO start/pause intent determines whether execution continues.
- **FR-009**: The runtime MUST use emulated work budgets or explicit host
  commands to advance state; host wall-clock time MUST NOT change core results.
- **FR-010**: The command path MUST provide bounded back-pressure and MUST NOT
  silently drop accepted commands or permit unbounded pending work.
- **FR-011**: Shutdown MUST stop acceptance, complete or reject already accepted
  commands according to one documented policy, join the owner, and release the
  machine only after owner access has ended.
- **FR-012**: An execution failure MUST leave the runtime faulted at a safe
  point, reject further execution, preserve query access, and permit reset or
  destruction as documented recovery.
- **FR-013**: Every fallible C entry point MUST return a stable structured status
  category; success-with-no-value MUST be distinguishable from failure.
- **FR-014**: No C++ exception MUST cross the C ABI, and an operation failure
  MUST NOT leak partial mutation or another caller's diagnostic.
- **FR-015**: The Swift wrapper MUST preserve the C status category and message
  as a typed, recoverable `BeebError` and MUST remain safe to use across Swift
  concurrency domains.
- **FR-016**: Input bytes MUST be copied before an accepted command outlives the
  call; returned values MUST be owned copies or have an explicit lifetime that
  the Swift wrapper resolves before returning.
- **FR-017**: The same initial machine state, accepted command order, command
  payloads, and emulated budgets MUST yield the same final state and safe-point
  signature across at least ten repeated runs.
- **FR-018**: Race-focused stress tests and the supported thread-sanitizer
  profile MUST cover concurrent start, pause, reset, load, input, query, failure,
  and shutdown interactions without reporting unsynchronized machine access.
- **FR-019**: Existing C0 deterministic fixtures, version/error boundary tests,
  generated documentation, and supported aggregate verification MUST remain green.
- **FR-020**: Public documentation MUST define the lifecycle graph, command
  matrix, threading, ownership, lifetimes, safe point, errors, and recovery.
  Every new or changed private/internal named type and interface MUST document
  its purpose, responsibility boundary, and important invariants, with relevant
  ownership, lifetime, threading, and collaboration constraints. Generated
  documentation validation MUST show zero debt growth.

### Key Entities *(include if feature involves data)*

- **Machine Runtime**: Owns one emulator instance, its owner execution context,
  lifecycle state, accepted-command sequence, and current failure.
- **Runtime Command**: One ordered request with a kind, copied input payload,
  sequence number, completion outcome, and optional returned value.
- **Quiescent Safe Point**: Observable completed-instruction boundary identified
  by machine cycle count and command sequence after all aggregate device time is
  advanced.
- **Runtime Status**: Stable outcome category plus operation-scoped diagnostic;
  represents success, validation/state rejection, execution failure, resource
  exhaustion, unavailability, or internal failure.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: Contract tests cover every lifecycle transition and every entry in
  the public command/state matrix, including idempotent and rejected cases.
- **SC-002**: Ten repetitions of each named concurrent command scenario produce
  an identical accepted-command order replay result, final CPU/device signature,
  and safe-point cycle count.
- **SC-003**: Race-focused stress scenarios complete at least 10,000 mixed
  commands per supported sanitizer run with zero data-race, deadlock, lost
  accepted-command, or use-after-destroy report.
- **SC-004**: Every fallible C boundary operation has automated success,
  invalid-input, invalid-state, execution-fault, and stale-diagnostic coverage
  where applicable; Swift tests prove corresponding typed recovery.
- **SC-005**: `make test`, the supported sanitizer gates, `swift test`,
  `swift build`, `make verify-c0`, and `make docs-check` pass after C1 delivery.
- **SC-006**: Generated C/C++ and Swift documentation covers every changed
  public surface and private/internal named abstraction, contains working links
  to the state/command/safe-point guides, and leaves the documentation-debt
  baseline at zero.

## Assumptions

- C1 is planned as one phase package with two dependent, independently testable
  stories: single-owner runtime first, recoverable boundary transport second.
- The current instruction-level `CPU6502::step()` plus `BBCMicro::tick()` return
  already forms the required safe point; C1 names and protects that invariant
  without implementing bus-cycle micro-operations.
- Sustained execution means an owner can continue executing bounded emulated
  work until an ordered command changes state; no real-time pacing guarantee is
  part of this feature.
- FIFO order is the order in which the runtime accepts commands, not an attempt
  to make operating-system thread scheduling deterministic.
- The existing C ABI may grow additively during the unreleased development
  line; any replacement or deprecation must remain explicit and version-safe.
- C2 owns frame/audio capacity and overflow contracts, and C3 owns persistence;
  C1 exposes only the owner and safe-point foundation they require.
