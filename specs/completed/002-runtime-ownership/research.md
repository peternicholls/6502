# Phase 0 Research: Phase C1 Runtime Ownership

## Decision 1: Owner-thread runtime wrapper

**Decision**: `MachineRuntime` owns `BBCMicro` and one `std::jthread`; only its
owner loop dereferences the machine. Hosts submit value commands.

**Rationale**: Ownership is structural and auditable. Device classes remain
dependency-light and unsynchronized.

**Alternatives**: A recursive/global mutex hides ordering and permits accidental
direct access; an actor library adds a dependency; Swift-only ownership leaves C
and C++ hosts unsafe.

## Decision 2: Bounded synchronous FIFO

**Decision**: Capacity is 64 accepted-but-incomplete commands. Submitters wait
for space and then for their operation-scoped result; shutdown wakes all waiters.
Re-entrant submission from the owner is rejected.

**Rationale**: The API applies back-pressure, cannot silently drop work, and
gives each caller its own result/diagnostic.

**Alternatives**: An unbounded queue violates C1; drop/coalesce changes command
meaning; callbacks create re-entrancy and lifetime hazards.

## Decision 3: Safe point and running slices

**Decision**: The safe point is immediately after `CPU6502::step()` returns and
its `Bus::tick(cycles)` has advanced all devices. Sustained running performs a
minimum 2,048-cycle whole-instruction slice only when no command is pending,
then checks FIFO again.

**Rationale**: This names the current invariant, bounds control latency, and
leaves future bus-cycle internals free while preserving the public boundary.

**Alternatives**: Per-cycle polling prematurely implements C4; whole-frame
slices make pause latency workload-dependent; wall-clock slices violate
deterministic core authority.

## Decision 4: Replay an execution ledger

**Decision**: Tests record owner-accepted commands and internal execution slices
with sequence, requested/actual cycles, and resulting safe-point identity.
Determinism means replaying that ledger from the same fixture is exact.

**Rationale**: OS scheduling may change when a host request arrives relative to
a running slice. Recording the interleaving makes this explicit host input
rather than pretending command names alone determine elapsed emulated time.

**Alternatives**: Command-only replay is underspecified; wall-clock timestamps
are non-portable; stopping execution for every enqueue defeats sustained run.

## Decision 5: Lifecycle and transaction policy

**Decision**: States are `paused`, `running`, `faulted`, `shutting_down`.
Create/reset end paused. Start/pause are idempotent. All other operations are
FIFO safe-point transactions. Reset is the only recovery from fault and ends
paused. Shutdown rejects new calls, completes commands accepted before shutdown,
then joins. A caller must not use the handle after destroy returns.

**Rationale**: Four states cover legal behavior without speculative stopped or
snapshot states. FIFO ordering, not hidden auto-resume, determines intent.

## Decision 6: Structured 0.2 boundary

**Decision**: Revise the pre-1.0 C API so fallible functions return `beeb_status`
by value and write successful values through validated out-parameters. Status
contains a stable enum and fixed operation-owned message. `beeb_create` returns
status plus handle output. Swift maps code and message into typed `BeebError`.
Update all in-repo consumers and version/changelog to 0.2.0 at C1 completion.

**Rationale**: Per-machine `last_error` races and sentinel overloads cannot meet
the phase contract. A deliberate SemVer minor pre-1.0 migration is smaller and
safer than maintaining two public APIs indefinitely.

**Alternatives**: Thread-local errors separate status from the operation;
heap-owned messages complicate lifetime; compatibility shims preserve the very
ambiguous fallible entry points C1 must remove.

## Decision 7: Race evidence

**Decision**: Add a dedicated C++ ThreadSanitizer target on supported toolchains,
10,000-command mixed stress, timeout/deadlock detection, replay comparison, and
Swift task-group tests. Keep existing UB/ASan profiles separate.

**Rationale**: Functional locks do not prove ownership. Focused race evidence
is required without weakening existing sanitizer coverage.

## Decision 8: Documentation and release governance

**Decision**: Add `runtime-ownership.md`; update declaration contracts,
architecture/host/timing guides, status/roadmap/changelog/version, and run the
existing generated-doc debt ratchet.

**Rationale**: The threading and ABI migration are public architectural facts,
and C1 is not complete until the project authorities reflect verified delivery.
