# Decision Architecture

## Executive Summary

This architecture composes a high-performance C++ BBC Micro core behind a stable C API and a platform‑native Swift/SwiftUI app that renders via Metal at 50 Hz with AVAudioEngine audio. All integration is deterministic and scheduler‑driven, ensuring display fidelity (4:3, modes 0–7 with crisp Mode 7) and low‑latency audio while keeping the UX simple, accessible, and App Store compliant.

**RFC Process:** Significant architectural decisions follow the RFC workflow documented in `docs/rfc-workflow.md`. ADRs below reference approved RFCs archived in `docs/archive/rfcs/`.

## Project Initialization

This project is an Xcode SwiftUI app (iOS/macOS). The emulator core is integrated as a C/C++ static library surfaced through a C API boundary.

Recommended initialization for the core integration (choose one):

- Submodule + Local Build (recommended for active development)
  1. git submodule add https://github.com/peternicholls/6502-BBC-Micro-Emulator External/BeebCore
  2. Initialize nested submodules recursively: `git submodule update --init --recursive`
  3. The C++ emulator core lives at `External/BeebCore/6502/6502/6502Lib` (nested submodule pointing to fork: https://github.com/peternicholls/6502Emulator)
  4. Build static lib with Xcode project in `External/BeebCore/BeebCore.xcodeproj`
  5. Expose `core_api.h` in a bridging header and link the static lib in the app target

- XCFramework + Binary Target (recommended for distribution stability)
  1. Build universal XCFramework for macOS + iOS (device/simulator)
  2. Add SwiftPM binary target in the app project and link
  3. Maintain versioned releases for reproducibility

First implementation stories: create scheduler‑driven render/audio loop, wire the C API, and surface a HUD with FPS/audio stats (Epic 1).

## Decision Summary

| Category | Decision | Version | Affects Epics | Rationale |
| -------- | -------- | ------- | ------------- | --------- |
| Platforms | iOS 17+/iPadOS 17+, macOS 14+ | SDK current | M0–M5 | Target modern OS features; App Store alignment |
| UI | SwiftUI + AppKit/UIKit bridges where needed | Xcode SDK | M0, M2, M4, M5 | Native controls, a11y, multi‑platform |
| Rendering | Metal at 50 Hz; 4:3 aspect; integer scaling; nearest‑neighbor | SDK current | M0–M3, M5 | Accuracy and performance; crisp Mode 7 |
| Audio | AVAudioEngine; low‑latency; underrun <0.1% | SDK current | M0–M3, M5 | Reliability with platform audio stack |
| Core Integration | C++ core behind C API boundary (`core_api.h`) | C++17 | M0–M5 | Clear ABI for Swift interop and testing |
| Timing | Deterministic scheduler drives CPU/devices/frame | N/A | M0–M3 | Stable 50 Hz pacing and device sync |
| File I/O | DocumentPicker; sandboxed; import‑only | N/A | M2–M5 | Compliance; user‑owned media only |
| Tape DSP | UEF/WAV decode + mic capture pipeline | N/A | M2 | Real I/O with observable waveform |
| Disk | 8271 DFS; .ssd/.dsd; write‑protect; shift‑boot | N/A | M3 | Compatibility and safety |
| Editor | Label‑aware tokenizer/detokenizer; diff | N/A | M4 | Zero round‑trip loss; modern UX |
| Keyboard | Authentic matrix mapping; overlay; help; capture | N/A | M1, M5 | Usability without expert knowledge |
| Full‑screen | Opt‑in; easy exit; discoverable tip | N/A | M1, M5 | Prevent user confusion; reviewer‑friendly |
| Logging | os_log unified logging; categories per module | N/A | All | Diagnostics without overhead |
| Errors | Swift `throws`/Result; C error codes; user toasts | N/A | All | Predictable handling; good UX |
| Accessibility | VoiceOver; Dynamic Type; high contrast; a11y labels | WCAG AA | M5 | Inclusive experience and compliance |
| Packaging | No JIT; no bundled ROMs; import‑only | N/A | All | App Store policy compliance |

## Project Structure

```
BBC Model B/                          # Superproject (Swift app)
  BBC_Model_BApp.swift
  ContentView.swift
  BBC_Model_B/
    BBC_Model_BApp.swift
    ContentView.swift
    App/
      UI/
        Machine/
          MachineView.swift
          HUDOverlay.swift
        Media/
          TapeDeckView.swift
          WaveformView.swift
          DiskManagerView.swift
        Editor/
          EditorView.swift
          DiffView.swift
      Services/
        CoreBridge.swift              # Swift wrapper for C API
  External/
    BeebCore/                         # Submodule: C API wrapper repo
      include/
        core_api.h                    # C API boundary (extern "C")
        module.modulemap              # Swift module definition
      src/
        core_api_stub.cpp             # C API implementation (stub)
      6502/                           # Nested submodule: emulator repo (fork)
        6502/
          6502Lib/                    # Actual C++ emulator sources
            src/
              public/m6502.h
              private/m6502.cpp
          6502Test/                   # Emulator unit tests
      BeebCore.xcodeproj              # Static library target
      CONTRIBUTING.md
      .gitignore
  BBC Model B.xcworkspace/
  docs/
        Audio/
          AudioEngine.swift
        Files/
          FileImportService.swift
        Settings/
          SettingsStore.swift
      Platform/
        Keyboard/
          KeyboardOverlay.swift
          KeyHelpOverlay.swift
          GraphicalKeyboardView.swift (optional)
        Fullscreen/
          FullscreenController.swift
    Bridge/
      include/
        core_api.h
      CoreBridge.swift
    Core/ (integration choice)
      External/BeebCore/ (submodule) OR
      BeebCore.xcframework/ (binary)
  docs/
    PRD.md
    epics.md
    ux-design-specification.md
    architecture.md
```

## Epic to Architecture Mapping

| Epic | Architecture Surface |
| ---- | --------------------- |
| M0 Core Loop | MachineView, AudioEngine, CoreBridge, HUDOverlay |
| M1 Machine Mode | CoreBridge, KeyboardOverlay/GraphicalKeyboard, FullscreenController |
| M2 Tape Deck | TapeDeckView, WaveformView, AudioEngine (mic), FileImportService |
| M3 DFS Disks | DiskManagerView, FileImportService, CoreBridge (8271) |
| M4 Editor v1 | EditorView, DiffView, CoreBridge (tokenize/detokenize) |
| M5 Polish & Compliance | SettingsStore, a11y labels, reviewer notes, localization infra |

## Technology Stack Details

### Core Technologies

- App: Swift/SwiftUI, Metal, AVAudioEngine, os_log
- Core: C++17 BBC Micro core exposed via C API (`core_api.h`)
- Build: Xcode project with either submodule (CMake → static lib) or XCFramework binary target
- Testing: Hybrid approach using Swift Testing (Swift/wrapper layer), Google Test (C++ wrapper + core CPU), covering all layers
  - See `docs/testing-strategy.md` for complete testing guidelines

### Integration Points

- CoreBridge.swift wraps `core_api.h` for Swift; provides init/reset/tick, memory/IO hooks
- Scheduler drives `tick()` at deterministic cadence; frame boundary at 50 Hz triggers Metal draw
- AudioEngine feeds PSG samples to AVAudioEngine; back‑pressure metrics surface to HUD
- Tape pipeline: UEF/WAV decode (file) and mic capture (iOS) → core input; WaveformView shows blocks/errors
- Disk pipeline: mount .ssd/.dsd → core 8271 I/O; write‑protect state maintained

## Novel Pattern Designs

- Label‑based BASIC editor with tokenization/detokenization and RAM↔source diff
  - Contract: zero round‑trip loss; token table BASIC II; labels restored on detokenize
  - Diff computes semantic deltas and highlights in EditorView

## Implementation Patterns

These patterns ensure consistent implementation across agents:

- Naming
  - Swift types: UpperCamelCase; files match type name; modules by feature (Machine, Media, Editor)
  - C API: `beeb_` prefix; verbs: `beeb_init`, `beeb_tick`, `beeb_load_tape`
- Code Organization
  - Feature‑first folders (Machine/Media/Editor); shared components under `App/UI` and `Services`
  - Bridge code isolated in `Bridge/`; core headers under `Bridge/include`
- Error Handling
  - Swift: `throws`/Result; present user‑safe messages via Toast/Banner; log diagnostic via os_log
  - C API: integer codes/enums; map to Swift errors centrally
- Logging
  - os_log with categories: core, render, audio, media, disk, editor; levels: debug/info/error
- Date/Time
  - Use monotonic clocks for scheduling; avoid wall‑clock drift; record timestamps for profiling only
- Keyboard
  - Authentic and Convenient presets; remap UI; searchable Key Help; input capture opt‑in; never shadow Cmd+Q
- Full‑screen
  - Opt‑in; visible exit; Escape/Cmd+Ctrl+F on macOS; single‑tap to reveal exit on iOS; one‑time tip

## Consistency Rules

### Naming Conventions

- Swift: `FeatureNameView`, `ServiceName`, test files `TypeNameTests`
- Files: `user-card.swift` style avoided; match type name exactly
- Assets: lower‑kebab for resources; namespace by feature

### Code Organization

- Co‑locate tests with features in `Tests/FeatureName/`
- Shared UI primitives in `App/UI/Shared/`
- No cross‑feature imports except via Services and Bridge

### Error Handling

- Errors visible to users are human‑friendly and actionable; technical details logged only
- All C API calls wrapped with guard; failing calls surface a toast and recover to a safe state

### Logging Strategy

- Structured messages with categories; sampling for verbose logs; disable in release except errors

## Data Architecture

- Minimal app data: preferences (scaling mode, keyboard mode, haptics/audio toggles)
- Emulator state snapshot for auto‑restore (M1): persisted using codable structs + core state blob

## API Contracts

- C API (illustrative):
  - `beeb_init(const struct beeb_config* cfg)` → int
  - `beeb_reset(void)`
  - `beeb_tick(uint32_t cycles)`
  - `beeb_load_tape(const uint8_t* data, size_t len)` → int
  - `beeb_mount_disk(const uint8_t* image, size_t len, int drive)` → int
  - `beeb_read_mem(uint16_t addr)` / `beeb_write_mem(uint16_t addr, uint8_t v)`

## Security Architecture

- Sandboxed app; no code download/JIT; import‑only policy for ROMs/games
- Permissions: microphone usage string; file access via DocumentPicker; no private APIs

## Performance Considerations

- Maintain 50 Hz frame pacing; assert drift and surface in HUD
- Avoid blocking on audio thread; keep buffers filled to prevent underruns (<0.1%)
- Prefer integer scaling; nearest‑neighbor for fractional; keep Mode 7 crisp

## Deployment Architecture

- Universal app with shared code; app targets for iOS and macOS
- No server components; data local to device; reviewer notes document compliance decisions

## Development Environment

### Prerequisites

- Xcode with iOS/macOS SDKs; CMake (if building core locally); git submodules

### Setup Commands

```bash
# (Option A) Add core as submodule
git submodule add https://github.com/peternicholls/6502-BBC-Micro-Emulator External/BeebCore

# (Option B) Use existing XCFramework (drop into Core/)
# ln -s /path/to/BeebCore.xcframework Core/BeebCore.xcframework
```

### Workspace

- A top-level Xcode workspace (`BBC Model B.xcworkspace`) is provided to host both the Swift app project and the C++ core project. See `docs/workspace-migration.md` for setup details and options (subproject vs SPM vs XCFramework).

## Testing Strategy

This project uses a **hybrid testing approach** to validate both the low-level CPU emulation and the high-level application layer:

### Swift Testing (Native Swift Testing)
**Purpose**: Test all Swift code, C API wrapper, and BBC Micro-specific features

**Coverage**:
- Swift wrapper layer (`CoreBridge.swift`)
- C API implementation (`core_api.cpp`) - integration level from Swift
- BBC Micro hardware emulation (keyboard, video, tape, sound)
- SwiftUI components and user interactions
- Platform-specific behavior (iOS vs macOS)

**Location**: `BBC Model BTests/`

**Usage**: Primary testing framework for TDD development
- Run via: Cmd+U in Xcode or `swift test` / `xcodebuild test`
- Integrated with Xcode test navigator
- Modern Swift-native framework with parameterized tests, async/await support, and tags
- Automatic in CI/CD pipelines
- **This is the standard testing framework for this project** - use `import Testing` and `@Test` attributes for all Swift test code

### C++ Unit Testing (Google Test)
**Purpose**: Test C++ wrapper implementation details and internal logic

**Coverage**:
- C API wrapper internal implementation (`core_api.cpp`)
- Memory management and lifecycle
- Error handling edge cases
- Hardware wrapper logic
- Complex scenarios difficult to test from Swift

**Location**: `External/BeebCore/tests/`

**Framework**: Google Test (gtest)
- Consistency with upstream 6502 emulator tests
- Mature, well-documented, industry-standard
- CMake-based build integration
- Rich assertion library and mocking support

**Usage**: White-box testing of C++ layer
- Run via: `cd External/BeebCore/build && make && ./tests/BeebCoreTests`
- CMake configuration similar to 6502 test setup
- Separate from Xcode production build (no gtest in shipped app)
- Optional execution during wrapper development

### Google Test (Upstream 6502 Emulator)
**Purpose**: Validate core 6502 CPU instruction execution

**Coverage**:
- Low-level CPU operations (existing comprehensive suite)
- Register operations, arithmetic, logical operations
- Branch, jump, and stack instructions
- Status flag behavior

**Location**: `External/BeebCore/6502/6502/6502Test/`

**Usage**: Validation when updating emulator core
- Run via: CMake build in `External/BeebCore/6502/build`
- NOT compiled into iOS/macOS app (no gtest dependency)
- Used for emulator regression testing only

**Build Configuration**:
- Xcode build (production): Compiles only library sources (`m6502.cpp`, `main_6502.cpp`) and wrapper (`core_api_stub.cpp`)
- CMake build (optional): Compiles library + test files + gtest for validation

### TDD Workflow

For new BBC Micro features:
1. **Red**: Write Swift test defining expected behavior using `@Test` attribute
2. **Green**: Implement minimal code to pass test (may include C++ wrapper code)
3. **Refactor**: Clean up while keeping tests passing
4. **Optional**: Add C++ unit tests for complex wrapper logic

For C++ wrapper implementation:
1. Write C++ unit test for internal logic using gtest
2. Implement C++ code to pass unit test
3. Verify integration with Swift Testing from consumer perspective

For core emulator changes:
1. Run existing gtest suite for baseline
2. Make changes to emulator sources
3. Re-run gtest to validate
4. Add Swift integration test for wrapper layer

**Complete Details**: See `docs/testing-strategy.md` for comprehensive guidelines, test organization, and BMAD agent instructions.

## Architecture Decision Records (ADRs)

- ADR‑001: Use C API boundary for C++ core to ensure ABI stability and Swift interop
- ADR‑002: Enforce 4:3 aspect, integer scaling, nearest‑neighbor; no CRT shaders (MVP)
- ADR‑003: Scheduler‑driven timing model with 50 Hz frame boundary
- ADR‑004: AVAudioEngine for audio; underrun budget <0.1% with HUD visibility
- ADR‑005: Import‑only policy (no bundled ROMs/games); sandboxed file access
- ADR‑006: Keyboard strategy with overlay/help and optional graphical keyboard; input capture opt‑in
- ADR‑007: Hybrid testing with Swift Testing (native Swift/wrapper) as primary framework, Google Test for both C++ wrapper and upstream CPU validation; TDD via Swift Testing for features, gtest for C++ implementation
- ADR‑008: Swift Testing (`import Testing`) is the standard test framework for all Swift code; XCTest is deprecated for new tests
- ADR‑009: 6845 Display Module architecture: CRTC6845 (timing only) → RasterBuilder (pixels) → HostPresenter (Metal); frame-boundary register handling (Phase 1), triple-buffer with OSAllocatedUnfairLock, timestamp-based frame repeat, bulk memory reads, 3-layer testing (unit/integration/golden frames), signpost-only diagnostics. See: RFC0001 & RFC0002 (pending archival: docs/archive/rfcs/2025/)

---

Generated by BMAD Decision Architecture Workflow v1.0
Date: 2025-11-03
For: BMad
