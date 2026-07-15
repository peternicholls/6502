---
title: Product Brief Bbc Model B 2025 11 03
status: DRAFT
type: Guide
version: 1.0.0
owner: TBD
date-created: '2025-11-05'
date-updated: '2025-11-05'
description: 'TODO: Add description'
---

# Product Brief: BBC Model B
# Product Brief: BBC Model B

**Date:** 2025-11-03  
**Author:** BMad  
**Status:** Draft for PM Review

---

## Executive Summary

BBC Model B is a faithful, legally-compliant BBC Micro emulator for iOS and macOS that delivers an authentic 8-bit computing experience with modern conveniences. The app solves the problem of retro computing accessibility by providing cycle-accurate BBC Model B emulation with a native code editor, real tape I/O (including live microphone capture), and DFS disk support—all while meeting Apple App Store requirements and avoiding legal complications through optional clean-room firmware and user-provided content.

**Target Market:** Retro computing enthusiasts, BASIC programmers, educators, and hobbyists who own BBC Micro software/games  
**Key Value Proposition:** Authentic Beeb experience meets modern UX—type BASIC naturally, load from real audio tapes via microphone, edit code without line numbers

---

## Problem Statement

Retro computing enthusiasts and BBC Micro fans face significant barriers accessing authentic 8-bit computing experiences:

- **Hardware Scarcity:** Original BBC Micro hardware is aging (40+ years old), expensive, and requires maintenance
- **Software Preservation:** Many users have cassette tapes and disk images but no way to run them on modern devices
- **Development Friction:** Authentic BBC BASIC requires line numbers and lacks modern editing conveniences, slowing creative iteration
- **Legal Barriers:** Existing emulators often bundle copyrighted ROMs or violate App Store policies with JIT compilation
- **Platform Limitations:** Most emulators are desktop-only, missing opportunities for iPad/iPhone convenience

**Impact:** Users cannot access their own software collections, experience educational computing history, or develop new BBC BASIC programs without workarounds or hardware investment of £200-500+.

---

## Proposed Solution

A universal iOS/macOS application providing:

- Cycle-accurate BBC Model B emulation passing compatibility suites for popular games/demos
- Modern code editor with label support (no required line numbers), auto-tokenization, and round-trip detokenization from RAM
- Real tape I/O from UEF/WAV files AND live microphone capture with DSP pipeline (DC removal, band-pass filtering, AGC, block framing)
- DFS disk support (.ssd/.dsd with catalog/boot/write-protect)
- Clean-room firmware option (OpenMOS + BASIC II-compatible) OR user-provided authentic ROMs
- App Store compliance (no JIT, no downloaded native code, user-imports only)

**Differentiators:**

- Only emulator with live microphone tape loading on iOS/macOS
- Native modern editor with seamless round-trip to/from tokenized RAM
- Legal clarity through clean-room default + user-ROM option
- First-class SwiftUI/Metal implementation optimized for Apple platforms
- Universal app (iPhone/iPad/macOS) with platform-appropriate UX

---

## Target Users

### Primary User Segment

**Creator Profile:**

- **Demographics:** Ages 35-60, technical background (software developers, engineers, educators)
- **Current Problem-Solving:** Using desktop emulators (BeebEm, JSBeeb) or JavaScript environments, dealing with line number friction and limited mobile access
- **Pain Points:** Can't code on iPad/iPhone during commute, line numbers slow iteration, no seamless modern editor integration
- **Goals:** Write BASIC programs efficiently with modern tooling, test on authentic emulation, share creations

**Secondary Characteristics:**

- Nostalgic for BBC Micro era (1980s school computing)
- Values authenticity and accuracy over shortcuts
- Willing to import own ROMs/software for authentic experience

### Secondary User Segment

**Player/Hobbyist Profile:**

- **Demographics:** Ages 30-65, retro gaming enthusiasts, collectors with software libraries
- **Current Problem-Solving:** Maintaining original hardware, using desktop emulators, or unable to access own tape/disk collections
- **Pain Points:** Original hardware unreliable, desktop emulators lack portability, no way to load from physical tapes on modern devices
- **Goals:** Play owned BBC Micro games/software on modern devices, preserve personal software collections, experience computing history

### Tertiary Segment

- Educators using BBC BASIC for teaching programming concepts
- Interested in internals (CPU/VIA/CRTC inspection), debugging, timing accuracy

---

## Goals and Success Metrics

### Business Objectives

