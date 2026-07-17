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

## Runtime safe points

`MachineRuntime` serializes commands at the same aggregate boundary. A safe
point identifies the CPU cycle total, latest completed frame, runtime state,
and ledger sequence only after the instruction or interrupt and its device
ticks have completed. CPU, frame, and fault observations are owned copies made
there; callers cannot observe an intermediate instruction or device update.

Sustained execution selects a 2,048-cycle minimum slice only when the FIFO is
empty. Whole-instruction completion may take the actual count slightly beyond
the request. The owner checks the FIFO before selecting another slice, so an
accepted pause waits for at most the already selected slice. Bounded execution
is a separate paused-only command. Neither path consults host wall time.

If an emulated instruction fails, its processor-local transition is discarded
while earlier complete instructions and their device ticks remain committed.
The fault ledger records those retained `actualCycles`, and its safe point names
the same CPU/device boundary reported by observations and the replay digest.
Allocation and unexpected implementation failures restore the process-local
whole-machine checkpoint captured before the execution transaction, so they
cannot mix restored CPU time with advanced peripheral state.

## Output timebase

C2 publication is part of the owner transaction after completed execution, not
a host refresh callback. A newly completed machine frame is copied into the
bounded output FIFO only after the instruction and aggregate device tick that
made it complete. Continuous audio converts each committed CPU-cycle delta at
the exact 2 MHz-to-48 kHz ratio `3 / 125`; the owner retains the fractional
remainder between segments so slice boundaries do not change the sample stream.
Frame and audio production therefore remain deterministic across bounded and
sustained execution.

Consumer drains do not advance emulation, and output diagnostics only observe
one owner boundary. A host may later pair two diagnostic values with a measured
wall-clock interval to calculate emulated seconds per host second. That pure
calculation neither changes demand nor feeds time back into the CPU, device,
frame, or audio schedule.

## Where to extend fidelity

Future timing work should strengthen the device and bus model behind these
interfaces, retain deterministic elapsed-time accounting, and add evidence
before changing observable cycle or frame semantics.

A bus-cycle sequencer may add internal micro-step boundaries, but those are not
automatically host-visible safe points. Until a separately specified API
migration says otherwise, it must:

- keep each accepted command atomic with respect to a completed instruction and
  fully advanced devices;
- prevent pause, media, input, and observation commands from seeing a partial
  bus transaction;
- retain one total owner order for command and execution evidence; and
- preserve frame/audio publication only at complete externally visible safe
  points, including the exact audio remainder across any finer internal steps;
- revalidate the 2,048-cycle arbitration and pause-latency contract if internal
  execution no longer maps directly to `CPU6502::step()`.
