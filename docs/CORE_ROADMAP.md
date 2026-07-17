# Emulator core roadmap

**Status:** Canonical core planning document  
**Updated:** 2026-07-17

This roadmap governs the portable BBC Model B emulation foundation and its
public host boundary. It does not define the wider application experience;
those priorities live in the [product strand](product/README.md).

## How to use this roadmap

This document sequences outcomes and dependencies. It is not a sprint backlog
and it does not promise dates. Each candidate slice enters delivery through the
[Spec Kit workflow](../specs/README.md): specification, clarification where
needed, plan, tasks, consistency analysis, then implementation.

A phase may require several feature specifications. Do not schedule an entire
phase as one sprint. Sprint planning may select tasks only from reviewed feature
artifacts whose dependencies and entry criteria are satisfied.

### Phase status

- **Ready:** sufficiently bounded for the next Spec Kit specification.
- **Active:** at least one approved feature specification is in implementation.
- **Queued:** sequenced, but an earlier dependency or decision remains.
- **Later:** held until its named product horizon becomes active.
- **Blocked:** an identified external decision or dependency prevents progress.
- **Complete:** exit evidence is recorded in `STATUS.md` and all associated
  feature acceptance evidence passes.

Only one phase is the default source of the next core feature. Explicitly
independent preparation or compatibility work may run in parallel under the
rules below.

## Current baseline

The core already provides a tested NMOS 6502, BBC memory map, partial VIAs,
CRTC/Video ULA rendering, clean-room Mode 7, SN76489 sample generation,
logical 8271 disk operations, a single-owner machine runtime, and structured C
and Swift host boundaries.

Exact verified coverage and hardware gaps are recorded in
[STATUS.md](STATUS.md). Architecture changes must preserve the deterministic,
host-agnostic boundary in [ARCHITECTURE.md](ARCHITECTURE.md).

## Delivery map

| Phase | Status | Product trace | Depends on | May overlap |
| --- | --- | --- | --- | --- |
| C0 — Baseline evidence | Complete | Machine foundation | Current verified core | Compatibility fixture research |
| C1 — Runtime ownership | Complete | Horizon 1: sustained Machine runtime | C0 | C2/C3 research only |
| C2 — Bounded output contracts | Active | Horizon 1: continuous video and audio | C1 | C3 implementation |
| C3 — Session continuity | Ready | Horizon 1: background and restore | C1 safe-point contract | C2 implementation |
| C4 — Bus-cycle timing | Queued | Compatibility and timing fidelity | C1; C3 snapshot invariant | Pull-based compatibility fixtures |
| C5 — Dependable media core | Later | Horizon 2: Media | C1; slice-specific timing prerequisites | Curated device fixtures |
| C6 — Inspection and editor bridge | Later | Horizon 3: Editor | C1 and C3 | Read-only inspector research |

The default work is C2 exit verification. C3 may proceed beside it because C1
defines and verifies the quiescent safe point; C3 becomes the default next core
phase when C2 completion is recorded. C4 implementation begins only after the
snapshot invariant described in C3 is fixed. C5 and C6 do not enter the active
Machine critical path.

## Phase C0 — baseline evidence

**Status:** Complete

**Outcome:** Lock the observable foundation before changing execution and
buffer ownership.

**Product capability unlocked:** Safe delivery of the sustained Machine
runtime.

### Candidate Spec Kit slice

- `core-baseline-evidence`: consolidate deterministic boot, representative
  frame, ABI and performance baselines with documented fixture provenance.

### Entry criteria

- The verified baseline in `STATUS.md` matches the current test suites.
- Existing proprietary firmware remains outside the repository and cannot be a
  required CI fixture.

### Exit evidence

