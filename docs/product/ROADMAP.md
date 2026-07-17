# Product roadmap

**Status:** Canonical planning document  
**Updated:** 2026-07-17

This roadmap sequences product capabilities rather than promising dates. A
horizon advances when its exit outcomes are demonstrated; parallel fidelity
work may continue where it directly unblocks compatibility or a product slice.

## Status legend

- **Verified:** present in the current codebase with automated evidence.
- **Foundation:** usable by developers but not yet a complete user workflow.
- **Next:** the active product horizon.
- **Later:** intentionally sequenced after current dependencies.
- **Research:** requires evidence or a product decision before commitment.

## Foundation — portable machine core

**Status:** Foundation

Available now:

- dependency-free C++20 core and stable C boundary;
- complete documented NMOS 6502 instructions and arithmetic tests;
- BBC memory map and sideways ROM selection;
- partial System/User VIA, CRTC, Video ULA and keyboard matrix;
- bitmap modes and clean-room Mode 7 rendering;
- SN76489 register behavior and sample generation;
- logical SSD/DSD and 8271 sector operations;
- Swift wrapper, basic SwiftUI shell and document import;
- Linux/macOS CI, sanitizers, versioning and release checks.

Foundation exit condition:

- Retain the current green core, Swift and smoke-test suites while the first
  complete product workflow is built.

Detailed hardware fidelity remains tracked in [core status](../STATUS.md), and
technical sequencing belongs to the [core roadmap](../CORE_ROADMAP.md).

## Application delivery gates

The [Machine application delivery plan](MACHINE_DELIVERY_PLAN.md) connects the
product horizons to bounded core, host and cross-strand specifications. It
defines three cumulative evidence gates:

1. **M1 — Running Model B Machine:** firmware import, BASIC boot and input,
   continuous C2-backed video/audio, controls and diagnostics. M1 may complete
   before C3 and must not wait for Media or Editor work.
2. **M2 — Continuity-complete Machine:** M1 plus C3-backed backgrounding and
   safe session restoration.
3. **M3 — Post-C6 Model B+ Developer Preview:** M2 remains green for Model B;
   the same working application flow runs on the separately evidenced Model B+
   64K profile, demonstrates selected C4 and C5 progress, and consumes the
   completed C6 bridge contracts through bounded product demonstrations.

These gates prevent a collection of completed subsystems from being mistaken
for a working application. Every implementation slice still requires its own
Spec Kit feature artifacts.

## Horizon 1 — complete Machine experience

**Status:** Next

### 1. Host runtime and presentation

Verified technical prerequisite: the C1 core now owns sustained emulation on a
dedicated runtime thread with recoverable lifecycle commands and concurrent C
and Swift boundaries. The product still needs to adopt that capability in its
presentation loop.

- Drive the Machine UI through the sustained runtime without blocking the main
  actor.
- Add a bounded completed-frame queue and timestamped host presentation.
- Present frames through Metal with fixed 4:3 layout, nearest-neighbor scaling
  and correct 50-to-60/120 Hz repetition.
- Add an audio ring buffer suitable for an AVAudioEngine render callback.
- Expose underrun, frame age and emulation-rate metrics without putting host
  concerns into the core.

### 2. Machine controls and input

- Implement physical keyboard-to-matrix mapping with BBC-specific keys and
  chords, including BREAK and SHIFT+BREAK.
- Add an adaptive iOS/iPadOS keyboard overlay and searchable key help.
- Provide clear run, pause, reset, input-capture and full-screen behavior.
- Ensure controls and machine state are usable with VoiceOver and keyboard
  navigation.

### 3. Session continuity

- Define a versioned deterministic state snapshot in the core API.
- Save and restore CPU, RAM, ROM selection, devices and mounted-media state.
- Detect incompatible snapshots and recover without corrupting user data.

### 4. Machine validation

- Add redistributable boot fixtures and deterministic boot assertions.
- Add golden frames for representative bitmap modes and Mode 7.
- Measure 50 Hz stability, audio underruns and host-frame pacing over sustained
  runs.
- Establish a curated compatibility set with documented content provenance.

### Horizon 1 specification order

1. `machine-target-profile`
2. `machine-firmware-onboarding`
3. `machine-runtime-presentation`
4. `machine-audio-output`
5. `machine-keyboard-controls`
6. `machine-mvp-validation`
7. the four C3 core snapshot slices
8. `machine-session-lifecycle`

The presentation, audio, input and onboarding slices may be prepared in
parallel where their contracts do not overlap. `machine-mvp-validation` is the
M1 gate and `machine-session-lifecycle` is the M2 gate; neither is an umbrella
implementation feature.

Exit outcomes:

- A user can import firmware, boot, type and run a simple BASIC program.
- Video and audio run continuously without UI stalls or cross-thread races.
- Keyboard help makes BBC-specific input discoverable.
- A session survives app backgrounding and restoration.

