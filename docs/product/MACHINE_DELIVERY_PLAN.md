# Machine delivery plan

**Status:** Sole canonical forward programme authority
**Updated:** 2026-08-01

This is the only document that commits scope, selects next work, orders delivery
or defines programme gates. Every new feature must trace to a named row or gate
here before its Spec Kit artifacts are created.

[STATUS.md](../STATUS.md) owns verified state. [ARCHITECTURE.md](../ARCHITECTURE.md)
owns current boundaries. [IMPLEMENTATION_CONSTRAINTS.md](../IMPLEMENTATION_CONSTRAINTS.md)
owns technical constraints for unfinished work. [VISION.md](VISION.md) owns
durable intent and [DESKTOP_EXPERIENCE.md](DESKTOP_EXPERIENCE.md) owns the agreed
desktop interaction direction. None may create a competing backlog.

## State vocabulary

| State | Meaning |
| --- | --- |
| **DONE** | Implemented and backed by passed acceptance evidence linked from `STATUS.md`. |
| **ACTIVE** | The single feature named by `.specify/feature.json` is in implementation. |
| **PLANNING** | The single feature named by `.specify/feature.json` has active Spec Kit artifacts but implementation has not started. |
| **NEXT** | First committed work, not started. |
| **TODO** | Committed work, not delivered. |
| **RESERVED** | Named future option, not committed for delivery. |

## Current ledger

| Item | State | Boundary |
| --- | --- | --- |
| C0 baseline evidence | **DONE** | Reproducible foundation evidence |
| C1 runtime ownership | **DONE** | Single owner and recoverable C++/C/Swift boundary |
| C2 bounded output and Xcode delivery | **DONE** | Owned frame/audio output, diagnostics and maintained Apple builds |
| Active feature | — | No active Spec Kit feature; `005-model-b-workflow` acceptance remains open below |
| `machine-target-profile` | **DONE** | Extensible identity transport and safe rejection across core, C, Swift and host |
| M1 running Model B desktop | **TODO** | First complete functional macOS journey |
| M2 native desktop experience | **TODO** | AppKit-first machine window and bare terminal host |
| M3 durable desktop machine | **TODO** | Snapshots, lifecycle recovery and safe first media workflow |
| M4 desktop developer preview | **TODO** | Model B+ 64K, inspection and developer workspace |
| M5 iPhone and iPad adaptation | **TODO** | Platform-appropriate mobile experience over the portable runtime |

Completed foundations and profile identity are not a completed application. The
current host supplies bounded firmware onboarding, video presentation, keyboard
and control paths; direct ROM-backed input acceptance, application audio, the
native desktop experience, terminal host, snapshots and Model B+ behavior
remain undelivered.

## Programme focus

Delivery is desktop-first. M1 proves a functioning Model B on macOS, M2 makes
that host a deliberate native desktop product, and M3 makes the desktop machine
recoverable and useful with media. M4 then adds the deeper developer and Model
B+ experience. M5 retains the committed iPhone/iPad product, but mobile layout
and lifecycle work does not constrain the design or sequencing of the first
three desktop milestones.

After M3, M4 and M5 may advance independently where their prerequisites allow.
Portable core, C ABI and BeebKit contracts continue to be tested throughout;
desktop-first means host-product priority, not Apple-only core design.

## Machine profiles

Profiles use a stable base identifier plus explicit versioned expansions. They
are not a closed two-value enum.

- **Model B — committed and supported:** its identity flows across core, C,
  Swift and host construction/query; the complete application workflow is
  still required for M1.
- **Model B+ 64K — committed identity, unavailable behavior:** its distinct
  identity and host request are recognised, but construction rejects without
  fallback. Reference-led machine implementation remains required for M4.
- **B+ 128K, Master-family, Tube, Econet, ADFS and other storage/peripheral
  expansions — RESERVED:** each needs a product case, primary references,
  lawful fixtures and a plan amendment.

The active application or terminal session reports its profile. Profile choice,
hardware, firmware, ROM banks and compatibility configuration belong in the
Settings/configuration surface rather than the main macOS toolbar. Unknown
identity rejects without mutating the active machine and never falls back to
another profile.

## Delivery gates

### M1 — Running Model B Desktop

**State:** **TODO**

M1 passes when a macOS user can, without shell commands:

1. import and assign user-owned OS and BASIC ROMs;
2. boot Model B to BASIC;
3. type and run a documented short BASIC program;
4. receive continuous video from C2 completed-frame output;
5. hear continuous audio from C2 bounded audio output; and
6. run, pause, reset and BREAK while seeing useful runtime/output diagnostics.

The maintained run must keep the main actor responsive, keep host clocks out of
emulated time, recover to actionable states and bundle no proprietary bytes.
The transitional SwiftUI host may close this functional gate; M2 replaces its
macOS interaction shell with the committed AppKit-first experience.

### M2 — Native Desktop Experience

**State:** **TODO**

**Depends on:** M1 and the four desktop-experience slices below.

M2 passes when:

- AppKit owns the macOS window, responder chain, menus, toolbar, settings,
  lifecycle actions and accessibility surface;
