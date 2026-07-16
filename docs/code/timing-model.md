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

If an instruction, trace observer, or aggregate device transition throws, the
runtime restores the process-local whole-machine checkpoint captured before
that execution transaction. Discarded transitions contribute zero retained
`actualCycles`; the fault safe point therefore names the same CPU/device
boundary that observations and replay digest report, rather than mixing
restored CPU time with advanced peripheral state.

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
- revalidate the 2,048-cycle arbitration and pause-latency contract if internal
  execution no longer maps directly to `CPU6502::step()`.