## Horizon 2 — dependable Media experience

**Status:** Later

### 1. Disk workflow

- Add mount/eject, write-protect, drive-state and shift-boot controls.
- Expose catalog and controller errors in user-facing terms.
- Export modified disk images explicitly; never overwrite imported source
  media silently.
- Expand 8271 command, timing and error coverage against DFS fixtures.

### 2. Tape files

- Implement 6850 ACIA and Serial ULA behavior required by cassette I/O.
- Parse the required UEF chunks, carrier periods and gaps.
- Add WAV edge decoding with deterministic fixtures and checksum reporting.
- Reflect emulated motor state in the transport UI.

### 3. Live tape capture

- Build a microphone DSP pipeline only after file-based decoding is reliable.
- Provide levels, waveform, detected blocks and actionable recovery guidance.
- Measure success against a maintained recording corpus before publishing a
  reliability claim.

### Horizon 2 product specifications

1. `machine-disk-workflow`: mount/eject, protection, state, errors and explicit
   export over the selected C5 disk/controller capability.
2. `machine-tape-file-workflow`: UEF/WAV import, progress, motor state and
   recoverable decoding over the selected C5 cassette capability.
3. `machine-live-tape-capture`: microphone permission, DSP, feedback and corpus
   evidence only after the file workflow is dependable.

Exit outcomes:

- Users can understand, mount, protect and export DFS media safely.
- Supported tape files load deterministically with visible progress and errors.
- Live capture has evidence-backed success criteria or remains labelled beta.

## Horizon 3 — native BBC BASIC Editor

**Status:** Later

### 1. Core bridge capabilities

- Add controlled memory inspection and injection APIs.
- Add pause/transaction semantics so editor operations cannot race execution.
- Expose BASIC program boundaries and machine-side change detection.

### 2. Source transformation

- Implement BASIC II tokenization and detokenization from primary references.
- Resolve labels deterministically for GOTO, GOSUB and related constructs.
- Preserve a stable mapping between labels and generated line numbers.
- Add property and fixture tests for round-trip behavior.

### 3. Editing workflow

- Provide source editing, diagnostics, inject/run and retrieve operations.
- Show a semantic diff when RAM and source diverge.
- Require explicit conflict resolution; never discard either representation
  silently.

### Horizon 3 product specifications

1. `machine-inspector`: accessible read-only CPU, memory and device inspection
   over C6 stable observations.
2. `basic-source-transformation`: deterministic BASIC II tokenization,
   detokenization and label mapping.
3. `basic-editing-workflow`: edit, inject, run, retrieve, diff and explicit
   conflict resolution over C6 transactions and program boundaries.

Exit outcomes:

- A label-based program can be injected, run and recovered without unexplained
  data loss.
- Machine-side edits are detected and presented before source replacement.

## Horizon 4 — product readiness

**Status:** Later

- Complete responsive iPhone, iPad and macOS layouts.
- Finish onboarding, empty states, contextual help and error recovery.
- Validate VoiceOver, Dynamic Type, contrast and reduced-motion behavior.
- Establish en-GB localization and translation-ready strings.
- Prepare privacy descriptions, import-only reviewer notes and legal notices.
- Run compatibility, sustained performance and device-matrix testing.
- Decide firmware onboarding, application naming and release OS requirements.

Exit outcome:

- The application is understandable, accessible, legally reviewable and stable
  enough for an external beta followed by App Store submission.

## Emulator dependency strand

Hardware fidelity, runtime contracts and core API work are planned separately
in the [emulator core roadmap](../CORE_ROADMAP.md). Product horizons state the
user outcome; they should not prescribe CPU/device internals or turn every core
accuracy improvement into a product milestone.

## Research horizon

The following require explicit decisions before they enter delivery planning:

- clean-room default firmware;
- graphical keyboard versus overlay investment;
- inspector and time-travel debugging scope;
- public-domain content gallery and education partnerships;
- BBC Master, Tube, Econet, ADFS or joystick support;
- monetization and entitlement boundaries.

## Sequencing rules

- Deliver vertical user workflows, not isolated subsystems.
- Make M1 the first working-application gate; do not defer host adoption until
  C5 or C6.
- Preserve Model B as a regression profile while Model B+ 64K enters only
  through its explicit machine-profile specifications and evidence.
- Do not let host refresh timing become the source of emulated machine time.
- Stabilize file-based tape loading before microphone capture.
- Establish transactional memory/state APIs before building the editor.
- Add observable metrics before optimizing frame or audio pipelines.
- Keep imported content local and make destructive operations explicit.
- Update this roadmap when product priorities change; update core
  `STATUS.md` only when implementation evidence changes.

## Definition of done

A roadmap item is complete only when its user-visible behavior works, relevant
automated tests pass, performance or compatibility claims have evidence,
failure and recovery states are handled, accessibility has been considered and
the current documentation reflects the shipped boundary.
