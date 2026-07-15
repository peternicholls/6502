# The Host Boundary

The supported host path is `MachineRuntime` to structured C 0.2 to
`BeebMachine`. Each layer strengthens ownership and recovery without becoming a
second source of machine state or emulated time.

## C ABI

Every fallible C function returns one `beeb_status` by value. Its category maps
one-to-one to `RuntimeStatusCode`, its fixed diagnostic belongs to that
operation, and success is the sole status allowed to write a required
out-parameter. There is no shared last-error slot and no sentinel that must be
interpreted using a later call.

`beeb_create()` writes an opaque token only after both the runtime and the outer
handle state exist. The token is a registry key, not an object that entry points
dereference. `ActiveCall` looks it up under the registry mutex, retains the
shared `HandleState`, and increments the active-call count before touching
`MachineRuntime`. This indirection closes the raw-pointer race in which destroy
could otherwise free a mutex while another call was entering it.

The first `beeb_destroy()` caller marks the state as destroying, which makes new
entries return `unavailable`. It waits for admitted calls, invokes the runtime's
drain-and-join shutdown, removes the registry entry, and releases the token.
Concurrent destroy callers already inside wait for the same completion. A
caller must not use the pointer after destroy returns.

## Payloads and outputs

ROM and disc functions copy caller bytes into their runtime command before the
call completes. CPU, lifecycle, safe-point, and fault results are plain value
aggregates. Audio renders into a caller buffer only after validation and
successful owner completion.

`beeb_get_frame()` allocates a complete caller-owned RGBA copy. No completed
frame is a successful value with `available == 0`, null storage, and zero
metadata; it is not a failure. Every successful frame value is passed to
`beeb_frame_release()`, which releases its allocation and clears the aggregate.
A failed frame call leaves the caller's aggregate untouched.

No C++ exception crosses C. Adapter allocation failures become
`resource_exhausted`; contained standard or unknown failures become
`internal_failure`. Outputs remain untouched on every non-OK path.

## Swift wrapper

`BeebMachine` owns exactly one opaque C token and is `@unchecked Sendable`
because the underlying C runtime accepts concurrent calls. It deliberately has
no `NSLock`: adding another lock would hide the runtime's FIFO acceptance order
and duplicate lifecycle serialization. Each public method performs one
synchronous C operation and immediately copies its successful output.

`BeebStatusCategory` preserves the C category. `BeebError.coreStatus` retains
that category and the operation-owned diagnostic, while Swift-only validation
uses input-specific cases before crossing C. Lifecycle and fault values are
queried from the runtime rather than cached in Swift. Frame bytes are copied
into `Data` before the C frame is released; CPU, safe-point, fault, and audio
results are likewise independently owned Swift values.

Concurrent tasks retain the object through each method call. Deinitialization
runs only after the final strong reference is gone and performs blocking C
destroy. Public callbacks are not part of this boundary, so neither Swift nor C
invokes host code under runtime synchronization.

## Adding a boundary operation

Define the C contract first: legal lifecycle states, null behavior, copied input
ownership, success-only output writes, result lifetime, and side effects. Route
the implementation through one `MachineRuntime` command and contain every
exception. Then add a Swift value/error mapping without a host-side lock or
mirrored mutable state. Boundary tests must cover nulls, failure output
preservation, concurrent entry, and recovery before publishing the symbol.
