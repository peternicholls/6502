# Desktop experience direction

**Status:** Current product-experience contract; no delivery authority
**Updated:** 2026-08-01

This document records how the macOS and terminal experiences should work. The
[Machine delivery plan](MACHINE_DELIVERY_PLAN.md) alone decides order, feature
scope and completion gates. [STATUS.md](../STATUS.md) records what exists now.

## Experience model

The desktop product has two deliberately different hosts over the same machine
session:

1. an AppKit application that feels like a native Mac virtual-computer or
   terminal window; and
2. a bare terminal connection for direct interaction, diagnostics and
   deterministic testing.

Neither host owns emulated truth or advances emulated time from its presentation
clock. Both use the same runtime, firmware, keyboard, frame, audio, control and
diagnostic contracts.

## Main AppKit window

The machine window is intentionally uncluttered:

```text
┌─────────────────────────────────────────────────────────────────┐
│ [Run/Pause] [BREAK] [Reset] [Open Program] [Media] [Settings] │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│                            CRT                                  │
│                  centred 4:3 active display                     │
│                    within full-width underscan                  │
│                                                                 │
├─────────────────────────────────────────────────────────────────┤
│ ● Running · Model B · BASIC II · MOS 1.20 · 50.00 Hz · Drive 0 │
├────────────── optional BBC keyboard drawer ─────────────────────┤
│ [f0] … [f9] [BREAK] [COPY] [CAPS LOCK] [SHIFT LOCK] [arrows]  │
└─────────────────────────────────────────────────────────────────┘
```

The ordering is fixed: native toolbar, CRT, running-machine footer, then the
optional keyboard drawer. Opening the keyboard drawer increases the available
window height or reduces surround space; it must not distort the active raster.

### Toolbar

The native customisable toolbar operates the current machine. Its default
items are Run/Pause, BREAK, Reset, Open Program, Media, Developer Tools and
Settings. It may be collapsed or hidden.

Machine selection does not appear in the toolbar. Model, memory, hardware,
firmware, ROM banks and compatibility choices belong in Settings. Media
controls expose only operations actually supported by the active profile;
unimplemented media features do not appear as misleading active controls.

### CRT and resizing

- The CRT area spans the window from left edge to right edge.
- The final active viewport is 4:3 and centred inside an underscan/monitor
  surround.
- Source pixels use the aspect ratio of the emulated video mode; presentation
  must not assume that every source pixel is square.
- Raster sampling is nearest-neighbour with no antialiasing or smoothing.
- Normal resizing preserves the active aspect ratio. Extra space changes the
  surround rather than stretching the machine image.
- Full screen may hide toolbar and footer independently for a minimal monitor
  presentation while retaining a discoverable release path.
- The displayed text and graphics come from the emulated video/character
  generator. A Mac font must not replace the machine display.

Clicking the CRT makes the machine the active keyboard target. Focus capture
and release must remain predictable after toolbar, Settings, menu and drawer
interaction. Mouse scrolling may provide an optional convenience mapping where
the active machine or tool supports it; it must not invent unrequested BBC
input.

### Running-machine footer

The footer reports the active session rather than configuring it. It may show
run state, active model/profile, language ROM, MOS version, measured refresh,
keyboard lock state, selected ROM bank and mounted-media summary. It follows
the CRT and precedes the keyboard drawer, and may be hidden.

### BBC keyboard drawer

The drawer has Hidden, Essential and Full presentations. Essential mode covers
BBC keys that have no clear Mac equivalent, including BREAK, COPY, BBC function
keys, Caps Lock, Shift Lock and cursor keys. Full mode provides the complete BBC
layout. Physical key bindings remain configurable in Settings.

## Settings and safety interlocks

Settings is a separate native AppKit window, organised around General, Machine,
Firmware/ROM banks, Storage, Input, Display, Audio, Mac Integration and
Developer sections. Authentic, Balanced, Convenient and Custom presets may
provide understandable starting points without hiding the resulting settings.

Every setting declares one application class:

