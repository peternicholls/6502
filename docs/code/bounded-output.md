# Bounded Output Contracts {#bounded_output}

Phase C2 adds host-consumable video, audio, and diagnostic observations behind
the C1 `MachineRuntime` ownership boundary. This guide is the conceptual home
for those contracts as they are implemented. The approved requirements and
evidence remain in `specs/003-bounded-output-contracts/spec.md` and
`specs/003-bounded-output-contracts/plan.md`.

## Ownership boundary

Output is published only after a complete instruction and its aggregate device
tick. Public consumers receive owned values or caller-buffer copies; they never
borrow the runtime queue, renderer, or sound-generator storage. Runtime
synchronization remains internal, and no consumer callback runs while it is
held.

## Completed video frames

Each complete frame contains positive dimensions and exactly four packed
8-bit components per pixel in row-major red, green, blue, alpha order. Its
frame number is assigned when the runtime publishes the observation and is
strictly increasing for that runtime's lifetime, including across a device
reset. The identifier comes from emulated frame completion, never host time.

`MachineRuntime` copies the machine-owned render only after the instruction
that completed the frame and the corresponding aggregate device tick. It then
moves that value into a three-slot owner-only FIFO. Consumers dequeue the
oldest retained frame. When all three slots are occupied, publication replaces
exactly the oldest unconsumed frame, retains the newest three, reports overrun,
and increments the dropped-frame counter. At every observation:

```text
frames produced = frames consumed + frames dropped + retained frame depth
```

C++ dequeue transfers an owned `CompletedFrame`. C dequeue allocates a fresh
`beeb_frame` whose caller releases it with `beeb_frame_release()`. Swift copies
those bytes into `Data` before releasing the C allocation. None of these values
aliases the renderer, the FIFO, another result, or a later publication, so a
consumer may retain it while production continues.

An empty FIFO returns the structured empty category without mutating caller
output. Faulted and shutting-down lifecycles remain distinguishable from
ordinary emptiness. No callback or host lock participates in publication or
dequeue; both producer and consumer operations remain serialized by the C1
runtime owner.

## Continuous audio

Continuous output is one mono channel of IEEE 32-bit floating-point samples at
exactly 48,000 Hz. The 2 MHz emulated CPU clock is the only production clock:
each completed execution segment converts cycles to samples with the exact
reduced ratio `3 / 125`, retaining the fractional emulated-cycle remainder for
the next segment. SN76489 rendering and FIFO publication occur on the runtime
owner after the same completed-instruction/device-tick transition used for
frames.

The fixed ring retains at most 4,096 samples, approximately 85 ms at the fixed
rate. Demand is `max(0, 2,048 - retained depth)`. Production never waits for a
host consumer. When incoming samples exceed capacity, the ring discards exactly
the oldest retained samples, keeps the newest 4,096 in FIFO order, and increases
the cumulative overrun count by the number discarded.

A drain requests a maximum count and receives owned samples in FIFO order. C++
and Swift results own their arrays; C copies only valid values into caller
storage. If fewer samples exist, every available sample is returned and
`shortfall = requested - copied`; the operation reports structured underrun and
adds that exact shortfall to the cumulative underrun count. Allocation failure
occurs before ring mutation. After every operation:

```text
audio samples produced = samples consumed + samples overrun + retained depth
```

The drain result observes copied count, shortfall, post-drain demand, and both
pressure counters atomically. A Swift underrun remains a recoverable typed error
carrying the valid owned partial drain. Host elapsed time may be supplied later
to an observational rate helper, but no host clock is stored in BeebCore, drives
SN76489 production, changes demand, or advances emulated state.

## Diagnostics and recovery

One consistent snapshot exposes emulated progress, queue depths and capacities,
audio demand, and pressure counters. Host helpers calculate an informational
emulation-rate ratio from explicit observations; host time is not retained by
BeebCore. Empty, pressure, lifecycle, and production failures remain structured
and recoverable at each language boundary.

## Evidence

Focused C++, C, Swift, lifetime, replay, race, and sustained-measurement tests
own the implementation evidence. Generated artifacts remain under ignored
`.build/c2/` storage. Capacity, accounting, retained-value immutability, memory,
and rate-tolerance claims are not reported as delivered until the final C2
measurement and validation tasks pass.
