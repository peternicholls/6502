# The Timing Model

C0 is instruction-level, not cycle-exact. `CPU6502::step()` completes one
instruction or interrupt, determines its NMOS 6502 cycle count, then calls
`Bus::tick()` once with that count. The state change and the elapsed-time report
therefore form one deterministic transition.

## Clock domains

`BBCMicro::tick()` treats CPU cycles as the 2 MHz reference timebase.

- The system and user VIAs receive half that rate. A stored remainder preserves
  odd CPU cycles between instructions.
- The CRTC receives either the CPU rate or half the CPU rate according to the
  Video ULA clock selection. Its own remainder preserves fractional progress.
- The 8271 controller consumes CPU-cycle time directly for its current command
  latency and byte-transfer schedule.

This aggregation prevents each device from inventing its own host clock and
makes replay independent of wall time.

## Frames and transitions

The CRTC advances character, raster, character-row, vertical-adjust, and frame
counters from programmed registers. Completing a frame raises `frameReady`.
The machine then renders exactly one frame, pulses the system VIA's CA1 line,
and consumes the notification before the next CPU instruction.

`runFor()` may exceed its requested budget because an instruction is never
split. `runUntilFrame()` checks frame identity after each complete instruction.
These are deliberate C0 semantics and must not be described as bus-cycle or
hardware-clock fidelity.

## Where to extend fidelity

Future timing work should strengthen the device and bus model behind these
interfaces, retain deterministic elapsed-time accounting, and add evidence
before changing observable cycle or frame semantics.
