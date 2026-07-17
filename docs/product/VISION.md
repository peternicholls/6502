# Product vision

**Status:** Canonical  
**Updated:** 2026-07-17

## North star

Beeb6502 is a native Apple retro-computing environment that makes the BBC Model
B family approachable, useful and faithful on modern devices. It starts with
the verified Model B profile and progresses to a planned Model B+ 64K profile
without weakening the original regression baseline. It combines an authentic
machine, preservation-oriented media tools and a modern programming workflow
without hiding the character of the original computer.

The 6502 and hardware emulation are foundational capabilities, not the whole
product. Product decisions should be evaluated by the experience they enable
across Machine, Media and Editor workflows.

## The problem

Original BBC Micro hardware and media are increasingly difficult to maintain.
Existing emulators often prioritize desktop compatibility over mobile use,
modern programming workflows, accessibility or clear legal boundaries. People
who own historical software, want to learn BBC BASIC or want to preserve tapes
and disks need a dependable experience that fits contemporary Apple platforms.

## Who it serves

### Creators

Programmers, educators and learners who want to write BBC BASIC with modern
editing assistance while running the result on authentic emulated hardware.

Their core job is: **write, run, inspect and revise a program without losing
compatibility with a real BBC Micro.**

### Players and collectors

Retro-computing enthusiasts who own ROMs, tapes or disk images and want to use
and preserve them without depending on ageing hardware.

Their core job is: **import my own material, understand its state, and run it
reliably without damaging the source.**

### Explorers

Students, educators and technically curious users interested in how the CPU,
memory, display and peripherals interact.

Their core job is: **observe and understand the machine without first becoming
an emulator expert.**

## Product experiences

### Machine

The primary experience is an immediately recognizable BBC Micro: predictable
boot, accurate display and audio, authentic keyboard behavior and dependable
session continuity. Model identity and known compatibility limits must remain
visible rather than presenting Model B and Model B+ behavior as interchangeable.
The machine view should feel focused rather than like a developer test harness.

Key qualities:

- deterministic, compatibility-led emulation;
- a fixed 4:3 presentation with crisp nearest-neighbor scaling;
- responsive physical and on-screen keyboard input;
- clear run, pause, reset, full-screen and media state;
- save and restore without surprising the user.

### Media

Media handling supports use and preservation of user-owned material. Disk and
tape state must be explicit, recoverable and diagnosable.

Key qualities:

- system file pickers and sandboxed, import-only workflows;
- SSD/DSD disk mounting, write protection and explicit export of changes;
- UEF/WAV tape loading and, later, live microphone capture;
- useful progress, waveform, checksum and recovery feedback;
- no silent modification of the user's source file.

### Editor

The editor provides a modern source experience while preserving BBC BASIC's
runtime representation. Labels and tooling are conveniences layered over an
authentic machine, not a replacement language.

Key qualities:

- label-aware source without mandatory manual line-number management;
- deterministic tokenization and detokenization;
- explicit injection into the running machine;
- a semantic diff between editor source and RAM;
- zero unexplained loss across source-to-machine round trips.

### Platform experience

The application should look and behave like a well-made iPhone, iPad and macOS
product. Native conventions take priority over decorative nostalgia.

Key qualities:

- SwiftUI-first, platform-appropriate layouts and controls;
- VoiceOver, Dynamic Type, contrast and reduced-motion support;
- obvious keyboard mapping help and safe input capture;
- accessible full-screen entry and exit;
- concise onboarding and actionable errors.

## Durable product principles

1. **Authenticity with evidence.** Accuracy claims require compatibility,
   timing or reference evidence. Known fidelity limits remain visible.
2. **Modern assistance, authentic execution.** Editing and media tools reduce
   friction without changing what the emulated machine executes.
3. **User-owned content.** Do not bundle proprietary Acorn firmware, character
   ROMs, games or user media. Imports are explicit and local.
4. **Preservation before convenience.** Never silently overwrite source media;
   expose modified images as explicit exports.
5. **Native and accessible.** Use Apple platform conventions, minimal retro
   accents and semantic accessibility rather than skeuomorphic imitation.
6. **Deterministic core, decoupled host.** Emulation owns machine time. Display,
   audio and UI presentation consume stable outputs without driving hardware
   truth from host refresh timing.
7. **Portable foundation.** The core remains dependency-light, host-agnostic
   and testable outside Apple platforms.
8. **Safe failure.** Invalid content and unsupported behavior produce useful
   diagnostics and recoverable product states, never cross-language crashes.

## Product boundary

The product includes verified BBC Model B emulation, a planned BBC Model B+ 64K
profile, user-owned media workflows, a BBC BASIC-oriented editor and educational
inspection capabilities. It has no server dependency and does not download
executable content.

The following are not current product commitments:

- bundled proprietary firmware or commercial software;
- JIT or dynamically downloaded native code;
- preservation-grade flux emulation and copy-protection support;
- BBC Master, Tube, Econet or other expansion systems;
- the Model B+ 128K expansion until a separate product and compatibility case
  approves it;
- a content marketplace or cloud community;
- simulated CRT artefacts as a default visual treatment.

These may be reconsidered only through an explicit product and legal decision.

## Success outcomes

The product is succeeding when:

- a new user can import suitable firmware, reach the machine and type a simple
  program without external instructions;
- representative software runs with stable 50 Hz machine timing and dependable
  audio presentation;
- disk and tape workflows explain failures and protect original media;
- editor-to-RAM round trips are deterministic and loss is detected rather than
  hidden;
- core workflows are usable with VoiceOver and platform-standard input;
- compatibility and performance regressions are caught automatically;
- App Store review can understand the import-only, no-JIT and legal boundaries.

Exact acquisition, conversion and microphone-success targets remain hypotheses
until representative users and fixtures exist. They should not drive technical
claims before measurement is available.

## Current strategic focus

The next product objective is the M1 Running Model B Machine built on the
existing emulator foundation: firmware onboarding, host frame presentation,
real-time audio, keyboard mapping, machine controls and observable health. C3
then adds continuity, while the post-C6 target extends the same proven workflow
to Model B+ 64K and demonstrates measured Media and Editor progress. The
[delivery plan](MACHINE_DELIVERY_PLAN.md) defines the cross-strand gates.

## Open product decisions

- Whether a redistributable clean-room firmware experience is feasible and
  valuable enough for the first public release.
- Final application name and trademark-safe store presentation.
- Minimum supported Apple OS and hardware versions at release time.
- Whether monetization is appropriate, and which capabilities must always
  remain free.
- The balance between a simple keyboard overlay and an optional graphical
  keyboard on each device class.
- Which primary-reference and compatibility fixture set will ratify the exact
  processor and disc-controller variants for the Model B+ 64K profile.