- Launch v1.0 on App Store by M5 completion (~20-24 weeks, accelerated by existing 6502 foundation)
- User Acquisition: 5,000+ downloads in first 6 months
- App Store Rating: Maintain 4.5+ stars with "faithful emulation" feedback
- Compliance: Zero App Store rejections/removals (pass legal/technical review first submission)
- Monetization (optional): Establish freemium model potential (e.g., free core emulation, paid inspector/advanced features)

### User Success Metrics

- **Successful Tape Loading:** Users successfully load programs from UEF/WAV/microphone 90%+ success rate (based on feedback)
- **Editor Adoption:** 60%+ of active users engage with native editor (not just Machine mode)
- **Session Duration:** Average 15+ minutes per session (indicates engaging experience)
- **Return Frequency:** 40%+ weekly active users (DAU/MAU ratio)
- **Round-trip Success:** Zero data loss on editor round-trip (tokenize→inject→detokenize)

### Key Performance Indicators (KPIs)

| KPI                      | Target                                      | Measurement            |
|--------------------------|---------------------------------------------|------------------------|
| Compatibility Pass Rate  | 95%+ target games/demos run correctly       | Curated test suite     |
| Frame Rate Stability     | 50 Hz ±0.1 FPS, zero audio underruns        | Performance telemetry  |
| Editor Data Integrity    | 100% round-trip accuracy                    | Automated tests        |
| Time to First Run        | <5 minutes from install to "Hello, World!"  | User analytics         |
| Microphone Tape Success  | 85%+ successful loads from real tape audio  | User feedback/logs     |

---

## Strategic Alignment and Financial Impact

### Financial Impact

**Development Investment:**

- Estimated 20-24 weeks development @ 1 FTE (M0-M6, accelerated by existing 6502 CPU core)
- M0 reduction: 2-4 weeks saved by leveraging forked 6502Emulator codebase
- Additional 4-8 weeks for clean-room BASIC/MOS (optional, ongoing)

**Revenue Potential:**

- Freemium model: Free core emulator, $4.99-9.99 "Pro" (Inspector, advanced features, CRT shaders)
- Educational licensing: $49-99/year site licenses for schools
- Conservative estimate: 5,000 users × 20% conversion × $6.99 = ~$7,000 revenue Year 1

**Cost Savings/Intangible Value:**

- Personal project/portfolio showcase demonstrating: low-level emulation, Apple platform expertise, SwiftUI/Metal proficiency
- Fills market gap (no quality BBC Micro emulator on iOS currently)
- Preservation value: enables users to access own historical software

### Company Objectives Alignment

**If Internal Project:**

- Demonstrates technical excellence in system-level programming
- Showcases Apple platform capabilities (Metal, AVAudioEngine, SwiftUI)
- Educational value for technical demonstrations

**If Portfolio/Side Project:**

- Career advancement: demonstrates rare low-level + modern UI skills
- Community contribution to computing preservation

### Strategic Initiatives

- App Store Excellence: Demonstrates ability to navigate complex compliance (emulation, microphone, legal IP)
- Cross-Platform Mastery: Universal iOS/macOS implementation with platform-appropriate UX
- Preservation Mission: Contributes to computing history preservation through legal, accessible emulation

---

## MVP Scope

### Core Features (Must Have)

**M0-M1: Machine Mode Foundation**

