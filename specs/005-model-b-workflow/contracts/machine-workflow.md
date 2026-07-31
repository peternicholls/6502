# Model B Workflow Contract

## Input and controls

The macOS machine view accepts the documented physical-keyboard sequence when
it owns input focus. Each press/release uses the established owner-serialized
matrix command. Run, pause, reset and BREAK are independent keyboard-operable
controls; BREAK remains a BREAK request rather than a host shortcut for an
unrelated lifecycle mutation.

The acceptance program is entered exactly as `10 PRINT "BEEB6502"`, Return,
`RUN`, Return. The expected visible result includes `BEEB6502`. The implementation
records the physical key press/release mapping used for that sequence alongside
the macOS observation; it does not add text injection or another input path.

## Presentation

The host requests bounded runtime progression and displays only owned completed
frames. Display refresh and timer scheduling consume output; they do not derive
emulated time. The host discards stale output after reset/BREAK according to the
runtime output epoch and presents an actionable diagnostic if it cannot consume
a frame.

## Recovery

A firmware, input, control or presentation failure reports its typed status and
leaves the active supported profile visible. It never creates a second runtime,
borrows live core state or silently falls back to Model B+.

## Acceptance observation

The maintained macOS application is built and launched on a named host. The
record includes firmware-role selection, Model B identity, keyboard sequence,
visible BASIC result, two frame observations, each control result, one
recoverable failure and keyboard/assistive observations.
