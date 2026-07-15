# BBC Model B - Epic Breakdown

**Author:** BMad  
**Date:** 2025-11-03  
**Project Level:** 4  
**Target Scale:** Comprehensive (Greenfield)

---

## Overview

This document provides the detailed epic breakdown for BBC Model B, expanding on the high-level epic list in the [PRD](./PRD.md).

Each epic includes:

- Expanded goal and value proposition
- Complete story breakdown with user stories
- Acceptance criteria for each story
- Story sequencing and dependencies

**Epic Sequencing Principles:**

- Epic 1 establishes foundational infrastructure and initial functionality.
- Subsequent epics build progressively, each delivering significant end-to-end value.
- Stories within epics are vertically sliced and sequentially ordered.
- No forward dependencies - each story builds only on previous work.

---

## Epic 2: Display Module — 6845/ULA/Metal Pipeline

**Goal:** Implement the 6845 CRTC + ULA video rendering pipeline with Metal presentation, delivering authentic BBC Micro display modes (0–7) with correct timing, threading, and frame pacing.

**RFC Status:** ✅ Approved (2025-11-05) — All architectural decisions finalized

**Architecture Reference:** `docs/6845-display-module-rfc-discussion.md`, `docs/6845-display-module-specification-draft.md`

**Design Principles (from RFC consensus):**

1. **Separation of Concerns:**
   - `CRTC6845`: Pure timing + address generation (registers R0–R17, counters, sync signals)
   - `RasterBuilder`: Pixel rendering from ULA/RAM using CRTC signals
   - `HostPresenter`: Metal texture upload + frame repeat on host vsync

2. **Timing Model (Phase 1):**
   - Frame-boundary register handling (writes buffered until frame boundary)
   - Fixed-step: 20ms PAL (50 Hz) per `tickFrame()`
   - 256 visible scanlines (progressive, no interlace)
   - Phase 2: Per-scanline register writes for raster effects

3. **Threading & Synchronization:**
   - Triple-buffer ring (3 textures: write, read, spare)
   - `OSAllocatedUnfairLock` protecting ring indices only
   - Emulation thread: 50 Hz fixed-step
   - Render thread: Metal vsync (60/120 Hz)
   - Frame pacing: Timestamp-based selection, "most recent" policy (default)

4. **Performance Budget:**
   - Build 256-line texture: <2.0ms (Mode 0 worst-case)
   - Upload/blit: <1.0ms typical
   - Present jitter: <0.5ms at 120 Hz
   - Target: M1+ Apple Silicon, SIMD optimization for palette lookup

5. **Testing Strategy (3 layers):**
   - Layer 1: Unit tests (CRTC isolated, RasterBuilder with mocks)
   - Layer 2: Integration tests (CRTC + RasterBuilder, full-frame CRC)
   - Layer 3: Golden frames (BeebEm reference, CRC validation)

6. **Error Handling:**
   - Initialization: `throws` for fatal errors (Metal unavailable, invalid config)
   - Runtime: Fail-safe (clamp invalid registers, reuse previous texture on alloc fail)
   - Diagnostics: Signposts only (`CRTC_Frame`, `Raster_Build`, `Presenter_Upload`)

**Stories:**

1. **Story 2.1: CRTC6845 Core — Register & Counter Logic**
   - As a developer, I want a working 6845 CRTC with register handling and scanline counters, so that I can generate correct timing signals for video rendering.
   - **Acceptance Criteria:**
     1. `CRTC6845` class implements 18 registers (R0–R17) with validation (e.g., R1 > R0 throws `CRTCError`)
     2. `writeRegister()` buffers changes until frame boundary; `readRegister()` returns committed value
     3. `tickFrame()` returns array of 312 `CRTCSignals` (one per scanline) with VBLANK, CBLANK, HBLANK flags
     4. Counter state tracks scanline (0–311), character row, line address per BBC spec
     5. Cursor position calculated from R14/R15; `CRTCSignals.cursorActive` set when rendering cursor
     6. Mode switches (register changes) apply correctly on next frame boundary
     7. Unit tests validate: register read/write cycle, counter wraparound, mode-dependent interpretation
   - **Technical Notes:**
     - Frame-boundary timing per RFC Topic 1
     - `unowned` sink reference (not `weak`) per RFC Topic 8
     - Signals are `struct` (value type) to avoid reference counting per RFC Topic 8
     - OS signposts for frame timing diagnostics
   - **References:** RFC Topic 1 (Timing), Topic 9 (Error Handling), Story 2.1 (docs/stories/2-1-crtc-6845-core.md — extracted concepts)

