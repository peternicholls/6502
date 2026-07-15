# Emulator core roadmap

**Status:** Canonical core planning document  
**Updated:** 2026-07-15

This roadmap governs the portable BBC Model B emulation foundation and its
public host boundary. It does not define the wider application experience;
those priorities live in the [product strand](product/README.md).

## Current baseline

The core already provides a tested NMOS 6502, BBC memory map, partial VIAs,
CRTC/Video ULA rendering, clean-room Mode 7, SN76489 sample generation,
logical 8271 disk operations, a C ABI and a synchronized Swift wrapper.

Exact verified coverage and hardware gaps are recorded in
[STATUS.md](STATUS.md). Architecture changes must preserve the deterministic,
host-agnostic boundary in [ARCHITECTURE.md](ARCHITECTURE.md).

## Priority 1 — product-enabling runtime contracts

The next core work should make the existing machine safe to embed in a
sustained host runtime.

- Define single-owner execution semantics instead of relying on callers to
  coordinate arbitrary `run` calls.
- Add versioned, deterministic save-state serialization with compatibility
  checks and explicit mounted-media state.
- Add bounded completed-frame and audio-production contracts suitable for
  decoupled host consumers.
- Expose emulation rate, frame number, buffer demand and failure diagnostics
  without importing host timing APIs into the core.
- Add controlled pause-and-transact operations for memory inspection and
  mutation, required by future editor and debugger workflows.
- Export modified writable disk images explicitly.

Exit evidence:

- snapshot round trips reproduce CPU, memory and device state;
- host consumers cannot observe invalidated frame/audio memory;
- execution, inspection and media export cannot race;
- the C and Swift APIs report structured, recoverable failures.

## Priority 2 — bus-cycle timing foundation

Instruction-level results and aggregate cycles are already well tested, but
devices currently advance after each instruction. Replace that granularity
without discarding the semantic test layer.

- Express instruction execution as ordered bus-cycle micro-operations.
- Tick devices and apply slow-bus stretching at each relevant cycle.
- Sample IRQ, NMI and ready state at defined boundaries.
- Preserve documented dummy reads/writes and read-modify-write behavior.
- Add bus-trace fixtures for reset, interrupts, branches, indexed crossings and
  representative I/O accesses.

Exit evidence:

- existing CPU functional and arithmetic suites remain green;
- bus traces match primary or transistor-level references;
- device interrupts are no longer delayed by a complete instruction;
- timing-sensitive compatibility fixtures improve without host-specific hacks.

## Priority 3 — compatibility-led device completion

Complete hardware behavior in response to representative software and traces,
not by expanding every chip indiscriminately.

- VIA: shift modes, CA2/CB2 handshakes, latches and missing keyboard/IC32
  behavior.
- CRTC/ULA: cursor, sync widths, scrolling, address sequences and justified
  mid-frame changes.
- Mode 7: double height, hold/release graphics, conceal and control latency
  through clean-room implementation.
- SN76489: confirm noise behavior, improve clock coupling and add band-limited
  output only where measurements justify it.
- 8271: commands, timing and error semantics required by curated DFS media;
  flux and copy-protection remain outside current scope.

Every change requires a focused regression and a documented compatibility or
reference reason.

## Priority 4 — cassette hardware and media primitives

The core owns deterministic tape hardware and byte/edge processing; microphone
capture and waveform UI remain host/product responsibilities.

- Implement 6850 ACIA and Serial ULA behavior.
- Model motor state and cassette timing on the machine clock.
- Parse required UEF data, carrier and gap chunks behind a bounded media API.
- Add deterministic WAV edge-decoder fixtures before accepting live samples.
- Surface checksums and decode failures as structured diagnostics.

Exit evidence:

- redistributable UEF/WAV fixtures load repeatably;
- motor behavior follows emulated control state;
- file decoding is independent of host audio callback timing.

## Priority 5 — inspection and editor bridge

- Provide stable CPU/device snapshots for a future inspector.
- Add memory watchpoints, breakpoints and disassembly hooks without coupling to
  a particular UI.
- Define BASIC program memory discovery and atomic injection boundaries.
- Keep label resolution, source models and diff presentation outside the core;
  expose only authentic bytes and safe transactions.

## Validation track

- Maintain exhaustive arithmetic, opcode, device and ABI tests.
- Keep an independent 6502 functional suite and add redistributable boot
  fixtures.
- Add golden frames with documented provenance.
- Maintain sustained timing and buffer tests for host integration.
- Run warnings as errors, sanitizers and both C and Swift boundary tests in CI.
- Record every known accuracy limit in `STATUS.md` before making compatibility
  claims.

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
