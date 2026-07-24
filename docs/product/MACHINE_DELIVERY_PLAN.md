# Machine delivery plan

**Status:** Sole canonical forward programme authority
**Updated:** 2026-07-24

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
| Active feature | **NONE** | `.specify/feature.json` is empty after target-profile archival |
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

**Depends on:** M1 and the version-1 snapshot/host lifecycle slices.

M2 passes when background/termination restoration preserves the selected
profile, CPU, RAM, ROM selection, devices and mounted-media state. Stale output
does not cross restore. Corrupt, oversized or incompatible data leaves the
active session unchanged.

M2 is proved first on the maintained macOS application. iOS/iPadOS adaptation
remains committed release work after M1, but does not delay the portable
snapshot contract or macOS continuity evidence.

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

## Delivery strategy

The critical path is M1, then M2, then M3. Work is divided by demonstrable
outcome rather than by implementation layer: one bounded feature may cross
core, C, Swift and host boundaries when those changes are inseparable from the
same user journey. Features must not be split merely to create separate
firmware, presentation, keyboard or validation phases.

Only the feature named by `.specify/feature.json` is active in this repository.
Rows whose dependencies are already met may be selected in either order, which
keeps specialist or fixture work available when the critical-path slice is
temporarily blocked without creating multiple competing active features.

### Critical-path slices

| Order | State | Feature or gate | Depends on | Demonstrable outcome |
| --- | --- | --- | --- | --- |
| 1 | **DONE** | `machine-target-profile` | C2 | Extensible Model B/B+ identity across core, C, Swift and host; later identifiers reject safely. |
| 2 | **NEXT** | `machine-model-b-workflow` | 1, C1, C2 | A macOS user imports and remembers compatible OS/language ROM assignments, boots Model B, sees continuous completed frames, types/runs the M1 BASIC program and uses accessible run, pause, reset and BREAK controls with actionable diagnostics. |
| 3 | **TODO** | `machine-audio-output` | C2 | AVAudioEngine consumes bounded audio during the maintained Model B run with measured pressure and recoverable device lifecycle. |
| M1 | **TODO** | Running Model B gate | 2-3 | One integrated macOS journey proves every M1 acceptance bullet; no standalone validation feature is required unless integration defects create new behavior work. |
| 4 | **TODO** | `snapshot-continuity-v1` | 1, C1 | One bounded profile-aware format round-trips CPU, memory, ROM selection, devices and supported mounted-media state through owned C/Swift values; invalid restore is failure-atomic and stale output is discarded. |
| 5 | **TODO** | `machine-session-continuity` | M1, 4 | The macOS application saves and restores the active session across background/termination recovery. |
| M2 | **TODO** | Continuity-complete gate | 4-5 | The maintained Model B application proves the complete M2 recovery journey. |
| 6 | **TODO** | `bplus-reference-fixture-set` | 1 | Primary references and lawful fixtures bind the selected Model B+ processor, memory, display, firmware and controller claims. |
| 7 | **TODO** | `timing-fixture-v1` | 4 | The smallest bus-phase implementation needed by one named timing-sensitive M3 fixture, without attempting full timing refinement. |
| 8 | **TODO** | `machine-model-bplus-64k-workflow` | M2, 6-7 | Model B+ 64K selection, firmware boot, display/audio and snapshot continuation work end to end without weakening Model B. |
| 9 | **TODO** | `machine-disc-workflow-v1` | 4; relevant 7 | One named disc profile supports bounded core behavior plus safe host mount, diagnosis, protection and explicit export. |
| 10 | **TODO** | `machine-inspection-basic-bridge` | 4, C1 | Stable inspection, bounded breakpoint/watchpoint control, one atomic memory transaction and one BASIC program-boundary round trip work through the application. |
| M3 | **TODO** | Model B+ developer-preview gate | 8-10 | One integrated maintained-application journey proves every M3 acceptance bullet and records the known fidelity limits. |

Rows 6, 9 and 10 may be selected as soon as their dependencies pass; they do
not need to wait for unrelated work in the table. Rows 6 and 7 deliberately
bound B+ research and timing to the evidence M3 actually claims.

### Committed work outside the M1-M3 critical path

| State | Feature | Earliest start | Delivery purpose |
| --- | --- | --- | --- |
| **TODO** | `machine-ios-ipados-adaptation` | M1 | Maintained iPhone/iPad layouts, input, lifecycle and accessibility evidence before release claims include those platforms. |
| **TODO** | `machine-tape-file-workflow` | 4; selected timing evidence | Safe open, diagnosis and explicit export for named cassette/file formats; not required by M3's single-media demonstration. |
| **TODO** | `basic-transformation-editing` | 10 | Deterministic tokenization, labels, inject/retrieve and conflict handling after the program boundary is proven. |
| **TODO** | `product-release-readiness` | Selected maintained gates | Device, accessibility, onboarding, privacy, legal and beta evidence support a release decision. |

## Scope and evidence rules

- Product tests prove user journeys; core tests prove deterministic behavior;
  cross-strand tests prove the boundary.
- Every numeric compatibility, timing, latency or performance claim names its
  fixture, host/toolchain, observation interval and tolerance.
- Proprietary firmware and user media remain outside the repository.
- Accessibility and recovery are acceptance requirements of each user-facing
  slice.
- During implementation, run the narrow failing test and the immediately
  affected boundary checks. At slice completion, run its focused aggregate and
  affected wider regressions. Reserve the full maintained matrix and complete
  end-to-end application journey for M1, M2, M3 and release closure.
- User-facing slice acceptance still builds and launches the maintained
  application, but observes the changed journey rather than repeating every
  unchanged milestone step. Unit tests alone do not close a user-facing row.
- A milestone gate is closed by the final contributing feature's integrated
  acceptance evidence. It does not require a documentation-only validation
  feature or duplicate implementation phase.
- A row changes state only with its feature artifacts and acceptance evidence.
- Work not named here is unscheduled. Amend this file before specifying it.