2. **Story 2.2: RasterBuilder — Modes 0–6 Basic Rendering**
   - As a developer, I want to render BBC Micro Modes 0–6 to Metal textures, so that I can display screen content with correct colors and aspect ratio.
   - **Acceptance Criteria:**
     1. `RasterBuilder` accepts `BeebMemoryAccess` protocol + CRTC signals, renders to 320×256 BGRA texture
     2. Mode 0 (640×256 2-color): Renders test pattern correctly with bulk memory reads
     3. Mode 1 (320×256 4-color) and Mode 2 (160×256 16-color): Correct palette indexing
     4. ULA address decoding: Parse R12/R13 hardware scroll, 32KB wraparound, address lookup per mode
     5. Bulk memory API: `readBytes(addr, count, into: UnsafeMutableBufferPointer)` used for scanline fetches (no per-pixel calls)
     6. Golden frame tests: Mode 0/1/2 CRC match BeebEm reference output
     7. Texture double-buffer (current + next) with swap on frame boundary
   - **Technical Notes:**
     - Bulk reads per RFC Topic 3 (BeebMemoryAccess API)
     - SIMD palette lookup (Accelerate framework) deferred to Phase 2
     - Mode 0 worst-case: 640 pixels/scanline → ~20 bytes bulk read vs 320 individual calls
     - Test with mock memory + CRTC signals (Layer 1 testing)
   - **References:** RFC Topic 3 (Memory API), Topic 6 (Testing), Story 2.2 (docs/stories/2-2-raster-builder-modes-0-6.md — extracted concepts)

3. **Story 2.3: HostPresenter — Metal Integration & Frame Repeat**
   - As a developer, I want Metal texture presentation with correct frame pacing, so that 50 Hz emulation displays smoothly on 60/120 Hz hosts.
   - **Acceptance Criteria:**
     1. `HostPresenter` uploads BGRA texture to Metal and blits to drawable
     2. Triple-buffer ring: 3 textures protected by `OSAllocatedUnfairLock`
     3. Frame selection: "Most recent" policy (selects newest completed emu frame)
     4. Lock contention measured: <0.1ms lock time on M1+
     5. 50→60 Hz cadence: 5 emu frames span 6 host frames (no hardcoded pattern; timestamp-based)
     6. 50→120 Hz cadence: 12:5 mapping verified
     7. Present jitter: <0.5ms measured via signposts
   - **Technical Notes:**
     - Triple-buffer per RFC Topic 2 (Threading)
     - Timestamp comparison per RFC Topic 5 (Frame Pacing)
     - Configurable policy: "most recent" (default) or "closest timestamp" (lower judder, higher latency)
     - No forced sleeps on render thread
   - **References:** RFC Topic 2 (Threading), Topic 5 (Frame Pacing)

4. **Story 2.4: Golden Frame Testing — CRC Validation**
   - As a developer, I want automated golden frame tests, so that I can validate rendering accuracy against reference emulators.
   - **Acceptance Criteria:**
     1. Golden frame suite: Mode 0, 1, 2 test patterns captured from BeebEm with documented CRTC registers
     2. CRC32 computed from rendered texture matches reference
     3. Integration test: CRTC + RasterBuilder full-frame render, verify CRC
     4. Edge cases tested: palette wraparound, unusual CRTC register combinations
     5. Swift Testing framework used (`@Test` attributes)
   - **Technical Notes:**
     - Layer 3 testing per RFC Topic 6
     - BeebEm reference images with identical CRTC settings
     - Perceptual hash alternative if minor dither differences acceptable
   - **References:** RFC Topic 6 (Testing Strategy)