| Class | Examples | Required behavior |
| --- | --- | --- |
| Live | volume, CRT appearance, toolbar/footer visibility | Apply immediately without disturbing machine execution. |
| Pause-required | selected input or safe media operations | Pause at the runtime boundary, apply atomically, then offer or perform safe resume. |
| Restart-required | model, RAM, hardware devices, ROM layout | Stage visibly and apply only through **Apply and Restart**. |
| Potentially destructive | reset, writable-media replacement, incompatible restore | Create a recovery checkpoint or show a targeted warning based on actual vulnerable state. |

BREAK remains immediate. Warnings are not added indiscriminately: modern safety
comes primarily from failure-atomic changes, explicit writable-media policy and
recoverable checkpoints. Staged settings always distinguish the selected future
configuration from the active machine reported by the footer.

## Program text input

Open Program accepts explicitly supported text/source formats. Before loading,
a preview sheet:

- detects existing BASIC line numbers;
- preserves or renumbers numbered input as requested;
- can number unnumbered lines using a visible start and increment;
- identifies characters that cannot be represented by the selected key map;
- offers paced authentic typing and an explicitly labelled accelerated input;
- shows progress and permits cancellation.

The first implementation feeds the emulated keyboard/input path so MOS and
BASIC consume the text normally. It does not write directly into screen memory
or the BASIC workspace. Later tokenization and atomic program-boundary editing
remain separate developer/editor work.

## Media access

Routine media actions belong in the toolbar, menus and drag-and-drop handling.
A media popover may show drive/cassette slots, mounted names, protection state,
eject, choose/create and recent-media actions. A drop opens an explicit mount
workflow rather than silently choosing a device.

Settings owns hardware/controller configuration and durable media policy. The
main window owns frequent mount/eject operations. Imported source media is never
silently overwritten; modified content leaves through an explicit export.

## Bare terminal connection

Terminal mode is a direct connection to the Beeb, not a text-styled copy of the
AppKit application. Its interactive screen contains only machine display and
cursor output: no border, toolbar, footer, host status or decorative CRT.

- Terminal local echo is disabled; a key appears only when the emulated machine
  displays it.
- Ordinary input passes through the same BBC key mapping and runtime command
  path as AppKit.
- Paste is delivered as bounded, cancellable, paced keystrokes.
- A configurable escape prefix provides hidden quit, BREAK, reset,
  pause/resume, media and literal-prefix commands.
- Text modes are reconstructed from video state rather than by intercepting
  BASIC or OS print calls. Optional ANSI/block rendering may approximate
  graphics but does not claim CRT fidelity.
- Alternate-screen and raw-terminal state are restored on normal exit, errors
  and handled signals.

The same executable surface also supports noninteractive regression work. A
scripted run can select profile and user-supplied firmware, submit bounded key
events or a program, wait for a machine condition with an emulated-time limit,
and emit stable text, frame/state digests, diagnostics and process status.
Script mode must execute the production `MachineRuntime`; it may not substitute
a mock console, duplicate CPU or host-side emulation clock. This makes it useful
for CPU, machine, ROM and device integration testing while preserving the
portability of the C++ core.

The terminal host should remain portable across maintained POSIX build lanes by
using the stable C ABI. Terminal/TTY and ANSI implementation stays in the host
executable; it must not introduce platform headers or behavior into BeebCore.

## Developer workspace

Developer tools open in a separate AppKit window rather than permanently
reducing the CRT. Planned observations include A/X/Y, PC, stack pointer and
status flags; disassembly; stack page; zero page; memory; breakpoints and
watchpoints; VIA, CRTC, ULA, keyboard matrix, ROM, audio and disc-controller
state; and bounded runtime trace.

The 6502 has no modern CPU cache to display. Tools use stable owner-produced
snapshots at safe boundaries and bounded atomic commands; they never borrow live
core state from the UI thread.

## UI and UX evidence

User experience is reviewed while features are developed. Each affected slice
records, in proportion to its change:

- representative window sizes, resize transitions and full-screen behavior;
- raster aspect, pixel sharpness and underscan behavior;
- pointer and keyboard-only focus/capture/release journeys;
- toolbar, footer, drawer, Settings and interlock behavior;
- appearance, contrast, accessibility names/actions and status announcements;
- error recovery and the amount of interruption caused by safeguards; and
- direct hands-on acceptance supported by screenshots or recordings where they
  materially prove the claim.

Automated checks protect contracts and regressions, including terminal parity,
but do not replace direct observation of a changed AppKit interaction.
