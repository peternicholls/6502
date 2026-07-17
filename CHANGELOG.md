# Changelog

All notable changes to Beeb6502 are documented in this file. The project uses
[Semantic Versioning](https://semver.org/spec/v2.0.0.html) and follows the
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/) structure.

## [Unreleased]

No post-0.3.0-candidate changes have been assigned to a later version.

## [0.3.0] - Unreleased

This is the current development candidate. It has not been published as an
annotated `v0.3.0` tag or GitHub release; complete the release checklist before
replacing `Unreleased` with a date.

### Added

- Owner-only bounded output with a capacity-three completed-frame FIFO and a
  capacity-4,096 continuous mono Float32 audio ring driven at 48 kHz from
  committed emulated cycles.
- Owned C++, caller-owned C, and independently owned Swift frame/audio values,
  with exact empty, underrun, overrun, capacity, and production-failure status
  categories.
- Non-mutating output diagnostics with exact conservation counters, depths,
  capacities, demand, latest output status, and a pure host-observed emulation
  rate calculation.
- Lifetime, replay, concurrent-consumer, allocation-recovery, bounded-capacity,
  aggregate, and documentation-negative C2 evidence.
- A committed `Beeb6502.xcodeproj` with shared macOS, iOS Simulator, and test
  schemes plus matching clean-checkout CI gates.

### Changed

- **Breaking:** Expanded the pre-1.0 C and Swift status vocabularies for bounded
  output pressure and added a recoverable Swift audio error that carries valid
  partial samples and exact shortfall accounting.
- **Breaking:** Added the opaque `beeb_frame.release_context` ownership token.
  Frame transfer now allocates that context before destructive dequeue, moves
  pixel storage without a second copy, and preserves queue/accounting state on
  resource failure.
- Made completed-frame publication and continuous audio production part of the
  existing `MachineRuntime` owner transaction without adding host clocks,
  callbacks, locks, or borrowed producer storage.
- Elevated the committed Xcode project to the primary Apple development entry
  point while retaining Swift Package Manager and Make as independent required
  build surfaces.
- Split required C2 CI into a portable Linux aggregate with strict output-race
  TSan and an Apple Xcode contract, and expanded replay/concurrency evidence
  through the C and Swift boundaries.
- Replaced host-delay assumptions in destroy-overlap and sustained-lifecycle
  regressions with barriers on admitted work and the exact execution event.

## [0.2.0] - Unreleased

This is the current development candidate. It has not been published as an
annotated `v0.2.0` tag or GitHub release; complete the release checklist before
replacing `Unreleased` with a date.

### Added

- A single-owner `MachineRuntime` with a capacity-64 FIFO, deterministic
  instruction safe points, sustained execution, bounded commands, shutdown
  drain/join, fault recovery, and opt-in exact replay evidence.
- A completed structured C 0.2 boundary with eight operation-scoped status
  categories, success-only out-parameters, caller-owned frame allocations, and
  destroy-overlap safety.
- Typed Swift lifecycle, safe-point, fault, and status values with concurrent
  task-group coverage and no redundant host-side machine lock.
- A C1 aggregate covering public boundaries, transaction ordering, exact replay,
  10,000-command/shutdown races, negative documentation fixtures, and aggregate
  failure propagation.
- Swift package regression tests for the public host boundary.
- A public runtime version contract and `--version` command.
- A documented release checklist and version-consistency check.
- Separate product and emulator documentation strands, with canonical vision,
  roadmaps, a legacy decision register and an explicit authority hierarchy.
- A profile-aware `make verify-c0` baseline that reports every behavioral,
  sanitizer, version, boundary, provenance, exact-reference, and documentation
  group without masking later failures.
- Lawful clean-room Mode 7 and bitmap workloads with provenance, exact CPU/frame
  references, ten-run acceptance, and a separately guarded update flow.
- A reproducible five-sample throughput comparison baseline with explicit host,
  compiler, build mode, workload, median, and range context.
- Browsable Doxygen and Swift-DocC output behind one generated landing page,
  with strict markup/link/public-contract checks and a zero documentation-debt
  baseline.

### Changed

- **Breaking:** Replaced the pre-1.0 C sentinel and `beeb_last_error` API with
  structured status returns and explicit outputs; all repository consumers now
  use the 0.2 contract.
- Routed the headless BBC and evidence tools through the supported runtime
  owner. BBC-mode `--trace` is rejected; instruction-level tracing remains
  available only in standalone functional CPU mode.
- Made Swift reset, input, audio, and observation failures explicit throwing
  operations and moved serialization into the C++ runtime owner.
- Reworked the emulator roadmap into dependency-aware delivery phases with
  explicit product traceability, Spec Kit feature slices, parallelism rules and
  measurable entry and exit evidence.
- Added a governed code-documentation strategy to C0 and every later coding
  phase: language-native browsable output, useful contract and invariant
  guidance, generated-doc validation, and a non-increasing debt rule.
- Required verified, Lore-formatted commits after every task and at every phase
  boundary before subsequent project work begins.
- Expanded C, C++, and Swift public contracts with ownership, lifetime, error,
  concurrency, timing, and borrowed-buffer guidance plus focused architecture,
  timing, host-boundary, and evidence guides.

## [0.1.0] - 2026-07-15

### Added

- Complete documented NMOS 6502 instruction set with binary and decimal
  arithmetic tests.
- BBC Model B memory map, VIAs, CRTC, Video ULA, Mode 7, SN76489 and logical
  8271 disc support.
- Dependency-free C++20 core, C ABI, Swift wrapper, SwiftUI demo and headless
  command-line runner.
- SSD/DSD media support and clean-room demonstration ROM tooling.

### Fixed

- Prevented C++ exceptions from escaping across the C ABI into Swift hosts.
- Serialized ROM and disc mutation with emulation execution in `BeebMachine`.
- Corrected nonzero CRTC vertical-adjust frame timing.
- Rendered Mode 7 control-code cells using the active background colour.

[Unreleased]: https://github.com/peternicholls/6502/compare/develop...HEAD
