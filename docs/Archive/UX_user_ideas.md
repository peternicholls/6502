# ---
title: "BBC Micro Emulator UI - User Ideas"
status: ARCHIVE
type: UX
version: 1.0.0
owner: User
date-created: 2025-11-03
date-updated: 2025-11-05
description: "Brainstormed user interface ideas and accessibility considerations for BBC Micro Emulator."
tags: ["ux", "ideas", "archive"]
keywords: ["ui", "design", "accessibility"]
---
# BBC Micro Emulator UI - User Ideas

**Author:** User
**Date:** 2025-11-03
**Project Level:** 4
**Target Scale:** Comprehensive (Greenfield)

---

## Monitor Display Emulation

- The monitor area should be 4:3 PAL style, constrained to retain accuracy of emulation.
- BBC Model B should use display modes 0 - 7 as were the original specification of the machine.
- The monitor area shall be a replication of a CRT monitor, however we will not add CRT artifacts, emulation or characteristics (except for exploreing the CRT draw time and style)

---

## Keyboard Considerations (Brainstorm)

Principles: authenticity, straightforwardness, simplicity, easy understanding.

- Cross-platform mapping
	- Maintain a BBC key-matrix mapping layer; support “Authentic” and “Convenient” presets.
	- Allow optional per-key remapping with conflict detection and easy reset; presets for UK/US layouts and Games.

- iOS/iPadOS
	- On-screen KeyboardOverlay with BBC function row and special keys (BREAK, COPY, DELETE, SHIFT LOCK); single-tap for common chords (e.g., SHIFT+BREAK).
	- Hardware keyboard support with proper repeat; Input Capture toggle for uninterrupted play; one-tap help overlay (“Where is BREAK?” search).
	- Accessibility: Large touch targets, VoiceOver labels; haptics on key press where available.

- macOS
	- Hardware keyboard mapping with correct repeat/rollover; configurable handling for BREAK/COPY/DELETE/SHIFT LOCK.
	- Safe shortcuts: never shadow Cmd+Q; Input Capture in full-screen with clear escape (Esc) and first-use hint.
	- Discoverability: Menu for key mappings, searchable Help overlay, jump to Remap Preferences.

- Testing/acceptance ideas
	- BASIC typing test works from both overlay and hardware keyboard without docs.
	- SHIFT+BREAK available via overlay and keys; behaves consistently.
	- Capture prevents OS shortcuts during play; Escape/tap reliably releases.

---

## Graphical BBC Keyboard (Idea)

- Visual authenticity: Skin that resembles the BBC Micro keyboard (color, shading, layout), scaled to device; avoids copyrighted branding.
- Interaction: Touch produces mapped key events; supports modifiers and chords (e.g., SHIFT+BREAK); multi-touch rollover.
- Feedback: Haptic + audible “key click” reminiscent of the Beeb; toggles for sound/haptics; audio respects Silent Mode.
- Accessibility: Clear labels for each key; high-contrast mode; larger keys option; fully navigable with VoiceOver.
- User choice: Toggle between simple Overlay and Graphical Keyboard; remember per-device preference; fallback to Overlay on low-power if configured.
- Performance: Target touch→key latency under ~30 ms; low-GPU overhead rendering.
