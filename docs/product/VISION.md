# Product vision

**Status:** Durable intent only; no delivery authority
**Updated:** 2026-08-01

Beeb6502 is a native Apple environment for using, understanding and programming
BBC Micro-family machines. It combines authentic execution, careful handling
of user-owned media and modern development assistance without hiding known
emulation limits.

The [Machine delivery plan](MACHINE_DELIVERY_PLAN.md) is the only source of
committed scope, order and gates. [Implementation status](../STATUS.md) is the
only source of completion claims.

## People and outcomes

| Audience | Outcome |
| --- | --- |
| Creators and learners | Write, run, inspect and revise BBC BASIC without losing compatibility with the emulated machine. |
| Players and collectors | Import owned firmware and media, understand compatibility and avoid damage to source files. |
| Explorers and educators | Observe the CPU, memory and devices without first becoming emulator experts. |
| Terminal and automation users | Connect directly to the same machine without GUI chrome and reproduce complete-machine behavior in scripts. |

## Product experiences

- **Machine:** selectable, recognisable BBC profiles with dependable input,
  display, audio, controls, diagnostics and session continuity.
- **Media:** explicit mount, protection, diagnosis and export for user-owned
  disc and tape/file content.
- **Editor and inspection:** stable observations and modern BBC BASIC tools
  layered over authentic machine execution.
- **Desktop platform:** an AppKit-first macOS machine window, native Settings,
  safe modern conveniences and separate developer tools, plus a bare terminal
  connection over the same portable runtime.
- **Mobile platform:** later iPhone and iPad adaptation with touch, hardware
  keyboard, VoiceOver, Dynamic Type, lifecycle and media behavior appropriate
  to each device without forking machine semantics.

## Principles

1. Accuracy claims require reproducible evidence; gaps remain visible.
2. Emulated time belongs to the machine, never to host presentation clocks.
3. Modern assistance may reduce friction but must not change what the machine
   executes.
4. Proprietary firmware and user media remain external, local and explicit.
5. Imported source media is never silently overwritten.
6. Model and expansion identity is explicit and extensible.
7. Native accessibility is part of each user-facing slice, not a release-only
   retrofit.
8. Invalid or unsupported input fails safely and explains recovery.
9. UI and UX quality is evaluated with each user-facing slice, not deferred to
   final polish.
10. Desktop hosts may be native or terminal-based, but the C++ machine remains
    independent of Apple frameworks and host presentation.

## Product boundary

Committed profiles are Model B and Model B+ 64K. B+ 128K, Master-family
revisions, Tube second processors, Econet and additional storage or peripheral
expansions remain named options only. They require new research, evidence and a
delivery-plan amendment.

The product does not bundle proprietary ROMs or commercial software, download
executable code, depend on a server, silently rewrite imported media or claim
preservation-grade fidelity before the evidence supports it.

Success begins with a Mac user importing compatible firmware, booting to BASIC
and typing/running a program without shell commands. It then becomes a coherent
native desktop experience with a direct terminal alternative, dependable
continuity, media preservation, inspection and BBC BASIC assistance. iPhone and
iPad follow as committed platform adaptations over those stable portable
contracts rather than constraining the first desktop design.
