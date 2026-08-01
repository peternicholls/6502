# Research: Model B acceptance automation

## Decision: Extend existing production-path tests

The repository already exposes the required owned operations through
`BeebMachine`: firmware loading, reset, bounded execution, matrix input,
completed-frame dequeue, audio drain, diagnostics and safe-point observation.
Focused XCTest coverage can therefore prove machine/runtime behavior without
GUI automation or a second emulator.

## Decision: Use synthetic lawful fixtures for portable automation

Existing tests generate minimal OS ROMs in memory and do not commit proprietary
ROM bytes. The automated slice will use the same approach. User-owned ROMs
remain necessary for the final direct BASIC typing observation.

## Decision: Keep the terminal-style command bounded

The existing C++ headless host and C ABI are the portable evidence surface. The
slice adds a workflow script around that surface but does not implement the
interactive Terminal frontend planned for M2. This preserves the distinction
between automated machine evidence and a future user-facing terminal.

## Decision: Treat human acceptance as an explicit residual

Synthetic or Swift tests cannot prove physical Mac keyboard delivery, visual
CRT presentation, assistive technology or the actual user-owned BASIC ROM
journey. Evidence must list those observations as open rather than converting
automation into a false completion claim.

## Alternatives rejected

- GUI scripting as the primary proof: synthetic key injection can fail at the
  AppKit responder boundary and is not a reliable machine contract test.
- Direct RAM/screen mutation: bypasses MOS/BASIC and would invalidate the
  authenticity requirement.
- New test framework or terminal dependency: existing Make/XCTest surfaces are
  sufficient and keep the C++ core portable.
