# Runtime Ownership {#runtime_ownership}

`MachineRuntime` is the supported C++ synchronization boundary for one
`BBCMicro`. The runtime constructs the machine on one owner thread and never
returns a machine, device, frame-buffer, or sound-generator reference. Host
threads exchange copied commands and owned results with that owner.

Direct `BBCMicro` construction remains available only to low-level,
single-threaded core tests. The C and Swift hosts now use this runtime through
the completed, versioned C API 0.2 boundary; no supported host path accesses
the machine directly. This guide describes the implemented host contract, not
a deferred migration design.

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

## Transaction matrix

Every row below enters the same FIFO. "Preserve" means the command does not
invent a lifecycle transition: if it is ordered while running, sustained
execution continues only after the command completes; if it is ordered while
paused, it remains paused.

| Command family | Paused | Running | Faulted | Ownership and effect |
| --- | --- | --- | --- | --- |
| `start` / `pause` | Start runs; pause is idempotent | Start is idempotent; pause stops before another slice | Rejected | Empty payload; explicit lifecycle intent only |
| `reset` | Reset and remain paused | Reset and become paused | Clear fault and become paused | Machine reset is atomic at the command safe point |
| `runFor` / `runUntilFrame` | Execute the requested bounded work | Rejected | Rejected | Scalar budget; owned scalar result |
| OS ROM, sideways ROM, disc | Install and preserve state | Install and preserve state | Rejected | Caller bytes are copied before acceptance; invalid media leaves the prior installation unchanged |
| keyboard / BREAK | Mutate and preserve state | Mutate and preserve state | Rejected | Value payload; BREAK may reset `BBCMicro` without inventing a runtime pause/start |
| runtime state / safe point / CPU / frame | Observe | Observe between slices | Observe | Owned values; frame storage never aliases the machine |
| fault detail | Observe empty detail | Observe empty detail | Observe retained failure | Owned diagnostic and safe point |
| audio render | Mutate sound phase and preserve state | Mutate sound phase between slices and preserve state | Rejected | Owned sample vector; finite positive sample rate |
| completed-frame dequeue / continuous-audio drain | Transfer oldest retained output | Transfer between slices | Rejected | Owned frame or copied FIFO samples; structured empty/underrun/overrun pressure |
| output diagnostics | Observe | Observe between slices | Observe | One owned, non-mutating snapshot of progress, depths, demand, counters, and latest output status |
| shutdown | Stop acceptance and drain | Stop acceptance and drain | Stop acceptance and drain | One ordered marker, one owner join |

Payload validation that depends on current machine state occurs on the owner.
Copy allocation can fail before acceptance with `resourceExhausted`; once a
command has an acceptance identity, it receives exactly one completion. A
failed transaction does not restore an earlier running state or perform a
hidden resume; only later accepted `start` or `pause` commands express that
intent.

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

C2 production uses that same boundary. After execution advances whole
instructions and their aggregate device ticks, the owner publishes any newly
completed RGBA frame and converts the retained CPU-cycle delta to continuous
48 kHz mono samples. The owner alone mutates the capacity-three frame FIFO,
capacity-4,096 audio ring, their counters, and the fractional `3 / 125`
cycle-to-sample remainder. Dequeue, drain, and diagnostic commands therefore
join the existing FIFO instead of acquiring a producer lock or calling a
device directly. Reset changes machine state but does not erase runtime-lifetime
output identities, retained values, or monotonic accounting.

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

An emulated execution exception is caught on the owner after the failing
instruction has restored its processor-local pre-instruction state. Every
earlier whole instruction and its completed aggregate device tick remain
committed, so the fault safe point is the last completed instruction/device
boundary and ledger `actualCycles` reports that retained work. The runtime then
enters `faulted` and returns `executionFailed`. Allocation or unexpected
implementation failures instead restore the process-local command checkpoint,
covering CPU, RAM, devices, mounted media state, frame storage, keyboard state,
and timing remainders, and report their non-execution category. Later fault
queries return an owned diagnostic and safe point. State, fault, CPU, and frame
queries remain legal. Start, pause, bounded execution, media, input, and audio
mutation are rejected until reset succeeds.

`CPU6502::step()` applies the same rule to processor-local failures. A trace
observer runs after opcode fetch but before instruction execution or device
time; if it throws, the pre-fetch CPU boundary is restored and the exception is
transported to the caller. Sustained-execution tests use an explicit closed JMP
loop so a lifecycle test can never fault merely by falling out of its fixture.
The headless tool exposes this observer only in standalone functional CPU mode;
BBC-mode execution through `MachineRuntime` rejects `--trace`.

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

Capacity includes the command currently executing. A shutdown racing a full
queue therefore wakes capacity waiters before it waits for one slot for its
marker. Rejected waiters have no acceptance identity; every nonzero accepted
identity precedes the marker and completes before owner exit. The machine is
released only after the owner has stopped dereferencing it.

The owner cannot invoke shutdown because it cannot join itself. Destruction
calls the same idempotent drain-and-join path. The C handle adds a separate outer
lifetime guard so `beeb_destroy` can overlap calls already inside the C API
without releasing the runtime early.

## Diagnostic replay and output observations

Full ledger capture is disabled by default. Tests may opt in to an in-memory
ledger containing one total sequence across accepted commands and internal
execution slices, plus requested/actual cycles, payload and result digests,
status, and resulting safe point. Test safe-point entries carry a process-local
whole-machine digest covering deterministic CPU, RAM, device, media, frame, and
timing values. The C1 replay tests compare CPU state, safe point, exact ledger,
and that machine digest across ten runs, and write a disposable text view under
ignored `.build/c1/` storage.

The ledger is evidence, not a persisted emulator format. C3 owns snapshots and
versioned persistence. New result variants must extend the result digest, and
new command payloads must preserve copied ownership and deterministic digesting.

Output diagnostics are a smaller always-available observation, not the opt-in
ledger. One owner command copies cycles, latest published output identity,
queue depths and capacities, audio demand, exact counters, and latest output
status without mutating them. Host elapsed time is accepted only by the pure
rate calculation over two such values; it is never stored by the owner or used
to schedule production.

## Future timing work

Later bus-cycle sequencing may replace instruction-level internals, but it must
retain the externally observable completed-instruction/device-tick safe point
until a separately specified boundary migration supplies new evidence. It must
also preserve FIFO ownership: device-level timing work is not permission to
add host access or locks inside individual devices.
