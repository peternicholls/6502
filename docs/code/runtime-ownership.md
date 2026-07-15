# Runtime Ownership {#runtime_ownership}

`MachineRuntime` is the supported C++ synchronization boundary for one
`BBCMicro`. The runtime constructs the machine on one owner thread and never
returns a machine, device, frame-buffer, or sound-generator reference. Host
threads exchange copied commands and owned results with that owner.

Direct `BBCMicro` construction remains available only to low-level,
single-threaded core tests. The C and Swift hosts migrate atomically to this
runtime with the C API 0.2 boundary later in C1.

## Lifecycle

```text
create ──> paused ──start──> running
             ^                 │
             └─────pause───────┘
             ^                 │
             └─────reset───────┘

paused|running ──execution failure──> faulted ──reset──> paused
paused|running|faulted ──shutdown──> shutting down ──join──> destroyed
```

A new runtime is paused. `start` while running and `pause` while paused are
ordered idempotent successes. Reset is the only recovery from `faulted`; it
clears the stored fault and finishes paused. No media or input command silently
restores an earlier running state.

State-dependent validation occurs on the owner after FIFO acceptance. Checking
state on a caller thread would let a concurrent reset/start pair be rejected
from a stale observation instead of the accepted order.

## Acceptance, completion, and back-pressure

One queue node owns the command kind, copied payload, acceptance identity, and
caller-specific promise. Keeping those together prevents an allocation failure
from pairing a command with another caller's completion.

At most 64 accepted commands are incomplete, including a command executing on
the owner. A submitter waits for capacity without holding machine state. Space
is released only after the previous caller's promise is fulfilled. Re-entrant
submission from the owner is rejected because waiting on its own promise would
deadlock.

FIFO order is acceptance order under the queue mutex, not operating-system
thread launch order. Status messages and results are operation-owned values, so
concurrent callers never inspect a shared last-error slot.

## Safe point and execution arbitration

The quiescent safe point is immediately after `CPU6502::step()` completes an
instruction or interrupt and `BBCMicro::tick()` has advanced every aggregate
device by that instruction's cycles. `SafePoint` identifies CPU cycles, frame
number, lifecycle state, and the latest total ledger sequence at that boundary.

While running, the owner selects a minimum 2,048-cycle execution slice only
when no command is queued. `BBCMicro::runFor()` finishes whole instructions, so
actual cycles may exceed 2,048 by one instruction. The FIFO is checked again
after every slice. A command accepted just after selection can therefore wait
for that one slice, but the owner never starts a second slice while queued work
exists.

Bounded `runFor` and `runUntilFrame` are separate commands legal only while
paused. They are not implemented as `start` followed by `pause`, which would
lose the requested budget and make the result scheduler-dependent. Host wall
time is used only by tests as a deadlock guard and never changes emulated state.

## Fault containment

An execution exception is caught on the owner, the CPU is restored to the last
completed state captured before the failing run, and the runtime enters
`faulted`. The operation receives `executionFailed`; later fault queries return
an owned copy of the same diagnostic and safe point. State, fault, CPU, and
frame queries remain legal. Start, pause, bounded execution, media, input, and
audio mutation are rejected until reset succeeds.

Every owner command has an exception boundary. Allocation failure becomes
`resourceExhausted`; other known and unknown failures become operation-scoped
status values. A caller promise is fulfilled exactly once even when the caller
abandons its future.

## Shutdown

The first shutdown caller atomically stops new acceptance. Submitters not yet
accepted wake with `unavailable`; commands already accepted remain in FIFO and
complete. Once capacity permits, the shutdown marker receives the next
acceptance identity. The owner processes it only after the queue drains, enters
`shuttingDown`, records the final safe point, and exits. One caller joins while
concurrent shutdown callers wait for that same completion.

The owner cannot invoke shutdown because it cannot join itself. Destruction
calls the same idempotent drain-and-join path. The later C handle adds a separate
outer lifetime guard so `beeb_destroy` can overlap calls already inside the C
API without releasing the runtime early.

## Diagnostic replay

Full ledger capture is disabled by default. Tests may opt in to an in-memory
ledger containing one total sequence across accepted commands and internal
execution slices, plus requested/actual cycles, payload and result digests,
status, and resulting safe point. The C1 replay test captures a concurrent
interleaving, replays that exact ledger ten times, and writes a disposable text
view under ignored `.build/c1/` storage.

The ledger is evidence, not a persisted emulator format. C3 owns snapshots and
versioned persistence. New result variants must extend the result digest, and
new command payloads must preserve copied ownership and deterministic digesting.

## Future timing work

Later bus-cycle sequencing may replace instruction-level internals, but it must
retain the externally observable completed-instruction/device-tick safe point
until a separately specified boundary migration supplies new evidence. It must
also preserve FIFO ownership: device-level timing work is not permission to
add host access or locks inside individual devices.