- Cycle-accurate 6502 CPU: Adapt existing forked codebase (davepoo's 6502Emulator)
  - Complete interrupt handling (IRQ/NMI)
  - Add debugging hooks and bounds checking
  - Integrate with scheduler-based timing
  - Extend unit tests for BBC-specific requirements
- MC6845 CRTC + ULA video emulation (8 modes, 50 Hz stable)
- SN76489 PSG audio output via AVAudioEngine
- System/User VIA 6522 with keyboard matrix
- OpenMOS stub (reset vectors, OSBYTE/OSWORD/OSWRCH)
- SwiftUI Machine mode with status HUD
- Auto-state save/restore

**M2: Tape Deck**

- UEF/WAV file loading with transport controls
- Live microphone capture with DSP pipeline (DC removal, band-pass, AGC, symbol decode, block framing)
- Waveform visualization with block markers
- Motor control obeying MOS commands

**M3: DFS Disks**

- 8271 FDC emulation with .ssd/.dsd support
- Catalog/boot/write-protect functionality
- Shift-boot into disk images

**M4: Native Editor**

- Modern text editor (no required line numbers, label support)
- Tokenizer with label resolver (GOTO/GOSUB/ON...GOTO)
- Detokenizer with label restoration
- Inject compiled program into running machine
- Diff view showing RAM vs source changes

**M5: iOS/macOS Polish**

- iOS layouts with on-screen controls
- Document picker integration (Files app)
- Onboarding/help screens
- Accessibility compliance (VoiceOver labels)
- Localization infrastructure (en-GB base)

**Compliance (across all milestones):**

- No bundled third-party ROMs/games
- Clear "Notes for Reviewers" documentation
- Microphone permission prompts with purpose
- No JIT/dynarec (interpreted emulation only)

### Out of Scope for MVP

**Hardware Extensions:**

- Econet networking
- Co-processor Tube (80186/32016/ARM)
- Second processor support

**Advanced Disk:**

- ADFS/1770 FDC (defer to v1.1)
- .dsd double-sided disk advanced features

**Advanced Features:**

- Inspector panes (CPU/VIA/CRTC registers, disassembly, breakpoints) - macOS only, post-MVP
- CRT shader effects
- Run-ahead latency reduction
- Keymap presets beyond default
- Project templates (graphics/sound/adventures)

**Platform Extensions:**

- Bluetooth keyboard mapping customization (basic only for MVP)

### MVP Success Criteria

**Technical:**

- [ ]  Passes curated compatibility suite (10+ popular titles)
- [ ]  50 Hz frame rate stable for 10-minute continuous run
- [ ]  Audio buffer underruns <0.1% of frames
- [ ]  Tape loading from microphone succeeds 85%+ on test recordings

**User Experience:**

- [ ]  User can boot to prompt, type BASIC, see output within 2 minutes of first launch
- [ ]  User can load program from UEF file successfully
- [ ]  User can edit BASIC source with labels, compile, run without data loss
- [ ]  User can mount .ssd disk and *CAT catalog

**Compliance:**

- [ ]  App Store submission approved first attempt
- [ ]  Zero legal/IP concerns from review
- [ ]  All privacy prompts present and functional

---

## Post-MVP Vision

### Phase 2 Features (v1.1-1.2)

**Enhanced Hardware:**

- ADFS support with WD1770 FDC
- Joystick support (analog/digital)
- User port I/O emulation

**Advanced Editor:**

- Syntax highlighting themes
- Find/replace with regex
- Multi-cursor editing
- Command palette
- Code folding

**macOS Inspector:**

- Real-time CPU register view
- Memory editor with hex/ASCII
- Disassembly window with breakpoints
- VIA/CRTC/PSG register monitoring
- Raster beam position viewer

**Visual Enhancements:**

- CRT shader toggle (scanlines, phosphor glow)
- Integer scaling options
- Palette customization

### Long-term Vision (1-2 years)

**Content Ecosystem:**

- Curated gallery of public-domain programs (with permissions)
- User-submitted creations gallery
- Project templates (graphics, sound, text adventures)
- Tutorial series integrated into app

**Advanced Features:**

- BBC Master 128 emulation mode
- Network play via iCloud (multiplayer games)
- Time-travel debugging (rewind/replay)
- Scripting API for automation

**Platform Expansion:**

- macOS widgets for quick ROM/tape launch

### Expansion Opportunities

- Educational Market: Partner with schools teaching retro computing/computer science history
- Preservation Partnerships: Collaborate with organizations like Centre for Computing History
- Content Creator Tools: Export recorded sessions as video tutorials
- Developer Community: API for third-party tools (assemblers, graphics editors) to inject into running machine

---

## Technical Considerations

### Platform Requirements

**Platforms:**

- iOS 17+ (iPhone/iPad)
- iPadOS 17+ (optimized layouts)
- macOS 14+ (Sonoma+)

**Performance:**

- Target: A-series iPhone 12+ / M-series Mac
- Minimum: A13 Bionic / Intel Mac with Metal support

**APIs:**

- Metal for GPU-accelerated rendering
- AVAudioEngine for audio I/O (output + microphone input)
- SwiftUI for all UI (cross-platform)
- DocumentPicker for Files integration

**Standards:**

- WCAG 2.1 AA accessibility (VoiceOver, Dynamic Type)
- Localization: i18n infrastructure (base: en-GB)

### Technology Preferences

**Core Emulation:**

- C++ for performance-critical emulation (CPU, video, audio, I/O)
- C API boundary for Swift bridging
- Static library compiled into app bundle

**User Interface:**

- SwiftUI for all UI (universal codebase)
- Metal shaders for video rendering
- Combine for reactive state management

**Build System:**

- Xcode with Swift Package Manager
- Optional: CMake for Core library unit testing
- fastlane for CI/CD automation

**Testing:**

- XCTest for Swift unit/integration tests
- C++ unit tests for core emulation
- Snapshot testing for SwiftUI views

### Architecture Considerations

**Core Architecture:**

```
beeb/
  Core/ (C++ static lib - portable)
    cpu_6502/ (FORKED: davepoo's 6502Emulator - needs adaptation)
    crtc_6845/, ula_video/, saa5050_teletext/
    via_6522/, sn76489/, fdc_8271/
    tape/, mos_shims/, rom_mapper/
    scheduler/, core_api.h (C bridging)
  
  AppleApp/
    SharedUI/ (SwiftUI cross-platform)
    iOSApp/ (MTKView, AVAudioEngine, pickers)
    macOSApp/ (Menus, Inspector, macOS-specific)
```

**Display Module Architecture (Epic 2 — RFC Approved 2025-11-05):**

- **CRTC6845**: Pure timing model (registers R0–R17, counters, sync signals); frame-boundary register handling
- **RasterBuilder**: Pixel rendering (Modes 0–7) with bulk memory reads; ULA address decoding; delegates to SAA5050 for Mode 7
- **HostPresenter**: Metal texture upload; triple-buffer with `OSAllocatedUnfairLock`; timestamp-based frame repeat (50→60/120 Hz)
- **Performance**: <2ms frame build (Mode 0), <1ms upload, <0.5ms present jitter on M1+
- **Testing**: 3-layer strategy (unit, integration, golden frames with BeebEm CRC validation)
- **See:** `docs/6845-display-module-rfc-discussion.md`, `docs/architecture.md` ADR-009

**Existing Foundation:**

- Fork of davepoo's 6502Emulator (https://github.com/peternicholls/6502-BBC-Micro-Emulator)
- All legal 6502 opcodes implemented (except decimal mode - not needed for BBC Micro)
- CMake build system in place
- Unit tests exist (6502Test framework)
- Known issues documented: interrupts incomplete, needs debugging hooks, bounds checking
- Last updated November 2020 - requires modernization and BBC-specific adaptations

**Key Design Decisions:**

- C API boundary ensures core can be tested independently
- Scheduler-based timing (CPU cycles drive devices) - needs integration with existing CPU
- Scanline-accurate video (not pixel-perfect unless needed)
- Deterministic state for save/restore
- Adapt existing 6502 core rather than build from scratch

**Integration Points:**

- Metal texture upload from emulated video RAM
- AVAudioEngine callback driven by PSG output
- Microphone buffer fed to tape DSP pipeline
- Document storage in app sandbox + iCloud

---

## Constraints and Assumptions

### Constraints

**Technical:**

- No JIT compilation (App Store policy) - interpreter only
- No downloaded native code - all emulation compiled into app
- Sandboxed storage - Files integration for user content
- Audio latency >10ms typical on iOS (hardware limitation)

**Legal:**

- Cannot bundle Acorn BBC Micro ROMs (copyright)
- Cannot bundle third-party games/software
- Must avoid "BBC Micro" trademark in app name/marketing

**Resources:**

- Solo developer timeline: 24-26 weeks (M0-M5)
- Budget: Personal project (no funding constraints, but limited time)
- Team: 1 FTE (cross-discipline: emulation + UI + compliance)

**Platform:**

- Must pass App Store review (technical + content policies)
- iOS performance varies (A13+ required for stable 50 Hz)

### Key Assumptions

**User Behavior:**

- Users own BBC Micro ROMs legally (personal archives, open-source alternatives)
- Users willing to import ROMs/software via Files (not in-app downloads)
- Microphone tape loading: users have quiet environment for 85%+ success

**Technical Feasibility:**

- Cycle-accurate 6502 at 2 MHz achievable on A13+ without JIT
- Existing forked 6502 codebase adaptable to BBC Micro requirements
- Forked codebase quality sufficient (requires completion, not rewrite)
- CMake build system portable to Xcode/SPM workflow
- AVAudioEngine microphone latency acceptable for tape DSP
- Metal can handle 50 Hz video updates without frame drops

**Market:**

- Sufficient BBC Micro enthusiast demand (5,000+ potential users)
- Willingness to pay $5-10 for Pro features (20% conversion)
- App Store policies remain stable (no emulator crackdown)

**Legal:**

- OpenMOS/BASIC II clean-room approach legally sound
- User-provided ROM loading does not violate App Store policies
- "Emulator" category acceptable if no bundled content

---

## Risks and Open Questions

### Key Risks

| Risk                            | Impact                         | Likelihood  | Mitigation                          |
|---------------------------------|--------------------------------|-------------|-------------------------------------|
| App Store Rejection             | High (blocks launch)          | Medium      | Clear reviewer notes, no bundled ROMs, compliance script, test with similar emulator precedents |
| Forked Codebase Integration     | Medium (M0 delays)            | Medium      | Audit existing code quality, complete interrupt handling, modernize build system, extend tests |
| Timing Accuracy Issues          | High (compatibility)          | Medium      | Golden test suite, per-title probes, scanline scheduler, phased compatibility testing |
| Microphone Tape Unreliable      | Medium (UX frustration)       | High        | Robust DSP with tolerances, live monitoring/guidance, fallback to file loading, user education |
| Performance on Older Devices    | Medium (limited market)       | Medium      | Require A13+, profile early, optimize hot paths, reduce accuracy modes if needed |
| Legacy Code Technical Debt      | Low (maintainability)         | Medium      | Refactor as needed, document adaptations, add comprehensive tests for BBC-specific behavior |
| Clean-Room BASIC Complexity     | Low (defer to Authentic mode) | High        | Start with OpenMOS stubs only, defer full BASIC II to post-MVP or optional |
| Legal Challenge (Trademark/IP)  | High (shutdown risk)          | Low         | Avoid BBC Micro trademark, clear disclaimers, clean-room default, user-provided content only |

### Open Questions

- Forked Codebase Strategy: Complete adaptation of davepoo's 6502Emulator vs. selective integration?
  - Recommendation: Full integration - code quality appears solid, unit tests valuable, CMake portable
  - Action: M0 kickoff with codebase audit, identify gaps (interrupts, debugging hooks, scheduler integration)
- Clean-Room BASIC Priority: Should we build BASIC II-compatible from scratch (12+ weeks) or focus on OpenMOS stubs + user-provided BASIC ROM for v1?
  - Recommendation: Defer clean-room BASIC to v1.1; OpenMOS + user ROM sufficient for MVP
- Minimum CRTC Accuracy: How cycle-perfect must CRTC be for target titles? Mode 7 teletext requires specific timing.
  - Action: Define curated compatibility suite early (M0), establish accuracy baseline
- ADFS in v1 or v1.1? DFS covers most use cases, but ADFS needed for some software.
  - Recommendation: DFS only for v1 (8271), add ADFS (1770) in v1.1
- Monetization Model: Freemium (free emulator + paid Pro) or paid upfront ($4.99)?
  - Action: Research market (JSBeeb free, desktop emulators free/donationware), lean freemium
- Inspector Priority: macOS Inspector valuable for developers but niche. MVP or v1.1?
  - Recommendation: Post-MVP (v1.1) - focus M5 on iOS polish and compliance

### Areas Needing Further Research

- App Store Precedents: Review similar emulators (DOSBox, C64, etc.) for approval patterns and notes
- Microphone DSP Tuning: Real-world tape testing with varied audio sources (laptop speaker, actual Beeb tapes)
- User Onboarding: A/B test onboarding flows (quick start vs detailed tutorial)
- Compatibility Suite: Curate 10-15 test titles covering common timing/hardware patterns
- Legal Review: Consult IP attorney on clean-room approach and trademark concerns (optional but recommended)

---

## Appendices

### A. Research Summary

- Comprehensive overall plan (`bbc_model_B_user_overall_plan.md`) provided complete technical specifications, milestone breakdown, persona definitions, and acceptance criteria
- Plan includes 6 milestones (M0-M6), detailed emulation specs, DSP pipeline pseudo-code, API sketches, and compliance strategy

### B. Stakeholder Input

**Primary Stakeholder:** BMad (developer/author)

- Strong technical vision with detailed implementation plan
- Prioritizes authenticity and compliance
- Phased approach allows iterative validation

**Target User Input:**

- Personas defined: Creator (modern editor focus), Player (zero-friction UX), Hobbyist (inspector interest)
- Pain points identified through implied user research (hardware scarcity, development friction, legal barriers)

### C. References

- **Technical Documentation:**
  - Overall Plan: `/Users/peternicholls/Dev/BBC Model B/docs/bbc_model_B_user_overall_plan.md`
  - Forked 6502 Codebase: [GitHub Repository](https://github.com/peternicholls/6502-BBC-Micro-Emulator) (davepoo's 6502Emulator)

- **External Resources:**
  - BBC Micro hardware specifications (6502, 6845, 6522, SN76489, 8271)
  - App Store Review Guidelines (emulation, microphone permissions, dynamic code)
  - UEF tape format specification
  - DFS disk format documentation

---

_This Product Brief serves as the foundational input for Product Requirements Document (PRD) creation._

_Next Steps: Handoff to Product Manager for PRD development using the `workflow prd` command._