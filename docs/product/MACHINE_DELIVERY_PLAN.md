# Machine delivery plan

**Status:** Sole canonical forward programme authority
**Updated:** 2026-07-21

This is the only document that commits scope, selects next work, orders delivery
or defines programme gates. Every new feature must trace to a named row or gate
here before its Spec Kit artifacts are created.

[STATUS.md](../STATUS.md) owns verified state. [ARCHITECTURE.md](../ARCHITECTURE.md)
owns current boundaries. [IMPLEMENTATION_CONSTRAINTS.md](../IMPLEMENTATION_CONSTRAINTS.md)
owns technical constraints for unfinished work. [VISION.md](VISION.md) owns
durable intent. None may create a competing backlog.

## State vocabulary

| State | Meaning |
| --- | --- |
| **DONE** | Implemented and backed by passed acceptance evidence linked from `STATUS.md`. |
| **ACTIVE** | The single feature named by `.specify/feature.json` is in implementation. |
| **NEXT** | First committed work, not started. |
| **TODO** | Committed work, not delivered. |
| **RESERVED** | Named future option, not committed for delivery. |

## Current ledger

| Item | State | Boundary |
| --- | --- | --- |
| C0 baseline evidence | **DONE** | Reproducible foundation evidence |
| C1 runtime ownership | **DONE** | Single owner and recoverable C++/C/Swift boundary |
| C2 bounded output and Xcode delivery | **DONE** | Owned frame/audio output, diagnostics and maintained Apple builds |
| Active feature | **ACTIVE** | `machine-target-profile` is at its completion/version checkpoint pending archival |
| `machine-target-profile` | **DONE** | Extensible identity transport and safe rejection across core, C, Swift and host |
| M1 running Model B application | **TODO** | First usable application |
| M2 continuity | **TODO** | Snapshot and lifecycle completion |
| M3 Model B+ 64K developer preview | **TODO** | Required post-C6 outcome |

Completed foundations and profile identity are not a completed application. No
firmware onboarding, host video/audio presentation, full host keyboard
workflow, snapshots or Model B+ machine behavior has been delivered.

## Machine profiles

Profiles use a stable base identifier plus explicit versioned expansions. They
are not a closed two-value enum.

- **Model B — committed and supported:** its identity flows across core, C,
  Swift and host construction/query; the complete application workflow is
  still required for M1.
- **Model B+ 64K — committed identity, unavailable behavior:** its distinct
  identity and host request are recognised, but construction rejects without
  fallback. Reference-led machine implementation remains required for M3.
- **B+ 128K, Master-family, Tube, Econet, ADFS and other storage/peripheral
  expansions — RESERVED:** each needs a product case, primary references,
  lawful fixtures and a plan amendment.

The application displays the selected profile. Firmware, snapshots and media
must identify compatibility explicitly. Unknown identity rejects without
mutating the active machine and never falls back to another profile.

## Delivery gates

### M1 — Running Model B Machine

**State:** **TODO**

M1 passes when a macOS user can, without shell commands:

1. import and assign user-owned OS and BASIC ROMs;
2. select Model B and boot to BASIC;
3. type and run a documented short BASIC program;
4. receive continuous video from C2 completed-frame output;
5. hear continuous audio from C2 bounded audio output; and
6. run, pause, reset and BREAK while seeing useful runtime/output diagnostics.

The maintained run must keep the main actor responsive, keep host clocks out of
emulated time, recover to actionable states and bundle no proprietary bytes.
iPhone/iPad layout and input claims require the later adaptation row.

### M2 — Continuity-complete Machine

**State:** **TODO**

**Depends on:** M1, C3 snapshot contracts and iOS/iPadOS adaptation.

M2 passes when background/termination restoration preserves the selected
profile, CPU, RAM, ROM selection, devices and mounted-media state. Stale output
does not cross restore. Corrupt, oversized or incompatible data leaves the
active session unchanged.

### M3 — Post-C6 Model B+ Developer Preview

**State:** **TODO**

**Depends on:** M2, ratified B+ references/fixtures, the B+ 64K workstream,
selected C4/C5 evidence and completed C6 contracts.

M3 passes when:

- Model B still passes M2;
- a user selects Model B+ 64K, imports compatible firmware, boots to BASIC and
  types/runs the M1 program with continuous video and audio;
- a B+ snapshot round trip continues deterministically;
- one timing-sensitive fixture shows measured C4 progress;
- one safe mount/protect/export workflow shows C5 progress on a named profile;
- inspection, bounded breakpoint/watchpoint control, one atomic memory
  transaction and one BASIC program-boundary round trip exercise C6; and
