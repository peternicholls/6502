# Implementation status

**Status:** Sole authority for verified implementation claims
**Updated:** 2026-07-19

This file states what exists now. It does not choose future work. Direction and
gate definitions live only in the
[Machine delivery plan](product/MACHINE_DELIVERY_PLAN.md).

## At a glance

| Scope | State | Meaning |
| --- | --- | --- |
| C0 baseline evidence | **DONE** | Reproducible clean-room, build and documentation evidence exists. |
| C1 runtime ownership | **DONE** | Supported hosts use one recoverable runtime owner across C++, C and Swift. |
| C2 bounded output and Xcode delivery | **DONE** | Owned video/audio output, diagnostics and maintained Apple build surfaces are verified. |
| Active feature | **NONE** | `.specify/feature.json` is empty. |
| Next feature | **NEXT** | `machine-target-profile` has not started. |
| M1 running Model B application | **TODO** | No complete user-facing boot/type/run/video/audio workflow exists. |
| M2 continuity | **TODO** | Snapshot and application lifecycle contracts do not exist. |
| M3 Model B+ 64K preview | **TODO** | The selectable B+ profile and post-C6 product demonstrations do not exist. |

`DONE` means implemented with passed evidence. `NEXT` and `TODO` never imply
partial delivery.

## Verified baseline

- All 151 legal NMOS 6502 opcodes decode; functional, arithmetic, decimal,
  interrupt and stack tests pass.
- The Model B core implements the base memory map, System and User VIAs, CRTC,
  Video ULA, Mode 7, SN76489, keyboard matrix and logical 8271 SSD/DSD access.
- A user-supplied OS 1.20 and BASIC II smoke run reaches the BASIC idle loop and
  renders startup. The repository supplies no proprietary ROM.
- `MachineRuntime` owns aggregate machine state, serializes a bounded command
  FIFO and exposes structured recovery through C++/C/Swift boundaries.
- Completed frames and continuous mono audio use bounded owner-only queues with
  exact pressure accounting and owned cross-language values.
- Swift Package Manager, the checked-in Xcode project, macOS, iOS Simulator and
  the shared test scheme are maintained build surfaces.

## Known fidelity limits

| Area | Verified now | Still missing |
| --- | --- | --- |
| CPU timing | Instruction semantics and aggregate cycle counts | Per-bus-cycle micro-operations; optional undocumented opcodes |
| Memory | Model B RAM, OS ROM, 16 sideways banks and I/O overlay | Sideways RAM policy; exact 1 MHz bus wait phasing |
| 6522 | Ports, timers, interrupts, CA1/CB1 and PB7 timer output | Shift modes and complete CA2/CB2 handshake/pulse behavior |
| Display | Normal CRTC geometry, bitmap modes, Mode 7 and bounded completed frames | Cycle raster, interlace/cursor variants, full Mode 7 controls and exact glyph evidence |
| Audio | Deterministic SN76489 generation and bounded samples | Exact analogue fidelity and application audio-device presentation |
| Disc | Logical SSD/DSD sector read/write through the 8271 model | Full command timing/errors, formatting, deleted sectors and flux |
| Keyboard | Matrix injection through the System VIA | Complete host mapping and all IC32 details |
| Cassette | Not implemented | 6850, Serial ULA, UEF/WAV decoding and motor timing |
| Profiles | Model B core only | Selectable Model B application and Model B+ 64K implementation |
| Persistence | None | Versioned snapshots and lifecycle restoration |

## Evidence

Detailed completion records are preserved without mixing them into this live
baseline:

- [C0 baseline ledger](completed/CORE_BASELINE.md)
- [Full status ledger through C2](completed/STATUS-through-C2-2026-07-19.md)
- [Completed Spec Kit runs](../specs/completed/)

Current verification commands remain `make test`, `make sanitize`,
`make verify-c0`, `make test-c1`, `make test-c2`, `swift test`, `swift build`,
`make docs-check` and `make format-check`. Local unsupported ThreadSanitizer is
`N/A`; supported Linux CI remains strict.

## Claim rule

A row moves into this file only after its feature acceptance evidence passes.
Plans, archived documents, smoke observations and reserved profile names are
not implementation evidence.
