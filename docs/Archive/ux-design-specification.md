---
title: Ux Design Specification
status: DRAFT
type: UX
version: 1.0.0
owner: TBD
date-created: '2025-11-05'
date-updated: '2025-11-05'
description: 'TODO: Add description'
---

BBC Model B UX Design Specification

Created on 2025-11-03 by BMad
Generated using BMad Method - Create UX Design Workflow v1.0

---

## Executive Summary

BBC Model B delivers an authentic BBC Micro experience on iOS and macOS with modern conveniences. The UX prioritizes an instantly familiar Machine mode, effortless media loading (UEF/WAV, microphone, DFS disks), and a modern label-based editor that round-trips to the running machine without data loss. The visual language is platform-native and minimal, with tasteful retro accents that evoke the Beeb without sacrificing accessibility or App Store compliance.

---

## 1. Design System Foundation

### 1.1 Design System Choice

- Primary: Apple Human Interface Guidelines (HIG), SwiftUI-first on all platforms
- Typography: SF Pro Text/Display; monospaced for code views (SF Mono)
- Iconography: SF Symbols; custom glyphs only where necessary (e.g., Tape, Disk)
- Theming: System light/dark with a “Retro Accent” palette (phosphor greens/ambers) applied to HUD and waveform highlights
- Motion: Subtle; prefer reduced motion honoring system settings

---

## 2. Core User Experience

### 2.1 Defining Experience

- Machine-first: Boot straight to the Beeb prompt at 50 Hz with a minimal HUD
- Frictionless media: Load from UEF/WAV or microphone with clear transport and waveform feedback
- Modern editing: Label-based editor that tokenizes/detokenizes without line numbers; injects seamlessly into the running machine
- Seamless platform fit: On-screen controls for iOS; menus/shortcuts and Inspector (post-MVP) on macOS
- Clarity-first UI: Clear, approachable controls with obvious navigation; no expert knowledge required to perform core tasks.

### 2.2 Novel UX Patterns

- Live waveform with block markers and error badges for tape loads
- “Round-trip diff” between RAM and source with token-aware highlights
- Shift-boot affordance on disks with clear state and write-protect toggle
- Status HUD with FPS/audio buffer and device indicators; collapsible and non-intrusive

### 2.3 Error Handling and Recovery

- **Error State Visuals:**
  - Use non-blocking toasts for transient errors (e.g., tape load checksum failure).
  - Persistent inline error messages for critical issues (e.g., disk mount failure).
  - Error banners with actionable guidance (e.g., "Retry" or "Cancel").

- **Error Taxonomy:**
  - **Tape Loading:** Checksum failures, unsupported formats, mic input errors.
  - **Disk Operations:** Corrupt images, write-protect conflicts, unsupported formats.
  - **Editor:** Tokenization errors, syntax issues, detokenization mismatches.

- **Recovery Patterns:**
  - Retry options for transient errors.
  - Clear guidance for user actions (e.g., "Check file format" or "Re-import ROM").
  - Fallback to default state where possible (e.g., load OpenMOS firmware if user ROM fails).

---

## 3. Visual Foundation

### 3.1 Color System

