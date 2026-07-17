# Research: Phase C2 Bounded Output Contracts

## Decision: Publish only at the C1 quiescent safe point

**Rationale**: C1 already defines the completed-instruction/device-tick boundary as the legal observation point. Publishing there prevents partial frames and half-updated audio/device state while preserving deterministic emulated time.

**Alternatives considered**: Publish per host callback (rejected because host scheduling would become authoritative); publish per CPU bus cycle (rejected because C4 is explicitly queued and would change the safe-point contract).

## Decision: Use owned output values at every public boundary

**Rationale**: A consumer can retain output while production continues without observing mutation. C++ dequeues an owned value; C receives a caller-owned frame released explicitly and audio copied into caller-provided storage; Swift copies into independently owned values before releasing C storage. No public result aliases queue or producer storage.

**Alternatives considered**: Return a pointer into the active renderer/audio buffer (rejected because the next production operation could invalidate it); require callers to lock the runtime (rejected because C1 makes synchronization an internal runtime responsibility).

## Decision: Use a three-frame FIFO with drop-oldest overflow

**Rationale**: Three retained frames provide current, next, and consumer-held progress without blocking the runtime. Consumers dequeue the oldest retained frame. When full, production discards exactly the oldest unconsumed frame, enqueues the newest, and increments a monotonic dropped-frame counter. This keeps latency bounded while preserving FIFO order among retained frames.

**Alternatives considered**: One latest-frame slot (rejected because it provides no bounded handoff slack); unbounded queues (rejected due to memory risk); blocking the owner (rejected because host scheduling must not stall emulation); silent discard (rejected because accounting must be recoverable).

## Decision: Use a 4,096-sample mono Float32 ring at 48 kHz

**Rationale**: The explicit format removes caller-selected-rate ambiguity from sustained production. A 4,096-sample ring bounds storage to about 85 ms, with a 2,048-sample target. Demand is `max(0, 2048 - available)`. Consumers drain FIFO samples; an underrun returns every available sample plus exact shortfall, while overflow discards oldest samples and increments an exact overrun count. Producer work never blocks on the host.

**Alternatives considered**: Preserve arbitrary synchronous render rates (rejected because it is not a sustained producer contract); block on a full ring (rejected because host callbacks would control runtime progress); discard newest samples (rejected because it retains stale latency).

## Decision: Preallocate the C frame release context before dequeue

**Rationale**: C must not report a frame as consumed until ownership reaches the
caller. A small opaque context is allocated before the runtime command; the
returned vector then moves into it without allocating or copying pixels. This
keeps allocation failure atomic across the C boundary and gives release one
explicit ownership token.

**Alternatives considered**: Peek then consume (rejected because separate owner
commands admit interleaving consumers); requeue after failure (rejected because
it cannot restore FIFO order after interleaved production); copy after dequeue
(rejected because allocation failure loses the consumed frame).

## Decision: Core diagnostics expose counters; hosts calculate emulation rate

**Rationale**: Total emulated cycles, frame number, queue depth/capacity/demand, and pressure counters explain core behavior. C and Swift host helpers calculate `emulated-seconds delta / positive host-seconds delta` from two snapshots. Host time is an explicit observation input and is never stored in or used to advance BeebCore.

**Alternatives considered**: Host timestamps as the source of progress (rejected by constitution and roadmap); sentinel values (rejected because C1 intentionally replaced ambiguous failure shapes).

## Decision: Reset discards retained output without resetting lifetime accounting

**Rationale**: Frames and samples produced before device reset are not current
output afterward. Reset empties both queues and the fractional audio remainder,
but frame/sample identities and counters remain monotonic for the runtime
lifetime. Retained values are counted as frame drops/audio overruns so the
published conservation equations remain exact at the zero-depth boundary.

**Alternatives considered**: Retain queued output (rejected because it exposes
pre-reset media as current); zero every counter/identity (rejected because it
breaks runtime-lifetime diagnostics and delta observers); add reset-only discard
counters (rejected because the pre-1.0 contract already has exact
non-consumption discard terms and a wider ABI expansion adds no recovery value).

## Decision: Commit a top-level Xcode project over existing sources

**Rationale**: `Beeb6502.xcodeproj` provides stable shared macOS app, iOS Simulator app, and test schemes from a clean checkout. It references the same local package/core sources and keeps `Package.swift` plus the Makefile as independent authorities. Shared project metadata is tracked; ignored local user data may exist during ordinary use, while signing identities and derived output are never tracked or required.

**Alternatives considered**: Continue opening `Package.swift` only (rejected because the requested delivery surface is an Xcode project); copy sources into a separate Apple tree (rejected because it creates divergent authorities); adopt Xcode Cloud/signing in C2 (rejected as distribution scope).

## Decision: No persisted output format in C2

**Rationale**: Frames/audio are transient bounded observations. Versioning belongs to any future persisted snapshot or export format, not this in-memory producer contract.

**Alternatives considered**: Serialize output chunks for later replay (rejected as scope expansion into C3/session continuity and evidence tooling).