5. **Story 2.5: Mode 7 Renderer — SAA5050 Basic (Phase 1.5)**
   - As a user, I want Mode 7 (Teletext) rendering, so that I can run titles requiring Teletext display.
   - **Acceptance Criteria:**
     1. `SAA5050Renderer` module implements basic character mode (12×10 pixel glyphs)
     2. Character ROM loaded (not in main RAM); 25 rows × 40 columns at 0x7C00–0x7FFF
     3. `TeletextRenderer` protocol: `renderScanline(y: Int, chars: [UInt8]) -> [UInt32]`
     4. `RasterBuilder` delegates to SAA5050 when mode == .mode7
     5. Foreground/background colors rendered; flashing/graphics modes deferred to Phase 2
     6. Golden frame test: Mode 7 test card CRC validated
   - **Technical Notes:**
     - Separate module per RFC Topic 4 (Mode 7 Integration)
     - ~200 lines of code for basic character mode
     - Full SAA5050 emulation (doubleheight, graphics, etc.) deferred to Phase 2
     - Approved for Phase 1.5 (post-MVP, pre-Polish)
   - **References:** RFC Topic 4 (Mode 7), RFC Topic 10 (Roadmap), Approved Decision #1

**Sequencing:** Stories 2.1 → 2.2 → 2.3 → 2.4 (parallel with 2.3) → 2.5 (Phase 1.5)

**Estimated Effort:** 8-12 developer-days (per RFC)

**Phase 2 Enhancements (deferred):**
- Per-scanline register writes (mid-frame effects)
- SIMD palette optimization (Accelerate/NEON)
- Full SAA5050 emulation (graphics mode, doubleheight)
- 120 Hz tuning + present-age metrics
- Interlace toggle (R8 support)

**Phase 3 (deferred):**
- NTSC support (60 Hz, 200 visible lines) — approved for deferral; may not be needed
- Raster effects (per-scanline callbacks)
- Dirty-rect optimization
- Video export (offscreen rendering)

---

## Epic 1 (M0): Project Foundation & Core Loop

**Goal:** Establish project infrastructure, core render/audio loop, and baseline scaffolding.

**Stories:**
1. **Story 1.1: Repository Setup**
   - As a developer, I want a version-controlled repository with CI/CD pipelines, so that I can ensure code quality and automated builds.
   - **Acceptance Criteria:**
     1. Repository initialized with README and license.
     2. CI/CD pipeline configured for macOS and iOS builds with smoke tests.
     3. Swift Testing framework integrated (per ADR-008).

2. **Story 1.2: Core Loop (Metal + AVAudioEngine)**
   - As a developer, I want a basic emulator loop (render + audio), so that I can verify the core pipelines.
   - **Acceptance Criteria:**
  1. Metal-backed view renders a test grid at 50 Hz ±0.1 for 10 continuous minutes.
  2. AVAudioEngine outputs a test tone without underruns (<0.1% frames) on target hardware.

3. **Story 1.3: C API Boundary & HUD Stub**
   - As a developer, I want a C API boundary scaffold and status HUD, so that the Core can be bridged to Swift cleanly and app state is visible.
   - **Acceptance Criteria:**
  1. `core_api.h` created with create/reset/tick hooks compiled in the app.
  2. Swift layer calls C API without crashes; status HUD shows FPS and buffer stats.

---

## Epic 3 (M1): Machine Mode Foundation

**Goal:** Bring up CPU, VIA/IRQ, keyboard, audio, and auto-state save for the Machine mode.

**Stories:**
1. **Story 2.0: ROM Loader Integration**
   - As a developer, I want to implement a ROM loader, so that the emulator can load user-provided or clean-room firmware.
   - **Acceptance Criteria:**
     1. ROM loader supports user-imported ROMs via DocumentPicker.
     2. Clean-room OpenMOS firmware loads by default if no user ROM is provided.
     3. ROM validation ensures compatibility and prevents crashes.

2. **Story 2.1: Adapt Forked 6502 Core (davepoo)**
   - As a developer, I want to adapt the existing forked 6502 core, so that we leverage prior work and accelerate delivery.
   - **Acceptance Criteria:**
  1. Forked core integrated behind C API; all legal opcodes supported (decimal mode not required for BBC).
  2. Bounds checking and debugging hooks added; unit tests compile and run.

3. **Story 2.2: IRQ/NMI & Scheduler Integration**
   - As a developer, I want correct IRQ/NMI handling and scheduler-based timing, so that timing-sensitive software behaves.
   - **Acceptance Criteria:**
  1. IRQ/NMI behavior validated with targeted tests and timing probes.
  2. Deterministic tick scheduler drives devices; 50 Hz frame pacing maintained.

4. **Story 2.3: VIA 6522 + Keyboard Matrix**
   - As a user, I want keypresses to be handled authentically, so that typing behaves like a real Beeb.
   - **Acceptance Criteria:**
  1. VIA timers and keyboard matrix wired; key echo visible at prompt from both physical and overlay input.
  2. Simple banner output via OpenMOS stub.
  3. Key repeat and chord handling verified (e.g., SHIFT+BREAK) via automated tests.