- Base: System background (dark preferred) for eye comfort during long sessions
- Retro accents: Phosphor green (#00E676) and amber (#FFB300) used sparingly for HUD, cursor, and markers
- Semantic roles: Success (green), Warning (amber), Error (red), Info (blue) mapped to Apple semantic colors for a11y
- Contrast: Meet WCAG 2.1 AA; ensure waveform overlays maintain legibility in light/dark modes

Interactive Visualizations:

- Color Theme Explorer: [ux-color-themes.html](./ux-color-themes.html)

---

### 3.2 Monitor Display Emulation

- Aspect Ratio: The MachineView render surface is fixed to 4:3 to reflect PAL-era display geometry. On wider screens, use letterboxing/pillarboxing; never stretch or crop.
- Display Modes: Support BBC display modes 0–7 per original specification, including proper Mode 7 (Teletext) rendering characteristics.
- Scaling: Use integer scaling factors when possible (1x, 2x, 3x, …) to maintain pixel-accurate output. When fractional scaling is required, apply nearest-neighbor to preserve crisp edges; disable any smoothing.
- CRT Effects: Do not simulate CRT artifacts (scanlines, bloom, curvature). Allow optional visualization of CRT draw timing/beam progression for educational purposes only (post-MVP toggle), with zero impact on core image fidelity.
- Framing/UI: HUD overlays must avoid occluding the active 4:3 raster; reserve margins within letterbox areas for indicators.

Acceptance hooks:
- Render surface remains 4:3 under all window/screen sizes (manual resize test).
- Mode switch UI (or BASIC commands) cleanly transitions among modes 0–7; Mode 7 teletext glyphs render correctly.
- Integer scaling preferred; non-integer scaling does not introduce blur.

---

## 4. Design Direction

### 4.1 Chosen Design Approach

- Direction: Modern Native with Retro Accents
- Rationale: Platform-native controls deliver performance and accessibility; retro styling is used judiciously to signal context (e.g., waveform, HUD) without impairing readability or App Store review outcomes. Skeuomorphic decks are avoided to reduce clutter on small screens.

Interactive Mockups:

- Design Direction Showcase: [ux-design-directions.html](./ux-design-directions.html)

---

## 5. User Journey Flows

### 5.1 Critical User Paths

1. First Run → Beeb Prompt → Type `PRINT "HELLO"` → See output (≤2 minutes)
2. Load Program from UEF → Transport Play → Waveform shows blocks → Program runs
3. Load from Microphone → Monitor levels → Decode → Program runs (≥85% success on fixtures)
4. Mount .ssd → *CAT catalog → Shift-boot → Title runs
5. Open Editor → Write with labels → Tokenize/Inject → Run → Detokenize back with labels → Diff clean
6. Enter Full Screen → HUD auto-hides → Single-tap reveals controls → Exit via visible button or Escape/Cmd+Ctrl+F → Return to windowed layout
7. “Where is BREAK?” → Open BBC Key Help → Search “break” → Overlay highlights BREAK mapping → User invokes BREAK successfully

---

## 6. Component Library

### 6.1 Component Strategy

- MachineView (Metal surface + HUD overlay)
- TapeDeckView (transport, waveform, meters, markers)
- DiskManagerView (mount/eject, catalog, write-protect, shift-boot)
- EditorView (SwiftUI text editor with gutter, labels, diff panel)
- Shared: LEDIndicator, HUDOverlay, WaveformView, MeterView, Toast/Banner
- Input: On-screen soft keys (iOS), hardware keyboard shortcuts (macOS)

Component details:
- MachineView
  - Responsibilities: Render 4:3 raster, enforce scaling rules, expose mode 0–7 selection (developer menu or commands), present unobtrusive HUD in letterbox margins.
  - States: Running, Paused, Resizing; Mode changed.
  - Constraints: No CRT shaders; integer scaling preferred; nearest-neighbor sampling only.
  - Full-screen: Provides opt-in full-screen toggle; ensures a persistent, obvious exit affordance; honors platform shortcuts (Escape, Cmd+Ctrl+F) and iOS tap-to-reveal controls.
  - Accessibility: Announce mode/full-screen changes; HUD and exit controls labeled with appropriate traits.

-- KeyboardOverlay
  - Responsibilities: Provide BBC-specific keys and function rows when a hardware keyboard is absent or lacks BBC keys; show context-aware hints.
  - Variants: Compact (iPhone), Expanded (iPad), Hidden (hardware keyboard connected).
  - States: Visible, Hidden, Hints-only.
  - Accessibility: Fully navigable; labels map BBC keys to VoiceOver-friendly names.

- GraphicalKeyboardView (Optional)
  - Responsibilities: Present a visually authentic BBC Micro keyboard (color, shading, layout) that responds to touch with haptic and audible key-click feedback.
  - Interaction: Touch/click generates mapped key events; supports modifiers and chords (SHIFT, CTRL, SHIFT+BREAK); rollover handling for multi-touch.
  - Settings: Toggle on/off; sound/haptics toggles (respect system silent mode and Reduce Motion/Sound); size and dock position (bottom/float).
  - Performance: Low-latency input path (<30 ms touch→key injection target); GPU-friendly rendering.
  - Accessibility: Full labels for keys (e.g., “BREAK”, “COPY”); groupings for VoiceOver navigation; high-contrast mode.

### 6.2 Keyboard Strategy (Authentic, Simple, Discoverable)

Guiding principles: authenticity, straightforwardness, simplicity, and easy understanding.

- Cross-platform core
  - Emulate the BBC key matrix faithfully; maintain a mapping layer from OS key events to matrix rows/columns.
  - Two presets: “Authentic” (true-to-hardware mappings) and “Convenient” (laptop-friendly).
  - Remapping: Optional per-key remap UI with presets (UK/US layout, Games profile). Export/import mappings as JSON.

- iOS/iPadOS
  - Hardware keyboard: Support OS key events and repeat; honor chords; provide an “Input Capture” toggle to prevent OS shortcuts from stealing focus during play.
  - On-screen overlay: BBC function row, special keys (BREAK, COPY, DELETE/BACKSPACE behavior, SHIFT LOCK), and common chords (SHIFT+BREAK) as single-tap buttons.
  - Discoverability: “BBC Key Help” overlay with search (e.g., “Where is BREAK?”) and visual highlight on corresponding control/shortcut.
  - Accessibility: Large touch targets; labels and traits; haptic confirmation on key press (where available).

- macOS
  - Hardware keyboard: Mapping with correct repeat and rollover; configurable handling of keys like BREAK, COPY, DELETE, SHIFT LOCK.
  - Shortcuts: Platform-safe defaults (never shadow Cmd+Q). Provide “Input Capture” mode in full-screen; display a tip on how to release capture.
  - Discoverability: Menu items and a help overlay listing key mappings; searchable panel; quick-jump to Remap Preferences.

Acceptance hooks:
- Typing test: User can type and run a simple BASIC program using either hardware or on-screen keys without consulting external docs.
- Chord test: SHIFT+BREAK chord is performable via one control (overlay) and via physical keys; behavior matches expectations.
- Capture test: In full-screen with capture ON, OS shortcuts don’t interfere; Escape (or a dedicated chord) releases capture.

Graphical Keyboard Mode (optional):
- Visual: Authentic BBC look (colors, shading, layout) without copyrighted branding; scalable; respects light/dark surroundings.
- Feedback: Haptic and audible key-click (toggleable); audio respects Silent Mode and volume; accessibility alternative text feedback.
- Behavior: Supports modifiers (SHIFT, CTRL), special keys (BREAK, COPY, ESCAPE, arrow keys), and chords; orientation-aware on iPad.
- Fallbacks: Users can choose Overlay vs Graphical; automatically fall back to Overlay on low-power mode if configured.
- Acceptance: Touch→key latency under 30 ms median; VoiceOver can navigate and activate every key; toggles persist across sessions.

---

## 7. UX Pattern Decisions

### 7.1 Consistency Rules

- Navigation: iOS uses segmented control/tab between Machine | Media | Editor; macOS uses sidebar + toolbar
- File Access: System DocumentPicker for import; no in-app downloads of binaries
- Scaling: Enforce 4:3 aspect at all times; integer scaling preferred (1x/2x/3x...). If fractional, use nearest-neighbor; no smoothing. Mode 7 must remain crisp and legible.
- CRT Effects: No CRT artifact simulation in MVP; optional educational draw-timing overlay (beam progression) may be added post-MVP and defaults OFF.
- Feedback: Non-blocking toasts for state changes; persistent inline errors for media decoding issues
- Controls: Keep primary actions within thumb reach on iPhone; expose advanced options in sheets/menus
- Full-screen Behavior:
  - Entry: Full-screen is opt-in via a visible toggle (toolbar button) and standard shortcuts.
  - Exit: Always easy—visible “Exit Full Screen” control, plus Escape (macOS), Cmd+Ctrl+F (macOS), single-tap to reveal controls on iOS with an explicit exit button.
  - Discoverability: A one-time, dismissible tip highlights how to exit full-screen the first time a user enters it.
  - Accessibility: Controls labeled for VoiceOver; focus management keeps exit affordance reachable.

- Keyboard Modes & Discoverability:
  - Modes: “Authentic” and “Convenient” presets, clearly described and switchable with preview.
  - Help: “BBC Key Help” overlay with search; highlights mapped control/shortcut.
  - Remapping: Non-destructive; show conflicts and suggestions; one-click reset to defaults.

- Input Capture:
  - Off by default; explicit toggle when entering full-screen; shows a small “Captured” indicator with a release hint.
  - Platform-appropriate release: Escape (macOS), long-press Exit control (iOS), or menu option.

- Keyboard UI Options:
  - Overlay (default) and Graphical Keyboard (optional) are user-selectable per device.
  - Audio/Haptics: Default respectful of system settings (Silent Mode, Reduce Motion/Sound); per-feature toggles in Settings.
  - A11y: High-contrast keycaps, larger sizes, and VoiceOver landmarks.

---

## 8. Responsive Design & Accessibility

### 8.1 Responsive Strategy

- iPhone: Portrait-first; overlay soft controls; condensed HUD
- iPad: Split views; expanded waveform and editor side-by-side
- macOS: Resizable panes; menu items and shortcuts; Inspector post-MVP
- Accessibility: VoiceOver labels for controls and waveform blocks; Dynamic Type where applicable; color contrast AA+; reduce motion honored

---

## 9. Implementation Guidance

### 9.1 Completion Summary

- UX aligns to PRD Epics M2–M5 with clear acceptance criteria hooks (50 Hz, <0.1% audio underruns, ≥85% tape success, zero round-trip data loss)
- Visuals rely on native components with minimal custom drawing beyond waveform and HUD
- Next: Wireframes for Machine, Tape Deck, Disk Manager, Editor; then Figma or direct SwiftUI builds backed by snapshot tests

---

## Appendix

### Related Documents

- Product Requirements: `docs/PRD.md`
- Product Brief: `docs/product-brief-BBC Model B-2025-11-03.md`
- Brainstorming: `N/A`

### Core Interactive Deliverables

This UX Design Specification was created through visual collaboration:

- Color Theme Visualizer: docs/ux-color-themes.html
  - Interactive HTML showing all color theme options explored
  - Live UI component examples in each theme
  - Side-by-side comparison and semantic color usage

- Design Direction Mockups: docs/ux-design-directions.html
  - Interactive HTML with design direction summaries
  - Screens overview for Machine, Media, Editor
  - Rationale for chosen approach

### Optional Enhancement Deliverables

This section will be populated if additional UX artifacts are generated through follow-up workflows.

### Next Steps & Follow-Up Workflows

This UX Design Specification can serve as input to:

- Wireframe Generation Workflow
- Figma Design Workflow
- Interactive Prototype Workflow
- Component Showcase Workflow
- Solution Architecture Workflow

### Version History

| Date       | Version | Changes                         | Author |
| ---------- | ------- | ------------------------------- | ------ |
| 2025-11-03 | 1.0     | Initial UX Design Specification | BMad   |

---

This UX Design Specification was created through collaborative design facilitation, not template generation. All decisions were made with user input and are documented with rationale.