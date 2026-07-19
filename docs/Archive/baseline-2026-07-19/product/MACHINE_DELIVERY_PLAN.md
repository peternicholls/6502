# Machine application delivery plan

**Status:** Sole canonical forward programme authority
**Updated:** 2026-07-19

This plan turns the emulator foundation into a working native application while
keeping product outcomes, portable core behavior and Apple host integration in
their proper strands. It is a programme map, not an umbrella feature
specification. Every named slice enters the repository through its own bounded
Spec Kit specification, plan, tasks, analysis and verified implementation.

## Authority contract

This file is the explicit and only authority for delivery order, current
direction, programme gates, named future slices and promotion of later work.
All new product, core and cross-strand specifications must trace to a row or
gate here before planning begins. Work not named here is not scheduled: amend
this plan first through a reviewed documentation change.

Other documents have deliberately narrower roles:

- [VISION.md](VISION.md) records durable product intent;
- [ROADMAP.md](ROADMAP.md) catalogues possible product capabilities;
- [CORE_ROADMAP.md](../CORE_ROADMAP.md) records core dependencies, invariants
  and bounded technical decompositions;
- [ARCHITECTURE.md](../ARCHITECTURE.md) records current boundaries;
- [STATUS.md](../STATUS.md) records only verified implementation evidence; and
- [LEGACY_DECISIONS.md](LEGACY_DECISIONS.md) interprets historical material.

None of those files may independently select next work, change priority, add a
delivery commitment or redefine an M1/M2/M3 gate. Completed feature artifacts
remain evidence and decision history, not forward authority. In any conflict,
this plan governs direction while the constitution governs how work is
specified and verified and `STATUS.md` governs claims about what exists.

The historical [product requirements document](../Archive/PRD.md) supplies
useful intent: an authentic Machine, dependable Media, a modern BBC BASIC
Editor, inspection, accessible Apple-platform UX and a primary journey in which
a user types and runs a program. Its milestone ordering, implementation
assumptions and unvalidated numeric targets are not current requirements; the
[legacy decision register](LEGACY_DECISIONS.md) records their disposition.

## Delivery status — 2026-07-19

Status terms are deliberately strict:

- **DONE:** implemented and backed by the evidence linked from `STATUS.md` and
  its completed feature artifacts.
- **ACTIVE:** an approved feature is in implementation and is named by
  `.specify/feature.json`.
- **NEXT:** committed first work, but no feature has started yet.
- **TODO:** committed by this plan but not delivered.
- **RESERVED:** an acknowledged future option, not a delivery commitment.

### Work done

| Delivered foundation | State | Evidence boundary |
| --- | --- | --- |
| C0 — baseline evidence | **DONE** | Completed specification plus C0 aggregate evidence in `STATUS.md` |
| C1 — runtime ownership | **DONE** | Completed specification plus runtime, replay and recovery evidence in `STATUS.md` |
| C2 — bounded output and Xcode contracts | **DONE** | Completed specification plus frame, audio, diagnostics, lifetime and project evidence in `STATUS.md` |
| Single programme-authority governance | **DONE** | Constitution v1.4.0, fail-closed Spec Kit pointer resolution and this canonical plan |

These completed foundations are not a completed application. In particular,
firmware onboarding, host presentation, host audio, complete keyboard controls,
M1, snapshots, Model B+ 64K and M2/M3 remain undelivered.

### Work active now

**NONE.** `.specify/feature.json` is empty. No implementation feature is active.

### Work yet to do

- **NEXT:** create and complete `machine-target-profile`.
- **TODO:** complete specification rows 2–22 below subject to their explicit
  dependencies.
- **TODO:** achieve M1, then M2, then M3. None of these gates is currently
  achieved.
- **RESERVED:** B+ 128K, Master-family, Tube, Econet, additional storage and
  other expansion options require later plan amendments before implementation.

## Target profiles