5. **Story 2.4: SN76489 Audio Output**
   - As a user, I want authentic PSG audio, so that sound matches expectations.
   - **Acceptance Criteria:**
  1. PSG registers implemented; test tones play without underruns (<0.1%).
  2. Audio latency within platform norms (<10 ms typical on iOS).

6. **Story 2.5: Auto-state Save/Restore**
   - As a user, I want session continuity, so that I can resume where I left off.
   - **Acceptance Criteria:**
  1. State persists on background/quit and restores on launch.
  2. Deterministic state validated with simple round-trip test.

7. **Story 2.6: Display Mode Fidelity (0–7) & 4:3 Enforcement**
     - As a user, I want authentic BBC display behavior across modes 0–7 in a 4:3 frame, so that visuals match the original hardware.
     - **Acceptance Criteria:**
       1. MachineView maintains 4:3 aspect at all window/screen sizes (no stretch/crop; letter/pillarboxing as needed).
       2. Modes 0–7 render correctly; Mode 7 teletext glyphs and attributes display crisply.
       3. Integer scaling used where possible; where not, nearest-neighbor sampling preserves sharp pixels; no CRT shaders.

  8. **Story 2.7: Full-screen Toggle & Easy Exit**
     - As a user, I want to optionally enter full-screen and exit easily, so that I can focus when I want and never get stuck.
     - **Acceptance Criteria:**
       1. Visible full-screen toggle control is present; entering is explicit and opt-in.
       2. Exiting is always easy: Escape and Cmd+Ctrl+F (macOS), single-tap to reveal controls with an “Exit Full Screen” button (iOS).
       3. One-time, dismissible tip shows how to exit full-screen on first use; VoiceOver labels and keyboard focus cover exit controls.

  9. **Story 2.8: Keyboard Overlay & Help (iOS)**
     - As a user on iPhone/iPad, I need BBC-specific keys on-screen and a way to discover where special keys are, so that I can use the emulator without a hardware keyboard.
     - **Acceptance Criteria:**
       1. KeyboardOverlay provides BBC function row and special keys; adapts between compact (iPhone) and expanded (iPad) variants.
       2. “BBC Key Help” overlay supports search (e.g., “break”) and highlights the mapped control/shortcut.
       3. Accessibility labels and large touch targets; overlay hides automatically when a hardware keyboard is detected.

  10. **Story 2.9: Input Capture & Shortcut Safety (macOS/iOS)**
     - As a user, I can enable Input Capture to prevent OS shortcuts from interfering during play while retaining an obvious escape hatch.
     - **Acceptance Criteria:**
       1. Capture toggle available (prominent in full-screen); indicator shows when active.
       2. Escape (macOS) or long-press Exit control (iOS) releases capture reliably; help tip shown on first activation.
       3. Platform-reserved shortcuts (e.g., Cmd+Q) are never shadowed; compliance note documented.

---

## Epic 4 (M2): Tape Deck

**Goal:** Enable tape I/O from files and live microphone with robust DSP and UI.

**Stories:**
1. **Story 3.1: UEF/WAV File Loading**
   - As a user, I want to load programs from UEF/WAV files, so that I can run my existing software.
   - **Acceptance Criteria:**
  1. UEF/WAV parsed and decoded; checksum verified; program loads successfully.
  2. Time to load within ±10% of real hardware on fixtures.

2. **Story 3.2: Live Microphone Capture**
   - As a user, I want to load programs from real tapes via microphone, so that I can preserve my software collection.
   - **Acceptance Criteria:**
     1. Microphone input processed through DSP pipeline.
  2. Programs loaded successfully with ≥85% reliability on test recordings.

3. **Story 3.3: Waveform Visualization & Block Markers**
   - As a user, I want to see waveform with block markers, so that I can diagnose loading issues.
   - **Acceptance Criteria:**
  1. Waveform view renders with header/data block overlays.
  2. Error badges show checksum failures and guidance.

4. **Story 3.4: Motor Control Obeying MOS**
   - As a developer, I want motor control integrated, so that the deck respects MOS commands.
   - **Acceptance Criteria:**
  1. Motor start/stop follows MOS; UI transport reflects state.
  2. Loading behavior consistent with BBC MOS commands.

