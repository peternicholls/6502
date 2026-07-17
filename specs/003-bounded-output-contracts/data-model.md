# Data Model: Phase C2 Bounded Output Contracts

## CompletedFrame

Represents one complete video observation published at a C1 safe point.

| Field | Rule |
| --- | --- |
| frame number | Monotonic emulated identifier; never derived from host time |
| dimensions | Positive, fixed for the configured output format |
| pixel format | Explicitly named and stable for the contract |
| bytes/view | Valid for the documented ownership lifetime; never concurrently mutated |
| status | Success, empty, lifecycle, capacity, or production failure as applicable |

## AudioChunk

Represents an ordered bounded sequence of generated samples.

| Field | Rule |
| --- | --- |
| sequence/position | Emulated sample position, deterministic for identical input |
| sample format | Explicit scalar format, channel count, and rate metadata |
| sample count | Never exceeds requested output capacity |
| samples/view | Valid for documented lifetime and ownership |
| demand/pressure | Reports available count, requested count, and recoverable shortfall/overflow |

## OutputQueue

Finite storage owned by the runtime owner for one output kind. It has a
configured capacity, producer/consumer operations, empty/full transitions, and
a deterministic overflow policy. Queue mutation happens only at the owner
boundary; consumers receive operation-owned or immutable values.

## OutputDiagnostics

One consistent observation containing emulated progress, latest frame number,
frame availability, audio availability/demand, overrun/underrun counters, and
the last recoverable output status. It is observational and does not advance
machine state.

## State transitions

`Produced -> Queued -> Transferred/Consumed` is the normal path. A full queue
transitions a produced item through the documented overrun policy; an empty
consumer request returns `Empty` or `Underrun` without creating an invalid
view. Paused, faulted, and shutting-down runtime states reject or qualify
production and consumption through existing lifecycle statuses.
