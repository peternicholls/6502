# Product capability catalogue

**Status:** Supporting product scope; not a delivery authority
**Updated:** 2026-07-19

This catalogue describes the user-facing capabilities Beeb6502 may deliver.
It does not choose the next feature, assign priority, define gates, or promote
work into scope. The
[Machine application delivery plan](MACHINE_DELIVERY_PLAN.md) is the sole
forward programme authority. If this catalogue and that plan differ, the plan
wins and this file must be corrected.

Verified implementation state belongs in [core status](../STATUS.md). Current
technical boundaries belong in [architecture](../ARCHITECTURE.md). The
[product vision](VISION.md) supplies durable intent, not delivery order.

## Machine experience

The Machine experience makes supported BBC Micro profiles recognisable,
selectable and honest about their compatibility boundaries.

Capability scope:

- import, validate and assign user-owned operating-system, language and filing
  system ROMs without bundling proprietary content;
- select among implemented machine profiles without inferring identity from a
  filename or silently reinterpreting state;
- boot, type and run BBC BASIC programs through accessible physical and
  on-screen input;
- present bounded video and audio without making host refresh or device
  callbacks the source of emulated time;
- expose distinct run, pause, reset, BREAK, full-screen and input-capture
  controls;
- report active profile, runtime health, output pressure and known fidelity
  limits; and
- preserve sessions through versioned, profile-aware snapshots and
  platform-appropriate lifecycle handling.

## Media experience

Media handling supports use and preservation of user-owned material.

Capability scope:

- mount, eject, protect and diagnose supported disk images;
- export modified media explicitly without silently overwriting imports;
- load supported UEF and WAV tape files with progress, motor state and
  recoverable decoding errors;
- add live microphone capture only after file-based decoding has maintained
  corpus evidence; and
- make controller, filing-system and profile compatibility explicit.

## Editor and inspection experience

The Editor adds modern assistance without replacing authentic execution.

Capability scope:

- inspect stable CPU, memory and device observations without racing execution;
- provide bounded breakpoint and watchpoint control;
- perform validated atomic machine-memory transactions;
- tokenize and detokenize BBC BASIC deterministically;
- map labels to generated line numbers without changing the executed language;
- inject, run, retrieve and compare programs; and
- require explicit conflict resolution when source and machine RAM diverge.

## Native product experience

The application uses native Apple-platform conventions while preserving a
portable core.

Capability scope:

- maintained macOS, iPhone and iPad layouts appropriate to each input model;
- VoiceOver, Dynamic Type, contrast, reduced-motion and keyboard access;
- concise onboarding, empty states and actionable recovery;
- import-only legal and privacy explanations suitable for review; and
- compatibility, sustained-performance and device-matrix evidence before
  external beta or store claims.

## Machine and expansion namespace

The application architecture is not limited to two hard-coded machines.
Machine identity consists of a stable base-profile identifier plus explicit,
versioned configuration for implemented expansions. Unknown profiles or
expansions must reject safely rather than fall back to another machine.

The committed delivery plan names the profiles that may be implemented now:

- BBC Model B, retained as the permanent regression profile; and
- BBC Model B+ 64K, added as a separate selectable option for M3.

The following are named future candidates, not current delivery commitments:

- BBC Model B+ 128K;
- BBC Master 128 and other separately researched Master-family revisions;
- Acorn Tube interfaces and individually identified second processors;
- Econet and other network expansions;
- ADFS and additional storage/controller profiles;
- analogue, speech, joystick and other peripheral expansions; and
- additional BBC Micro-family revisions supported by primary references and
  lawful compatibility fixtures.

Each future candidate requires its own product case, profile or expansion
contract, lawful evidence, and delivery-plan amendment. Naming a candidate here
does not claim implementation, compatibility, priority, or inclusion in M3.

## Product-wide completion qualities

A selected slice is complete only under the gate that names it in the delivery
plan. Its feature specification must also cover, where applicable:

- an independently demonstrable user outcome;
- automated and maintained acceptance evidence;
- explicit failure and recovery behavior;
- accessibility and native-platform behavior;
- content provenance and non-destructive import/export rules;
- measured compatibility, performance or timing claims; and
- documentation that distinguishes delivered behavior from known limitations.

Changes to capability scope may be proposed here, but they do not enter the
programme until [MACHINE_DELIVERY_PLAN.md](MACHINE_DELIVERY_PLAN.md) is amended.
