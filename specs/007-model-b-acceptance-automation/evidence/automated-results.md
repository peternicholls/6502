# Automated Model B acceptance results

Date: 2026-08-01
Branch: `007-model-b-acceptance-automation`

## Automated evidence

- `swift test --filter BeebMachineTests/testProductionInputSequenceProducesDeterministicOwnedOutput` — PASS
- `swift test --filter BeebMachineTests/testRejectedFirmwareCandidatePreservesValidMachine` — PASS
- `make test-runtime-acceptance` — PASS
- The production Swift test drives matrix key press/release events, proves
  safe-point progress and compares owned frame/audio/diagnostic values across
  fresh runtimes.
- The portable harness builds a clean-room ROM, runs `beeb-headless` twice and
  compares normalized state output and complete PPM frames. No proprietary ROM
  bytes are committed or required.

## Human gates still open

These remain intentionally outside automation and must be observed by a
developer using the macOS application and local, user-owned ROMs:

- Type a BASIC program and confirm visible output through the real AppKit host.
- Confirm physical keyboard focus, BREAK/reset safety interlocks and visual CRT
  behavior (including resize/aspect/underscan presentation).
- Confirm invalid-ROM import recovery in the application, accessibility behavior
  and any assistive keyboard presentation.