- `make test`, `make sanitize`, `swift test` and `swift build` pass on their
  supported hosts ([clean exit gate record](CORE_BASELINE.md#c0-exit-gate-evidence)).
- A redistributable boot fixture reaches a named deterministic state with an
  exact cycle count or state signature
  ([approved clean-room evidence](CORE_BASELINE.md#approved-clean-room-evidence)).
- Representative bitmap and Mode 7 output has provenance-recorded golden
  evidence ([exact identities and limitations](STATUS.md#c0-foundation-evidence)).
- Current emulation throughput is measured reproducibly and recorded as a
  comparison baseline, not presented as a product guarantee
  ([measurement context](CORE_BASELINE.md#descriptive-throughput-comparison)).
- The C and Swift version/error boundary remains covered by automated tests
  ([verified commands and counts](STATUS.md#c0-foundation-evidence)).
- A single documented flow generates browsable C/C++, C boundary, Swift, and
  conceptual documentation; supported public surfaces are covered and the
  initial internal documentation-debt inventory is recorded
  ([rendered-page review](CORE_BASELINE.md#browsable-code-documentation-evidence)).
- Documentation validation rejects invalid markup, broken internal links, and
  undocumented new or changed public contracts without requiring low-value
  commentary on self-evident code
  ([quality-gate evidence](STATUS.md#c0-foundation-evidence)).

The transition gate was rerun successfully on clean revision
`4a87ba3624146403fd9f663118101db60321576c`: all 11 macOS aggregate groups,
all focused C0 contract scripts, and `git diff --check` passed. C0 changes no
hardware-fidelity status; it makes that status reproducible and safe to evolve.

### Non-goals

- Increasing hardware fidelity or restructuring the CPU.
- Adding a new benchmark dependency when the existing harness can record the
  required measurements.
- Attempting to annotate every unchanged private helper or tracking generated
  documentation output as an authoritative source.

## Phase C1 — runtime ownership and recoverable boundaries

**Status:** Complete

**Outcome:** Make machine execution a single owned state machine with explicit
commands and recoverable public failures.

**Product capability unlocked:** Sustained background execution without
cross-thread races or undefined pause/reset behavior.

**Depends on:** C0.

### Candidate Spec Kit slices

1. `single-owner-runtime`: execution states, command serialization and
   run/pause/reset lifecycle.
2. `recoverable-runtime-errors`: consistent ownership, nullability and failure
   contracts across C++, C and Swift.

### Required design decision

Define a **quiescent safe point** at the completed-instruction boundary after
all devices have advanced through the instruction's aggregate cycles. Pause,
snapshot and host transactions may complete only at such a point. The later
bus-cycle sequencer MUST preserve the ability to reach this boundary.

### Entry criteria

- C0 evidence is green.
- The specification defines legal execution states, command ordering and which
  operations reject, queue or wait while the machine is running.

### Exit evidence

- Concurrent run, pause, reset and load attempts are deterministically
  serialized or rejected according to contract tests.
- Race-focused stress tests and sanitizers find no unsynchronized access to
  machine state.
- Every fallible C entry point returns a structured, recoverable failure that
  the Swift wrapper preserves without a C++ exception crossing the ABI.
- Repeating the same command sequence from the same state produces the same
  result and quiescent boundary.

The C1 implementation is the completed `002-runtime-ownership` Spec Kit slice.
Its remediated completion candidate passed 49 C++ tests, 44 sanitizer tests,
ten Swift tests, all 11 C0 groups, all six C1 aggregate groups, strict generated
documentation, formatting, and static analysis. Final review found no CRITICAL
or HIGH issue ([exact gate record](STATUS.md#c1-remediation-completion-evidence)).
The local ThreadSanitizer probe is not executable and is recorded as N/A rather
than a pass; supported CI remains required to execute that instrumentation.

C1 establishes the dependency contract only. It does not supply C2's bounded
producer queues or C3's versioned snapshot format.

### Non-goals

- Arbitrary editor memory mutation, watchpoints or source-level debugging.
- Host frame presentation and audio-device integration.

## Phase C2 — bounded frame, audio and diagnostic contracts

**Status:** Active

**Outcome:** Give decoupled host consumers stable, bounded output without
making host timing authoritative.

**Product capability unlocked:** Continuous Machine video and audio that do not
stall the UI.

**Delivery elevation:** C2 promotes the Apple host from opening `Package.swift`
directly to a committed `Beeb6502.xcodeproj` with shared macOS, iOS Simulator,
and test schemes. The Xcode project is a host/build surface over the same local
Swift package products; Swift Package Manager and the portable Makefile remain
authoritative build paths, and no Apple framework enters `BeebCore`.

**Depends on:** C1.

**Parallelism:** C3 may proceed in parallel after the C1 safe-point contract is
accepted.

### Candidate Spec Kit slices

1. `completed-frame-contract`: immutable or explicitly owned completed frames,
   bounded capacity and an overflow policy.
2. `audio-production-contract`: bounded sample production and demand reporting
   suitable for a host ring buffer.
3. `runtime-diagnostics`: frame number, emulation rate, buffer demand and
   recoverable underrun/overrun diagnostics.

### Entry criteria

- C1 defines the only legal execution owner and command path.
- Each specification defines ownership, lifetime, capacity and overflow
  behavior before choosing an implementation.

### Exit evidence

- A consumer retaining a valid frame or audio value cannot observe invalidated
  or concurrently mutated storage.
- Capacity and overflow behavior are deterministic and covered at empty, full
  and sustained-production boundaries.
- Sustained production meets the measurable duration and tolerance established
  by the feature specification without unbounded memory growth.
- Metrics use emulated counters and explicit host observations; host wall-clock
  values never drive core state.
- C and Swift boundary tests cover lifetime, failure and back-pressure
  behavior.
- The committed Xcode project builds the macOS app, iOS Simulator app, and test
  scheme from a clean checkout through `xcodebuild`, without duplicating core
  sources or weakening the Makefile/Swift Package gates.

The implemented `003-bounded-output-contracts` slice has passed its required
measurement gate: 10,000 frames transferred with retained-value immutability;
separate 10,000-item pressure stress reached but did not exceed capacities 3
and 4,096; a 10-second warm-up plus 120,000,060 measured cycles balanced both
conservation equations; RSS growth was 32,768 bytes against a 16 MiB limit; and
the synthetic rate error was zero against a 0.1% tolerance. Shared Xcode
schemes build macOS and generic iOS Simulator apps and run all 14 Swift tests,
while Swift Package Manager and Make remain independently green. Final full
validation and documentation generation remain the exit checkpoint before the
phase status becomes Complete.

### Non-goals

- Metal presentation, AVAudioEngine callbacks or host refresh scheduling.
- Visual CRT effects or host UI diagnostics.
- Xcode Cloud, signing/distribution automation, App Store packaging, or replacing
  Swift Package Manager and the portable Makefile.

## Phase C3 — versioned session continuity

**Status:** Ready

**Outcome:** Save and restore architectural machine state deterministically and
reject incompatible or damaged state safely.

**Product capability unlocked:** Backgrounding and session restoration for the
Machine experience.

**Depends on:** The C1 safe-point contract.

**Parallelism:** May run beside C2.

### Candidate Spec Kit slices

1. `snapshot-format-v1`: format envelope, version policy and architectural
   state model.
2. `snapshot-round-trip`: CPU, memory, ROM selection and device state.
3. `snapshot-mounted-media`: mounted-media identity and modified in-memory
   media state.
4. `snapshot-host-boundary`: recoverable C and Swift load/save contracts.

### Snapshot invariant

Version 1 snapshots may be captured only at the C1 quiescent
completed-instruction boundary. They serialize architectural machine state,
not an in-progress CPU micro-operation. C4 MUST preserve this safe point. A
future requirement for mid-instruction snapshots requires a new format version
and an explicit compatibility or migration plan.

### Entry criteria

- The quiescent safe point is tested and documented.
- The specification defines compatibility policy, size limits, corruption
  handling and whether unknown optional data can be skipped.

### Exit evidence

- Snapshot round trips reproduce CPU, memory, selected ROM, device and mounted
  media state exactly at the safe point.
- Continuing both the original and restored machines for the same emulated
  interval produces matching state and observable outputs.
- Truncated, corrupt and unsupported-version input is rejected without
  partially mutating the destination machine.
- At least one version-1 fixture is retained to detect future compatibility
  drift.
- C and Swift callers receive structured failures and retain ownership of their
  original data.

### Non-goals

- Time-travel history, cloud synchronization or source-level editor state.
- Serializing host windows, audio devices or presentation queues.

## Phase C4 — bus-cycle timing foundation

**Status:** Queued

**Outcome:** Replace instruction-end device advancement with ordered bus-cycle
micro-operations while preserving the semantic layer.

**Product capability unlocked:** Evidence-led compatibility improvements for
timing-sensitive Machine software.

**Depends on:** C1 completion and the C3 snapshot invariant being fixed.

### Candidate Spec Kit sequence

1. `bus-trace-contract`: trace vocabulary and fixtures for reset, interrupts,
   branches, indexed crossings, read-modify-write and representative I/O.
2. `cpu-bus-sequencer`: fetch/read/write micro-operations while retaining
   instruction semantic tests.
3. `interrupt-and-ready-sampling`: IRQ, NMI and ready-state boundaries.
4. `slow-bus-and-device-ticks`: 1 MHz stretching and per-cycle device clocks.
5. `timing-compatibility-gate`: curated software fixtures that demonstrate the
   intended improvement.

These are sequential migration slices, not one feature or sprint. Trace-fixture
research may begin earlier; sequencer implementation may not.

### Entry criteria

- C0 baseline evidence remains reproducible.
- C1 owns execution and exposes the quiescent safe point.
- C3 has fixed the rule that version-1 snapshots exclude in-progress
  micro-operation state.
- Each slice identifies the primary or transistor-level reference used for its
  expected bus sequence.

### Exit evidence

- Existing CPU functional, arithmetic, device and ABI suites remain green.
- Bus traces match cited references for every fixture in the agreed set.
- Device interrupts are no longer delayed by a complete instruction.
- Dummy accesses, read-modify-write behavior and slow-bus stretching occur at
  their specified cycles.
- The C1/C3 quiescent safe point and version-1 snapshot compatibility remain
  intact.
- Throughput is compared with the C0 baseline; any accepted regression has a
  measured limit and rationale in the feature plan.
- Compatibility improves without host-specific timing hacks.

### Non-goals

- Undocumented opcodes unless a separate compatibility case justifies them.
- Indiscriminate completion of every device or expansion platform.

## Compatibility-led device workstream

This is a pull-based workstream, not an open-ended delivery phase. A device
slice becomes Ready only when it has:

1. a reproducible software, trace or product case;
2. a primary reference or documented clean-room observation;
3. a focused failing regression;
4. a bounded behavior change and explicit non-goals.

The current candidate pool is:

- VIA shift modes, CA2/CB2 handshakes, latches and keyboard/IC32 behavior;
- CRTC/ULA cursor, sync widths, scrolling, address sequences and justified
  mid-frame changes;
- Mode 7 double height, hold/release graphics, conceal and control latency;
- SN76489 noise confirmation, clock coupling and evidence-led band limiting;
- 8271 commands, timing and errors required by curated DFS media.

Each completed slice MUST pass its focused regression, preserve unrelated
suites, improve its named compatibility case, and update `STATUS.md`. Flux and
copy-protection remain outside the current horizon.

## Phase C5 — dependable media core

**Status:** Later

**Product trace:** Horizon 2 — Media.

**Outcome:** Provide deterministic, recoverable disk and cassette primitives
without silently modifying imported content.

**Depends on:** C1; each timing-sensitive slice must also state whether C4 is a
prerequisite.

### Candidate Spec Kit sequence

1. `writable-disk-export`: explicit export of modified in-memory SSD/DSD data.
2. `dfs-controller-compatibility`: only the 8271 behavior required by the
   curated Media fixture set.
3. `cassette-chipset`: 6850 ACIA, Serial ULA and motor timing.
4. `uef-media-primitives`: required UEF data, carrier and gap chunks.
5. `wav-edge-decoder`: deterministic edge fixtures after file-based UEF loading
   is reliable.

### Entry criteria

- The corresponding Product Horizon 2 slice is selected.
- Fixture provenance and supported media boundaries are documented.
- Import, in-memory mutation and explicit export ownership are specified.

### Exit evidence

- Writable disk data exports explicitly and never overwrites imported source
  media silently.
- Supported UEF/WAV fixtures load repeatably and report checksum or decode
  failures through structured diagnostics.
- Motor behavior follows emulated control state.
- File decoding is independent of host audio callback timing.

### Non-goals

- Microphone capture, waveform UI, flux preservation or copy-protection.

## Phase C6 — inspection and editor bridge

**Status:** Later

**Product trace:** Horizon 3 — Editor.

**Outcome:** Expose authentic bytes and atomic machine transactions without
coupling the core to source models or UI.

**Depends on:** C1 execution ownership and C3 session-state boundaries.

### Candidate Spec Kit sequence

1. `stable-inspection-snapshots`: CPU and device state for a read-only
   inspector.
2. `breakpoint-watchpoint-contract`: bounded execution control and observation.
3. `atomic-memory-transactions`: pause, validate, mutate and resume safely.
4. `basic-program-boundaries`: discover and inject BBC BASIC program bytes.

### Entry criteria

- The corresponding Product Horizon 3 workflow is specified.
- The host/core contract states concurrency, validation and rollback behavior.

### Exit evidence

- Inspection cannot race execution or expose invalidated state.
- Failed mutations leave the original machine state intact.
- BASIC program boundaries and injected bytes round-trip against documented
  fixtures.
- Label resolution, source models and diff presentation remain outside the
  core.

### Non-goals

- Source editing, UI navigation, label models or semantic diff presentation.
- Time-travel debugging without a separate product and storage decision.

## Phase and sprint gates

A candidate slice is **sprint-ready** only when:

- its Spec Kit specification names one independently testable outcome and the
  product capability it unlocks;
- phase dependencies and required decisions are complete;
- acceptance criteria include measurable failure, boundary and recovery cases;
- the first failing regression or evidence task is identified;
- ownership, lifetime, threading, compatibility and fixture provenance are
  explicit where applicable;
- public-contract, non-obvious-behavior and conceptual documentation impact is
  explicit, with a generated-documentation validation task or a concrete N/A;
- the plan passes every Constitution Check; and
- `tasks.md` is dependency-ordered, gives every task a verified commit
  checkpoint before the next task, records each phase boundary before the next
  phase, and contains no unresolved critical analysis finding.

A phase may move to **Complete** only when every exit-evidence item is linked to
passing tests, traces or measurements, affected C and Swift boundary tests pass,
sanitizers are green, and `STATUS.md`, architecture and changelog documentation
reflect the delivered boundary. Coding phases must also generate browsable code
documentation without invalid markup, broken internal links, or increased
documentation debt.

## Core non-goals

- Windowing, file pickers, Metal, AVAudioEngine and microphone permissions.
- Source editing, UI navigation or product analytics.
- Bundled proprietary ROMs, character generators, games or user media.
- JIT execution or downloaded native code.
- Flux-level preservation and copy-protection emulation in the current horizon.
- BBC Master, Tube, Econet and other expansion platforms without a separate
  product decision.

## Sequencing rules

- Prefer APIs that unlock a complete product workflow over speculative device
  surface area.
- Keep the emulated clock authoritative and host-independent.
- Preserve current semantic tests while replacing timing internals.
- Require evidence before accuracy, performance or compatibility claims.
- Avoid new dependencies unless they materially improve validation or safety.
- Update this roadmap for technical priority changes and `STATUS.md` only when
  implementation evidence changes.