- the main window presents the toolbar, full-width CRT area, running-machine
  footer and optional BBC keyboard drawer in that order;
- the active raster remains a centred 4:3 viewport with mode-correct pixel
  aspect, underscan and nearest-neighbour presentation during resize and full
  screen;
- machine and hardware selection live in Settings, whose live, pause-required,
  restart-required and destructive changes use targeted safety interlocks;
- a user can load numbered or unnumbered program text through a previewed,
  cancellable emulated-keyboard path without direct RAM or screen injection;
- routine media access has a native home in the toolbar and menus, while only
  already-supported safe media operations are enabled; and
- a bare terminal host provides a chrome-free keyboard/display connection to
  the same production runtime and a deterministic noninteractive test mode.

Every M2 user-facing slice includes visual, resize, keyboard-focus,
accessibility and safety-friction review. The detailed interaction contract is
in [DESKTOP_EXPERIENCE.md](DESKTOP_EXPERIENCE.md).

### M3 — Durable Desktop Machine

**State:** **TODO**

**Depends on:** M2, the version-1 snapshot contract, desktop session continuity
and the first safe disc workflow.

M3 passes when background, termination and explicit recovery preserve the
selected profile, CPU, RAM, ROM selection, devices and supported mounted-media
state. Stale output does not cross restore. Corrupt, oversized or incompatible
data leaves the active session unchanged. At least one disc workflow supports
explicit mount, protection and export without silently modifying source media.

Reset, restart-required settings and destructive media actions provide
recoverable checkpoints or targeted warnings proportional to actual risk. M3
is the first candidate point for a desktop beta/release-readiness decision.

### M4 — Desktop Developer and Model B+ Preview

**State:** **TODO**

**Depends on:** M3, ratified B+ references/fixtures, the B+ 64K workstream,
selected timing evidence and completed inspection/editor-bridge contracts.

M4 passes when:

- Model B still passes M3;
- a user selects Model B+ 64K in Settings, imports compatible firmware, boots
  to BASIC and runs the M1 program with continuous video and audio;
- a B+ snapshot round trip continues deterministically;
- one timing-sensitive fixture shows measured bus-cycle progress;
- the separate AppKit developer workspace exposes stable registers,
  disassembly, stack, memory, breakpoints/watchpoints and selected device state
  without borrowing or mutating live core storage; and
- one atomic memory transaction and one BASIC program-boundary round trip
  exercise the inspection/editor bridge.

M4 is a limited desktop developer preview, not preservation-grade or complete
BBC emulation.

### M5 — iPhone and iPad Adaptation

**State:** **TODO**

**Depends on:** M3 and the portable session contracts used by the maintained
desktop product. It does not require every optional M4 developer tool.

M5 passes when iPhone and iPad have platform-appropriate machine, input, media,
settings, lifecycle and accessibility experiences using the same core, C ABI
and BeebKit behavior as macOS. Mobile controls may differ, but firmware,
profile, snapshot, media-safety and emulated-time contracts may not fork.

Device, rotation, background/termination, hardware-keyboard, touch-input,
VoiceOver and Dynamic Type evidence is required before mobile release claims.

## Delivery strategy

The desktop critical path is M1, then M2, then M3. M4 deepens the desktop
product; M5 adapts the stable machine to iPhone and iPad. Work is divided by
demonstrable outcome rather than implementation layer: one bounded feature may
cross core, C, Swift and host boundaries when those changes are inseparable
from the same user journey.

Only the feature named by `.specify/feature.json` is active in this repository.
Rows whose dependencies are met may be selected in either order, allowing
portable contracts and fixtures to advance without creating competing active
features.

### Critical-path slices

