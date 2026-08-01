# Implementation Plan: Model B Acceptance Automation

**Branch**: `007-model-b-acceptance-automation` | **Date**: 2026-08-01 | **Spec**: [spec.md](spec.md)

## Summary

Add production-path automated evidence for the remaining M1 acceptance
boundary. Extend the existing BeebMachine XCTest coverage to prove deterministic
matrix input, owned frame/audio output and failure-atomic firmware rejection;
add a focused Model B workflow script that runs those tests and a portable
headless fixture check; record the results separately from the still-human
physical keyboard, visual and assistive-technology observations.

## Technical Context

**Language/Version**: C++20, Swift 5.9+

**Documentation Strand**: cross-strand

**Delivery-Plan Trace**: [M1 acceptance closure / `machine-model-b-workflow`](../../docs/product/MACHINE_DELIVERY_PLAN.md#critical-path-slices)

**Supporting Context**: [status](../../docs/STATUS.md),
[architecture](../../docs/ARCHITECTURE.md),
[implementation constraints](../../docs/IMPLEMENTATION_CONSTRAINTS.md),
[evidence guide](../../docs/code/evidence-and-testing.md), and
[desktop experience direction](../../docs/product/DESKTOP_EXPERIENCE.md).
Completed 005 artifacts are frozen evidence only.

**Primary Dependencies**: Existing BeebCore, C ABI, BeebKit, XCTest, Makefile
and generated clean-room fixtures. No new dependency.

**Storage**: Temporary `.build/` artifacts only; no ROM or user media bytes in
Git.

**Testing**: Focused `swift test --filter BeebMachineTests`, C++ `make test`,
`make test-model-b-workflow`, `make docs-check`, `make format-check`, and the
portable headless fixture command. No GUI automation claim is made.

**Code Documentation**: Test helpers and shell contracts document why they use
the production boundary and what they cannot prove. Run `make docs-check`.

**Git Checkpoints**: Commit the focused Swift test task after its test passes;
commit the aggregate/evidence task after its checks pass; commit final Spec Kit
bookkeeping only after the automated evidence record is complete. Preserve
user-owned dirty files and stage only feature paths.

**Target Platform**: Portable C++/C runner plus macOS Swift package tests.

**Project Type**: Portable emulator library with macOS host and test harness.

**Performance Goals**: Deterministic bounded runs; no host wall-clock-driven
execution and no unbounded fixture or output allocation.

**Constraints**: Keep the production runtime owner and C ABI unchanged unless a
test exposes a real contract gap; retain lawful-ROM boundaries; do not claim
direct human acceptance from automated checks.

**Scale/Scope**: One M1 acceptance closure slice: three focused automated
observations, no new product UI and no interactive Terminal implementation.

## Constitution Check

- [X] The feature is named by the M1 acceptance-closure row in the sole forward
      programme authority.
- [X] The feature is cross-strand and links only current supporting documents;
      completed 005 material is evidence only.
- [X] The outcome is a bounded vertical test/evidence slice with explicit
      non-goals.
- [X] Core determinism and portability are preserved; no host framework or
      wall-clock execution enters the core.
- [X] Test-first evidence covers C++/portable, C-boundary and Swift production
      wrapper paths as applicable.
- [X] This is not a user-facing implementation slice, so direct application
      launch/observation remains explicitly open rather than falsely marked.
- [X] Fidelity and compatibility claims remain tied to named fixtures and
      bounded observations.
- [X] Existing ownership, lifetime, errors, threading and ABI contracts are
      reused without a persisted-format change.
- [X] Test helpers and shell contracts document their responsibility; generated
      documentation remains validated by `make docs-check`.
- [X] No proprietary firmware or user media is imported into Git.
- [X] Accessibility is not claimed by automation and remains a human gate.
- [X] No new dependency or abstraction is required.
- [X] Every task has a focused verification and Lore commit checkpoint.
- [X] Completion will update the live evidence/status owner, clear the feature
      pointer and move accepted artifacts under `specs/completed/` only after
      all automated and required human gates are resolved.

## Project Structure

```text
Sources/BeebKit/BeebMachine.swift       # existing production wrapper
Tests/BeebKitTests/BeebMachineTests.swift # focused production-path tests
Tests/ModelBWorkflow/test-runtime-acceptance.sh # aggregate automation
Tests/ModelBWorkflow/testlib.sh          # existing shell helpers
specs/007-model-b-acceptance-automation/ # feature evidence and contracts
```

**Structure Decision**: Reuse the existing Swift wrapper test target and Model B
workflow shell aggregate. Do not introduce a second runtime, CLI framework or
test dependency.

## Complexity Tracking

No constitution violations or complexity exceptions.