Delivery uses an extensible profile identity so compatibility and persisted
state cannot silently change meaning. A configuration consists of a stable base
machine identifier plus versioned, explicitly supported expansion identifiers;
it is not a closed two-value enum.

Committed selectable profiles are:

- **Model B — CORE PROFILE DONE; APPLICATION TODO:** the implemented and
  verified first core profile. It is the shortest path to the first working
  Machine application and remains a permanent regression profile. M1 has not
  yet been achieved.
- **Model B+ 64K — TODO:** the planned post-C6 application profile. Its exact
  processor, memory, display, firmware and disc-controller behavior must be
  fixed from primary references and compatibility fixtures before
  implementation claims.

M1 may expose only the verified Model B profile. M3 must expose both Model B and
Model B+ 64K as explicit options and keep their firmware, snapshot, media and
compatibility evidence separate.

Named **RESERVED** later profile and expansion options are:

- Model B+ 128K;
- BBC Master 128 and other separately researched Master-family revisions;
- Acorn Tube interfaces and individually identified second processors;
- Econet and other network expansions;
- ADFS and additional storage/controller profiles; and
- analogue, speech, joystick and other peripheral expansions.

These names reserve an extensible architectural path; they are not part of M3
and carry no compatibility or schedule claim. Each requires a separate product
case, primary-reference research, lawful fixtures, bounded profile or expansion
specifications, and an explicit amendment to this plan before implementation.

The application must display the selected profile and reject firmware,
snapshots or media that are incompatible with it. No profile may be inferred
only from file names or host UI state. Unknown profile or expansion identifiers
must reject without mutating an active machine; they must never fall back to
Model B or Model B+ behavior.

## Platform scope

M1 is a maintained macOS user workflow; the shared iOS Simulator build remains
green but is not mistaken for a complete touch-input experience. Adaptive
iPhone/iPad keyboard and layout work enters through the separately named
`machine-ios-ipados-adaptation` slice before those platforms claim M1. M2 adds
platform-appropriate lifecycle evidence, including iOS/iPadOS background and
restoration behavior. M3 reruns
the maintained host workflows selected by its feature specification; external
beta and App Store device coverage require the later
`product-release-readiness` gate.

## Delivery gates

### M1 — Running Model B Machine

**Gate state:** **NOT ACHIEVED — TODO**

**Depends on:** Integrated C1 and C2; the M1 host slices below.

M1 is the first working application and must not wait for C5 or C6. It passes
only when a user can:

1. import user-owned OS and BASIC ROMs through the application;
2. boot to a BASIC-ready state without shell commands;
3. type and run a documented short BASIC program through host keyboard mapping;
4. see continuous video consumed from C2 completed-frame output;
5. hear continuous audio consumed from C2 bounded audio output; and
6. use distinct run, pause, reset and BREAK controls while diagnostics expose
   emulation rate, frame pressure or age, and audio pressure.

The maintained acceptance run must prove that the main actor remains responsive,
host refresh and audio callbacks never drive emulated time, failures recover to
an actionable state, and no proprietary ROM bytes enter the repository.

### M2 — Continuity-complete Machine

**Gate state:** **NOT ACHIEVED — TODO**

**Depends on:** M1, completed C3 snapshot contracts and
`machine-ios-ipados-adaptation`.

M2 passes when backgrounding and restoration preserve the selected profile,
CPU, RAM, ROM selection, devices and mounted-media state; stale frame/audio
output is not presented after restore; corrupt, oversized or incompatible state
is rejected without partially mutating the active session.

### M3 — Post-C6 Model B+ Developer Preview

**Gate state:** **NOT ACHIEVED — TODO**

**Depends on:** M2, the ratified Model B+ reference/fixture set, the Model B+
64K profile workstream, completed C6 bridge contracts, and the selected
compatibility/media evidence named by its feature specifications.

M3 is the requested post-C6 outcome. It passes only when:

