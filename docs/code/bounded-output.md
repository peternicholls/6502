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

The frame contract covers the packed RGBA format, monotonic emulated frame
identity, capacity-three FIFO, oldest-first dequeue, drop-oldest overflow, and
exact discard accounting. It also defines how empty and lifecycle outcomes map
across C++, C, and Swift.

## Continuous audio

The audio contract covers deterministic mono Float32 production at 48 kHz, the
4,096-sample FIFO and 2,048-sample demand target, caller-owned drains, and exact
underrun and overrun accounting. Host clocks may observe output pressure but do
not advance or schedule core state.

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
