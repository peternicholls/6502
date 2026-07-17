# Data Model: Phase C2 Bounded Output Contracts

## CompletedFrame

Represents one complete video observation published at a C1 safe point.

| Field | Rule |
| --- | --- |
| frame number | Monotonic emulated identifier; never derived from host time |
| dimensions | Positive, fixed for the configured output format |
| pixel format | Explicitly named and stable for the contract |
| bytes | Owned by the dequeued result; never aliases queue or producer storage |
| status | Success, empty, lifecycle, capacity, or production failure as applicable |

At the C boundary, an opaque release context is allocated before destructive
dequeue. The owned pixel vector moves into it without a second pixel allocation;
resource failure therefore cannot advance `frames consumed` without transfer.

## AudioChunk

Represents an ordered bounded sequence of generated samples.

| Field | Rule |
| --- | --- |
| sequence/position | Emulated sample position, deterministic for identical input |
| sample format | Mono Float32 at 48,000 Hz |
| sample count | Never exceeds requested output capacity |
| samples | Owned C++/Swift copy or copied into C caller-provided storage |
| demand/pressure | Reports available, requested, target 2,048, exact shortfall, and overrun count |

## OutputQueue

Finite storage owned by the runtime owner. The frame FIFO has capacity three,
dequeues oldest-first, and drops the oldest unconsumed frame on overflow. The
audio FIFO has capacity 4,096 samples, target fill 2,048, and drops oldest
samples on overflow. Queue mutation happens only at the owner boundary;
consumers receive owned values or copies.

## OutputDiagnostics

One consistent observation containing total emulated cycles, latest frame
number, frame/audio depth and capacity, audio demand, dropped-frame count,
audio overrun/underrun counts, and the last recoverable output status. It is
observational and does not advance machine state.

## EmulationRateObservation

Two diagnostic snapshots plus a positive host-observation interval. Host code
calculates the ratio of emulated-seconds delta to host-seconds delta. Host time
is never retained by or passed into a core state transition.

## XcodeProjectSurface

Tracked project metadata and shared macOS, iOS Simulator, and test schemes that
reference existing package/core products. User-specific state, absolute paths,
signing identities, and derived data are excluded from version control. Ignored
local `xcuserdata` created during ordinary use is permitted and never deleted by
validation.

## State transitions

`Produced -> Queued -> Transferred/Consumed` is the normal path. A full queue
transitions a produced item through the documented overrun policy; an empty
consumer request returns `Empty` or `Underrun` without creating an invalid
view. Paused, faulted, and shutting-down runtime states reject or qualify
production and consumption through existing lifecycle statuses.
