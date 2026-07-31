# Implementation status

**Status:** Sole authority for verified implementation claims
**Updated:** 2026-07-31

This file states what exists now. It does not choose future work. Direction and
gate definitions live only in the
[Machine delivery plan](product/MACHINE_DELIVERY_PLAN.md).

## At a glance

| Scope | State | Meaning |
| --- | --- | --- |
| C0 baseline evidence | **DONE** | Reproducible clean-room, build and documentation evidence exists. |
| C1 runtime ownership | **DONE** | Supported hosts use one recoverable runtime owner across C++, C and Swift. |
| C2 bounded output and Xcode delivery | **DONE** | Owned video/audio output, diagnostics and maintained Apple build surfaces are verified. |
| Machine target profile | **DONE** | Extensible profile identity, Model B construction/query and failure-atomic rejection are verified across C++, C, Swift and the host. |
| Active feature | **IN PROGRESS** | `005-model-b-workflow` has automated host workflow coverage; ROM-dependent macOS acceptance remains open. |
| Next feature | **NEXT** | Continue the bounded Model B workflow gate, then close the remaining M1 acceptance evidence. |
| M1 running Model B application | **TODO** | Firmware import, BASIC readiness, bounded keyboard/frame/control host paths exist; direct ROM-backed visual acceptance and audio presentation remain unverified. |
| M2 continuity | **TODO** | Snapshot and application lifecycle contracts do not exist. |
| M3 Model B+ 64K preview | **TODO** | B+ machine behavior and the post-C6 product demonstrations do not exist. |

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
- Machine profiles are owned bounded values across C++, C and Swift. Model B is
  the only supported runtime profile; owner-serialized queries return its
  immutable identity without changing safe-point or replay behavior.
- Model B+ 64K has a permanent distinct identity and an accessible host request,
  but construction reports recognised-unavailable. It does not emulate B+
  hardware, fall back to Model B or mutate the active Model B session.
- The maintained demo host has a bounded Model B workflow path for typed OS and
  language-ROM roles, fixed language bank 12, physical-key focus, owned frame
  dequeue, presentation epochs, and independent Run/Pause/Reset/BREAK controls.
  This is automated evidence only until lawful ROM-backed macOS observation is
  recorded.

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
| Profiles | Extensible identity transport; Model B construction/query; distinct recognised-unavailable B+ 64K request | Model B+ 64K machine behavior; persistence; later profile and expansion assignments |
| Persistence | None | Versioned snapshots and lifecycle restoration |

## Evidence

Detailed completion records are preserved without mixing them into this live
baseline:

- [C0 baseline ledger](completed/CORE_BASELINE.md)
- [Full status ledger through C2](completed/STATUS-through-C2-2026-07-19.md)
- [Completed Spec Kit runs](../specs/completed/)
- [Machine target-profile acceptance](../specs/completed/004-machine-target-profile/evidence/verification.md)

Current verification commands remain `make test`, `make sanitize`,
`make verify-c0`, `make test-c1`, `make test-c2`, `swift test`, `swift build`,
`make docs-check` and `make format-check`. They are the maintained verification
surfaces, not a requirement to repeat every surface after every focused change.
The delivery plan and [evidence guide](code/evidence-and-testing.md) define when
focused, affected-wider and full milestone verification run. Local unsupported
ThreadSanitizer is `N/A`; supported Linux CI remains strict.

## Claim rule

A row moves into this file only after its feature acceptance evidence passes.
Plans, archived documents, smoke observations and reserved profile names are
not implementation evidence.