- the Model B regression profile still passes M2;
- a user selects Model B+ 64K, imports compatible user-owned firmware, boots to
  BASIC, types and runs the M1 program, and receives continuous video and audio;
- a Model B+ snapshot round-trip preserves the profile and continues
  deterministically;
- one curated timing-sensitive case demonstrates progress from C4;
- one safe mount, protection and explicit-export workflow demonstrates progress
  from C5 on a named supported profile, while any B+ controller limitation is
  reported separately; and
- the completed C6 contracts are exercised through a read-only inspector,
  bounded breakpoint/watchpoint control, a validated atomic memory transaction,
  and a BASIC program-boundary round trip, without racing execution or losing
  data.

M3 is a limited Developer Preview, not a claim of preservation-grade or
complete BBC emulation. The application must expose its version, active profile,
runtime health and a route to the current `STATUS.md` fidelity catalogue.

## Specification sequence

Feature numbers are assigned only when a slice is selected. Names below are
stable planning identities, not permission to combine them into one sprint.
The numeric order is a readable topological sequence, not a demand for total
serialization: after row 1 completes, independent M1 and C3 slices may proceed
in parallel when their own dependencies are satisfied. A sequence row is a
planning container, not an umbrella specification. Each child must receive a
stable named identity in its own feature directory and satisfy the container's
entry dependencies before implementation.

| Order | Delivery state | Specification | Strand | Depends on | Independently demonstrable outcome |
| --- | --- | --- | --- | --- | --- |
| 1 | **NEXT — not started** | `machine-target-profile` | Cross-strand | C2 | An extensible profile/expansion identity is explicit across core, C, Swift and host configuration; Model B and Model B+ 64K are ratified while later identifiers reject safely. |
| 2 | **TODO — not started** | `machine-firmware-onboarding` | Product/cross-strand | Target profile | The app imports, validates, assigns and locally remembers user-owned OS and sideways ROMs without bundling content, and gives actionable compatibility/import guidance. |
| 3 | **TODO — not started** | `machine-runtime-presentation` | Cross-strand | C1, C2 | The host uses sustained runtime ownership and C2 frame dequeue without main-actor emulation or host-driven machine time. |
| 4 | **TODO — not started** | `machine-audio-output` | Cross-strand | C2 | AVAudioEngine consumes bounded audio with measured latency/pressure and recoverable device lifecycle behavior. |
| 5 | **TODO — not started** | `machine-keyboard-controls` | Product/cross-strand | Runtime presentation | Physical input, BBC mapping, key help, capture, full screen, run, pause, reset and BREAK are distinct and accessible. |
| 6 | **TODO — not started** | `machine-mvp-validation` | Cross-strand | Slices 2–5 | The automated/manual evidence bundle proves M1 from firmware import through BASIC execution and sustained output. |
| 7 | **TODO — not started** | `machine-ios-ipados-adaptation` | Product | M1 | iPhone and iPad gain maintained layouts, input and accessibility evidence before claiming the M1 journey. |
| 8 | **TODO — not started** | `snapshot-format-v1` | Core C3 | C1 safe point, target profile | A bounded, versioned envelope records machine-profile and expansion identity plus architectural state rules. |
| 9 | **TODO — not started** | `snapshot-round-trip` | Core C3 | Snapshot format | CPU, RAM, ROM selection and devices continue deterministically after restore. |
| 10 | **TODO — not started** | `snapshot-mounted-media` | Core C3 | Snapshot format | Mounted-media identity and modified private state restore or reject explicitly. |
| 11 | **TODO — not started** | `snapshot-host-boundary` | Core C3 | C3 state slices | C and Swift callers save/load with owned data and recoverable failures. |
| 12 | **TODO — not started** | `machine-session-lifecycle` | Cross-strand | M1, C3, `machine-ios-ipados-adaptation` | Background, termination and restoration satisfy M2 without stale host output. |
| 13 | **TODO — not started** | C4 bus-cycle sequence | Core | C3 invariant | Reference-backed traces and compatibility fixtures improve timing without breaking M2. |
| 14 | **TODO — not started** | `bplus-reference-fixture-set` | Core/cross-strand | Target profile | Primary references, lawful fixtures and explicit processor, memory, display, firmware and controller claims bound the first B+ implementation. |
| 15 | **TODO — not started** | Model B+ 64K profile sequence | Core/cross-strand | Ratified B+ reference/fixture set, C3, relevant C4 foundation | Profile selection, processor, memory/display, firmware boot and selected storage behavior satisfy the B+ portions of M3. |
| 16 | **TODO — not started** | `machine-disk-workflow` | Product/cross-strand | Selected C5 disc slices | User-owned disc media can be mounted, diagnosed and explicitly exported without silent source mutation. |
| 17 | **TODO — not started** | `machine-tape-file-workflow` | Product/cross-strand | Selected C5 cassette/file slices | User-owned tape/file media can be opened, diagnosed and explicitly exported without silent source mutation. |
| 18 | **TODO — not started** | C6 inspection/editor bridge sequence | Core | C1, C3 | Stable inspection, breakpoint/watchpoint control, atomic transactions and BASIC boundaries supply all M3 bridge contracts. |
| 19 | **TODO — not started** | `machine-inspector` | Product/cross-strand | C6 inspection and control contracts | A read-only inspector and bounded breakpoint/watchpoint controls expose machine state without racing execution. |
| 20 | **TODO — not started** | `machine-c6-bridge-validation` | Product/cross-strand | C6 transaction and BASIC-boundary contracts, machine inspector | Product demonstrations prove validated atomic memory transactions and BASIC program-boundary round trips. |
| 21 | **TODO — not started** | BASIC transformation and editing sequence | Product | C6 transaction/program boundaries | Tokenization, labels, inject/retrieve and conflict handling progress toward the full Editor vision through separately named child specifications. |
| 22 | **TODO — not started** | `product-release-readiness` | Product/cross-strand | Selected maintained gates | Device-matrix, accessibility, onboarding, privacy, legal and beta evidence support an explicit external-release decision. |