- the application displays version, profile, runtime health and known fidelity
  limits.

M3 is a limited developer preview. It is not preservation-grade or complete
BBC emulation.

## Specification sequence

Order is topological, not total serialization. After row 1, independent M1 and
C3 work may overlap when their dependencies are met. A sequence row is a
planning container; every child requires its own stable feature identity and
acceptance evidence.

| Order | State | Specification | Depends on | Demonstrable outcome |
| --- | --- | --- | --- | --- |
| 1 | **DONE** | `machine-target-profile` | C2 | Extensible Model B/B+ identity across core, C, Swift and host; later identifiers reject safely. |
| 2 | **NEXT** | `machine-firmware-onboarding` | 1 | Import, validate, assign and remember user-owned OS/language ROMs with actionable guidance. |
| 3 | **TODO** | `machine-runtime-presentation` | C1, C2 | Sustained owner execution and completed-frame presentation without host-driven time. |
| 4 | **TODO** | `machine-audio-output` | C2 | AVAudioEngine consumes bounded audio with measured pressure and recoverable device lifecycle. |
| 5 | **TODO** | `machine-keyboard-controls` | 3 | BBC mapping plus distinct accessible run, pause, reset, BREAK, capture and full-screen controls. |
| 6 | **TODO** | `machine-mvp-validation` | 2-5 | Automated/manual evidence proves M1 end to end. |
| 7 | **TODO** | `machine-ios-ipados-adaptation` | M1 | Maintained iPhone/iPad layouts, input and accessibility evidence. |
| 8 | **TODO** | `snapshot-format-v1` | C1, 1 | Bounded versioned profile-aware state envelope. |
| 9 | **TODO** | `snapshot-round-trip` | 8 | Deterministic CPU, memory, ROM and device continuation. |
| 10 | **TODO** | `snapshot-mounted-media` | 8 | Media identity/private modifications restore or reject explicitly. |
| 11 | **TODO** | `snapshot-host-boundary` | 8-10 | Owned C/Swift save-load values and recoverable failures. |
| 12 | **TODO** | `machine-session-lifecycle` | M1, 7-11 | Background, termination and restoration prove M2. |
| 13 | **TODO** | C4 bus-cycle sequence | 8-11 | Reference-backed bus traces improve timing without breaking M2. |
| 14 | **TODO** | `bplus-reference-fixture-set` | 1 | Primary references and lawful fixtures bind processor, memory, display, firmware and controller claims. |
| 15 | **TODO** | Model B+ 64K profile sequence | 1, 8-11, 13, 14 | Selection, boot, display/audio, persistence and selected storage satisfy the B+ portion of M3. |
| 16 | **TODO** | C5 disc-core sequence | 8-11; relevant 13 | Bounded deterministic controller/media contracts support named disc profiles. |
| 17 | **TODO** | `machine-disk-workflow` | 16 | Safe mount, diagnosis, protection and explicit export. |
| 18 | **TODO** | C5 tape/file-core sequence | 8-11; relevant 13 | Bounded deterministic cassette/file contracts support named formats. |
| 19 | **TODO** | `machine-tape-file-workflow` | 18 | Safe open, diagnosis and explicit export. |
| 20 | **TODO** | C6 inspection/editor bridge sequence | C1, 8-11 | Stable inspection, bounded control, atomic transactions and BASIC boundaries. |
| 21 | **TODO** | `machine-inspector` | 20 inspection/control | Read-only state and bounded breakpoint/watchpoint product workflow. |
| 22 | **TODO** | `machine-c6-bridge-validation` | 20 transaction/BASIC contracts, 21 | Product demonstrations close the C6 portion of M3. |
| 23 | **TODO** | BASIC transformation/editing sequence | 20 program boundary | Separately specified tokenization, labels, inject/retrieve and conflict handling. |
| 24 | **TODO** | `product-release-readiness` | Selected maintained gates | Device, accessibility, onboarding, privacy, legal and beta evidence support a release decision. |

## Scope and evidence rules

- Product tests prove user journeys; core tests prove deterministic behavior;
  cross-strand tests prove the boundary.
- Every numeric compatibility, timing, latency or performance claim names its
  fixture, host/toolchain, observation interval and tolerance.
- Proprietary firmware and user media remain outside the repository.
- Accessibility and recovery are acceptance requirements of each user-facing
  slice.
- User-facing acceptance builds and launches the maintained application,
  executes the documented journey and records observed visual, audio and
  interaction results; unit tests alone do not close a row or gate.
- A row changes state only with its feature artifacts and acceptance evidence.
- Work not named here is unscheduled. Amend this file before specifying it.