| Order | State | Feature or gate | Depends on | Demonstrable outcome |
| --- | --- | --- | --- | --- |
| 1 | **DONE** | `machine-target-profile` | C2 | Extensible Model B/B+ identity across core, C, Swift and host; later identifiers reject safely. |
| 2 | **NEXT** | `machine-model-b-workflow` acceptance closure | 1, C1, C2 | Direct observation proves the documented BASIC program can be typed and run and that a rejected firmware candidate preserves the working session. |
| 3 | **TODO** | `machine-audio-output` | C2 | AVAudioEngine consumes bounded audio during the maintained Model B run with measured pressure and recoverable device lifecycle. |
| M1 | **TODO** | Running Model B desktop gate | 2-3 | One integrated macOS journey proves every M1 acceptance bullet. |
| 4 | **TODO** | `macos-appkit-machine-window` | M1 | AppKit replaces the transitional root and owns native window, menu, toolbar, responder-chain, settings and accessibility behavior. |
| 5 | **TODO** | `macos-crt-input-experience` | 4 | Full-width CRT, mode-correct sharp scaling, footer, keyboard drawer, capture/release and UI/UX evidence work as one desktop interaction. |
| 6 | **TODO** | `machine-program-text-input-v1` | 4; M1 input | Numbered or unnumbered source is previewed and delivered as cancellable paced machine keystrokes; direct BASIC workspace mutation remains excluded. |
| 7 | **TODO** | `machine-terminal-host-v1` | M1; C1; C2 | A chrome-free interactive terminal and deterministic scripted mode use the production runtime for keyboard, display, controls, diagnostics and emulator regression evidence. |
| M2 | **TODO** | Native desktop experience gate | 4-7 | One integrated AppKit journey plus terminal parity evidence proves the complete M2 contract. |
| 8 | **TODO** | `snapshot-continuity-v1` | 1, C1 | A bounded profile-aware format round-trips machine state through owned C/Swift values; invalid restore is failure-atomic and stale output is discarded. |
| 9 | **TODO** | `machine-session-continuity` | M2, 8 | The AppKit application saves, restores and recovers the active session across termination and restart-required changes. |
| 10 | **TODO** | `machine-disc-workflow-v1` | 8; selected timing evidence | One named disc profile supports safe mount, diagnosis, protection and explicit export through native media controls. |
| M3 | **TODO** | Durable desktop machine gate | 8-10 | The maintained desktop journey proves recovery and safe first-media behavior. |
| 11 | **TODO** | `bplus-reference-fixture-set` | 1 | Primary references and lawful fixtures bind selected Model B+ processor, memory, display, firmware and controller claims. |
| 12 | **TODO** | `timing-fixture-v1` | 8 | The smallest bus-phase implementation needed by one named timing-sensitive M4 fixture. |
| 13 | **TODO** | `machine-model-bplus-64k-workflow` | M3, 11-12 | Model B+ selection, firmware boot, display/audio and snapshot continuation work end to end without weakening Model B. |
| 14 | **TODO** | `machine-inspection-basic-bridge` | 8, C1 | Stable inspection, bounded breakpoint/watchpoint control, one atomic memory transaction and one BASIC boundary round trip cross the owner safely. |
| 15 | **TODO** | `macos-developer-workspace` | 14; M2 | A separate AppKit workspace presents trace, registers, disassembly, stack, memory and selected devices from stable observations. |
| M4 | **TODO** | Desktop developer and Model B+ gate | 13-15 | One integrated desktop developer-preview journey proves the complete M4 contract. |
| 16 | **TODO** | `machine-ios-ipados-adaptation` | M3 | iPhone/iPad layouts, input, media, settings and accessibility preserve the shared machine contracts. |
| 17 | **TODO** | `machine-ios-ipados-lifecycle` | 8, 16 | Mobile background/termination recovery proves the same snapshot and stale-output rules on devices. |
| M5 | **TODO** | iPhone and iPad adaptation gate | 16-17 | Maintained device journeys prove the complete M5 contract without weakening desktop or portable evidence. |

Rows 8, 11, 12 and 14 may begin once their dependencies pass even when their
consumer milestone is later. After M3, M4 and M5 may proceed independently.
Terminal automation may support every milestone, but a terminal pass does not
replace direct AppKit or device acceptance for a user-facing claim.

### Committed work outside the milestone critical paths

| State | Feature | Earliest start | Delivery purpose |
| --- | --- | --- | --- |
| **TODO** | `machine-tape-file-workflow` | 8; selected timing evidence | Safe open, diagnosis and explicit export for named cassette/file formats. |
| **TODO** | `basic-transformation-editing` | 14 | Deterministic tokenization, labels, inject/retrieve and conflict handling after the program boundary is proven. |
| **TODO** | `desktop-release-readiness` | M3 | macOS accessibility, onboarding, privacy, legal, packaging and beta evidence support a desktop release decision. |
| **TODO** | `mobile-release-readiness` | M5 | Device, App Store, privacy, onboarding and beta evidence support an iPhone/iPad release decision. |

## Scope and evidence rules

- Product tests prove user journeys; core tests prove deterministic behavior;
  cross-strand and terminal tests prove the production boundary.
- The portable C++20 core must remain independent of AppKit, UIKit, SwiftUI,
  Foundation, Terminal/TTY handling, ANSI rendering, Metal and audio devices.
- Interactive and scripted terminal modes use the production `MachineRuntime`
  through the stable C ABI; Apple parity tests prove BeebKit preserves the same
  semantics. Hosts may adapt presentation but never replace the machine with a
  parallel console implementation.
- Every numeric compatibility, timing, latency or performance claim names its
  fixture, host/toolchain, observation interval and tolerance.
- Proprietary firmware and user media remain outside the repository.
- Accessibility, recovery and hands-on UI/UX review are acceptance requirements
  of each user-facing slice.
- During implementation, run the narrow failing test and immediately affected
  boundary checks. At slice completion, run its aggregate and affected wider
  regressions. Reserve the full maintained matrix and complete user journey for
  M1-M5 and release closure.
- User-facing acceptance builds and launches the changed host and observes the
  changed journey. Terminal or unit automation alone cannot close an AppKit or
  mobile row.
- A milestone gate is closed by the final contributing feature's integrated
  evidence. It does not require a documentation-only validation feature.
- A row changes state only with its feature artifacts and acceptance evidence.
- Work not named here is unscheduled. Amend this file before specifying it.
