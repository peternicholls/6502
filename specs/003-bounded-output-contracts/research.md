# Research: Phase C2 Bounded Output Contracts

## Decision: Publish only at the C1 quiescent safe point

**Rationale**: C1 already defines the completed-instruction/device-tick boundary as the legal observation point. Publishing there prevents partial frames and half-updated audio/device state while preserving deterministic emulated time.

**Alternatives considered**: Publish per host callback (rejected because host scheduling would become authoritative); publish per CPU bus cycle (rejected because C4 is explicitly queued and would change the safe-point contract).

## Decision: Use immutable or operation-owned output values

**Rationale**: A consumer can retain a frame or audio result while production continues without observing mutation. Results can be copied at the ABI/Swift boundary where needed; the core owns queue storage until transfer or release.

**Alternatives considered**: Return a pointer into the active renderer/audio buffer (rejected because the next production operation could invalidate it); require callers to lock the runtime (rejected because C1 makes synchronization an internal runtime responsibility).

## Decision: Make capacity explicit and policy deterministic

**Rationale**: Each output type has a configured finite capacity, observable empty/full state, and a documented policy. The initial policy is newest-completed-frame retention for video and bounded sample production with explicit discard/shortfall status for audio, subject to contract tests and measured rationale.

**Alternatives considered**: Unbounded queues (rejected due to memory risk); blocking the owner until a consumer catches up (rejected because UI/audio scheduling must not stall emulation); silently discard without diagnostics (rejected because hosts need recovery information).

## Decision: Diagnostics are emulated-counter observations plus operation status

**Rationale**: Frame number, emulated sample position, queue availability/demand, and pressure counters explain behavior without using host wall-clock time. Operation-scoped statuses preserve C1’s structured failure model.

**Alternatives considered**: Host timestamps as the source of progress (rejected by constitution and roadmap); sentinel values (rejected because C1 intentionally replaced ambiguous failure shapes).

## Decision: No persisted output format in C2

**Rationale**: Frames/audio are transient bounded observations. Versioning belongs to any future persisted snapshot or export format, not this in-memory producer contract.

**Alternatives considered**: Serialize output chunks for later replay (rejected as scope expansion into C3/session continuity and evidence tooling).
