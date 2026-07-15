# BBC Model B Product Requirements Document (PRD)

**Author:** BMad  
**Date:** 2025-11-03  
**Project Level:** 4  
**Target Scale:** Comprehensive (Greenfield)

---

## Goals and Background Context

### Goals

1. Faithful 8-bit BBC Model B emulation (cycle-accurate enough to pass popular games/demos).
2. Tape I/O from files and mic line-in; DFS disk support (.ssd/.dsd; later ADFS).
3. Native editor with labels (no required line numbers), round-trip to/from RAM.
4. Clean-room firmware option (OpenMOS + BASIC II-compatible) **or** user-provided ROMs.
5. First-class UX on iPad, iPhone, and macOS.

### Background Context

The BBC Model B emulator project aims to deliver an authentic 8-bit computing experience with modern conveniences. It addresses the challenges of hardware scarcity, software preservation, and development friction while ensuring compliance with App Store policies. The project targets retro computing enthusiasts, BASIC programmers, and educators, providing a cycle-accurate emulator with advanced features like real tape I/O and a modern code editor.

---

## Requirements

### Functional Requirements

- FR001: Provide cycle-accurate 6502 CPU emulation.
- FR002: Support MC6845 CRTC video modes (0–7) with scanline accuracy and correct Mode 7 (Teletext) rendering.
- FR003: Implement SN76489 PSG audio output with timing fidelity.
- FR004: Enable tape I/O from UEF/WAV files and live microphone capture.
- FR005: Support DFS disk operations (.ssd/.dsd) with catalog and write-protect functionality.
- FR006: Include a native text editor with label support and round-trip tokenization.
- FR007: Offer clean-room firmware (OpenMOS + BASIC II-compatible) or user-provided ROMs.
- FR008: Ensure App Store compliance (no JIT, no downloaded native code).
 - FR009: Provide opt-in full-screen mode with clear entry and an easy, platform-appropriate way to exit (visible control; Escape/Cmd+Ctrl+F on macOS; tap-to-reveal exit on iOS).
 - FR010: Deliver authentic keyboard behavior across iOS/macOS: hardware keyboard support, BBC key mapping, on-screen BBC key overlay on iOS, discoverable “BBC Key Help,” and an Input Capture toggle for uninterrupted play.
 - FR011 (Optional/Post-MVP): Provide a Graphical BBC Keyboard option with authentic visual layout, haptic and audible key-click feedback (respecting Silent Mode/Reduce Motion), full accessibility support, and user-selectable fallback to Overlay.
- FR009: Provide opt-in full-screen mode with clear entry and an easy, platform-appropriate way to exit (visible control; Escape/Cmd+Ctrl+F on macOS; tap-to-reveal exit on iOS).

### Non-Functional Requirements

- NFR001: Maintain 50 Hz video frame rate with ±0.1 FPS stability.
- NFR002: Achieve low-latency audio output (<10 ms typical).
- NFR003: Ensure accessibility compliance (VoiceOver, large text scaling).
 - NFR004: Maintain fixed 4:3 display aspect with integer scaling where possible; do not apply CRT artifact shaders; preserve pixel accuracy (nearest-neighbor when non-integer scaling is unavoidable).

---

## User Journeys

- **Primary Use Case**: A user powers on the emulator, types a BASIC program in the editor, and runs it on the virtual machine.
- **Secondary Use Case**: A user loads a game from a UEF file and plays it with accurate audio and video.
- **Tertiary Use Case**: A user inspects the internal state of the emulator (CPU registers, memory) using the macOS Inspector.

---

## UX Design Principles

- Prioritize authenticity and accuracy in the emulation experience.
- Ensure a seamless transition between Machine, Media, and Editor modes.
- Favor clarity and approachability—core actions should be obvious without prior emulator expertise.
- Provide clear onboarding and help documentation for new users.

---

## User Interface Design Goals

- Design a responsive UI for iOS and macOS platforms.
- Include intuitive navigation for switching between modes (Machine, Media, Editor, Inspector).
- Implement accessible controls and labels for VoiceOver support.
 - Offer user choice between a simple BBC key overlay and a richer Graphical BBC Keyboard; make defaults sensible and discoverable.

---

## Epic List

1. **Epic 1 (M0): Project Foundation & Core Loop**
   - Goal: Establish repo/CI, core render/audio loop, and C API/HUD scaffolding.
   - Stories: Repo & CI, Metal 50 Hz loop, AVAudioEngine test tone, C API boundary + HUD.

2. **Epic 2: Display Module — 6845/ULA/Metal Pipeline**
   - Goal: Implement 6845 CRTC + ULA video rendering with Metal presentation; authentic BBC display modes (0–7) with correct timing and frame pacing.
   - Stories: CRTC6845 Core (registers/counters), RasterBuilder (Modes 0–6), HostPresenter (Metal/frame repeat), Golden Frame Testing, Mode 7 Renderer (Phase 1.5).
   - **Status:** ✅ RFC Approved (2025-11-05) — All architectural decisions finalized
   - **Technical:** Frame-boundary timing, triple-buffer threading, timestamp-based frame repeat, bulk memory reads, 3-layer testing
   - **See:** `docs/epics.md` Epic 2, `docs/6845-display-module-rfc-discussion.md`

3. **Epic 3 (M1): Machine Mode Foundation**
   - Goal: Bring up CPU, VIA/IRQ, keyboard, audio, and auto-state save.
   - Stories: Adapt forked 6502 core, IRQ/NMI + scheduler, VIA + keyboard matrix, SN76489 audio, auto-state save/restore.

4. **Epic 4 (M2): Tape Deck**
   - Goal: UEF/WAV file loading, live mic capture, waveform/block UI, MOS motor control.
   - Stories: File loading, mic DSP, waveform + markers, motor control.

5. **Epic 5 (M3): DFS Disks**
   - Goal: 8271 FDC with .ssd/.dsd, catalog/boot/write-protect, shift-boot.
   - Stories: 8271 emulation, CAT/boot/write-protect, shift-boot.

6. **Epic 6 (M4): Native Editor v1**
   - Goal: Labels, tokenizer/detokenizer, inject, diff, zero round-trip data loss.
   - Stories: Label resolver, tokenize/detokenize, diff view, inject to running machine.

7. **Epic 7 (M5): iOS/macOS Polish & Compliance**
   - Goal: iOS layouts, Files integration, onboarding, a11y, localization, compliance pass.
   - Stories: Onboarding, accessibility, localization infra, reviewer notes & privacy prompts.

> **Note:** Detailed epic breakdown with full story specifications is available in [epics.md](./epics.md)

---

## Out of Scope

- Econet networking, co-processor Tube, and advanced disk features (ADFS).
- Bundling third-party ROMs or games.
- Support for undocumented hardware quirks beyond compatibility suite.