---

## Epic 5 (M3): DFS Disks

**Goal:** Support DFS disk operations with 8271 FDC and core UX.

**Stories:**
1. **Story 4.1: 8271 FDC Emulation**
   - As a user, I want DFS disk support, so that I can access my software library.
   - **Acceptance Criteria:**
  1. .ssd/.dsd images mount; sector I/O verified on fixtures.
  2. Compatibility validated on curated titles.

2. **Story 4.2: Catalog/Boot/Write-Protect**
   - As a user, I want to browse and protect disks, so that data is safe and accessible.
   - **Acceptance Criteria:**
  1. *CAT shows directory; write-protect toggle persists.
  2. Shift-boot into disk images works from cold start.

---

## Epic 6 (M4): Native Editor v1

**Goal:** Develop a modern text editor with tokenization and round-trip support.

**Stories:**
1. **Story 5.1: Label Resolver**
   - As a user, I want to use labels in my BASIC programs, so that I can write code without line numbers.
   - **Acceptance Criteria:**
     1. Labels resolved to line numbers during tokenization.
     2. GOTO/GOSUB statements updated with resolved line numbers.

2. **Story 5.2: Tokenization and Detokenization**
   - As a user, I want my programs tokenized and detokenized seamlessly, so that I can edit and run them efficiently.
   - **Acceptance Criteria:**
     1. BASIC programs tokenized per BASIC II table.
     2. RAM detokenized back to source code with labels restored; zero data loss on round-trip.

3. **Story 5.3: Diff View**
   - As a user, I want to see differences between my source code and the running program, so that I can track changes.
   - **Acceptance Criteria:**
     1. Diff view highlights changes between source and RAM.
     2. Changes displayed in a user-friendly format.

---

## Epic 7 (M5): iOS/macOS Polish & Compliance

**Goal:** Finalize UI/UX and ensure App Store compliance.

**Stories:**
1. **Story 6.1: Onboarding Screens**
   - As a new user, I want clear onboarding screens, so that I can understand how to use the app.
   - **Acceptance Criteria:**
     1. Onboarding screens explain key features and workflows.
     2. Screens are accessible and localized.

2. **Story 6.2: Accessibility Features**
   - As a user, I want VoiceOver support and large-text scaling, so that the app is accessible to everyone.
   - **Acceptance Criteria:**
     1. All UI elements labeled for VoiceOver.
     2. Text scaling works across all screens.

3. **Story 6.3: Localization**
   - As a user, I want the app localized to my language, so that I can use it comfortably.
   - **Acceptance Criteria:**
     1. Base localization in en-GB.
     2. Infrastructure ready for additional languages.

4. **Story 6.4: Compliance & App Store Readiness**
   - As a publisher, I want zero App Store violations, so that the app is approved on first attempt.
   - **Acceptance Criteria:**
     1. Reviewer notes prepared; no JIT/dynarec; import-only policy enforced.
     2. Microphone usage strings present; privacy prompts verified; compliance checklist passes.
     3. Display fidelity policy enforced: 4:3 aspect, no CRT artifacts, pixel-accurate rendering; documented in reviewer notes.

5. **Story 6.5: Integration Testing**
   - As a developer, I want end-to-end integration tests, so that I can validate the emulator's functionality across all features.
   - **Acceptance Criteria:**
     1. Integration tests cover tape, disk, editor, and keyboard workflows.
     2. Tests validate performance metrics (50 Hz, <10ms audio latency).
     3. Tests run automatically in CI/CD pipeline (GitHub Actions with platform-specific smoke tests).

---

## Story Guidelines Reference

**Story Format:**

```
**Story [EPIC.N]: [Story Title]**

As a [user type],
I want [goal/desire],
So that [benefit/value].

**Acceptance Criteria:**
1. [Specific testable criterion]
2. [Another specific criterion]
3. [etc.]

**Prerequisites:** [Dependencies on previous stories, if any]
```

**Story Requirements:**

- **Vertical slices** - Complete, testable functionality delivery.
- **Sequential ordering** - Logical progression within epic.
- **No forward dependencies** - Only depend on previous work.
- **AI-agent sized** - Completable in 2-4 hour focused session.
- **Value-focused** - Integrate technical enablers into value-delivering stories.

---

**For implementation:** Use the `create-story` workflow to generate individual story implementation plans from this epic breakdown.