## Dependency view

```mermaid
flowchart LR
    C2["C2 bounded output"] --> TP["Machine target profile"]
    TP --> H1["Firmware, runtime, audio and input slices"]
    H1 --> M1["M1 Running Model B Machine"]
    TP --> C3["C3 versioned snapshots"]
    C3["C3 versioned snapshots"] --> S["Machine session lifecycle"]
    M1 --> S
    S --> M2["M2 Continuity-complete Machine"]
    TP --> BR["B+ references and fixtures"]
    BR --> BP["Model B+ 64K profile"]
    C4["C4 timing foundation"] --> BP["Model B+ 64K profile"]
    C3 --> BP
    C5["Selected C5 media evidence"] --> M3["M3 Post-C6 Model B+ Preview"]
    C6["C6 inspection/editor bridge"] --> M3
    BP --> M3
    M2 --> M3
```

## Evidence and scope rules

- Product tests prove user journeys; core tests prove deterministic machine
  behavior; cross-strand tests prove the boundary between them.
- Each timing, compatibility, latency or frame-pacing claim defines its fixture,
  host/toolchain, observation interval and tolerance in the selected feature
  specification. Archived targets such as fixed `±0.1 FPS` or `<10 ms` audio
  latency are hypotheses until current research justifies them.
- Proprietary firmware and user media remain outside the repository. Automated
  evidence uses lawful fixtures; optional private compatibility runs may record
  hashes and results without copying the inputs.
- Accessibility is part of every user-facing slice, including keyboard-only
  operation, VoiceOver, focus recovery, Dynamic Type where applicable, and a
  discoverable full-screen exit.
- `docs/STATUS.md` remains the authority for verified hardware fidelity. This
  plan and the application may expose progress, but neither may convert planned
  behavior into a completion claim.
