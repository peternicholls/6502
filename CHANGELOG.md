# Changelog

All notable changes to Beeb6502 are documented in this file. The project uses
[Semantic Versioning](https://semver.org/spec/v2.0.0.html) and follows the
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/) structure.

## [Unreleased]

## [0.2.0] - 2026-07-15

### Added

- A single-owner `MachineRuntime` with a capacity-64 FIFO, deterministic
  instruction safe points, sustained execution, bounded commands, shutdown
  drain/join, fault recovery, and opt-in exact replay evidence.
- A structured C 0.2 boundary with eight operation-scoped status categories,
  success-only out-parameters, caller-owned frames, and destroy-overlap safety.
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
  owner. BBC-mode `--trace` is no longer accepted; standalone functional CPU
  tracing remains available.
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

[Unreleased]: https://github.com/peternicholls/6502/compare/v0.2.0...HEAD
[0.2.0]: https://github.com/peternicholls/6502/releases/tag/v0.2.0
[0.1.0]: https://github.com/peternicholls/6502/releases/tag/v0.1.0
