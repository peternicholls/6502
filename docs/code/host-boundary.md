# The Host Boundary

The supported host path is `MachineRuntime` to structured C 0.2 to
`BeebMachine`. Each layer strengthens ownership and recovery without becoming a
second source of machine state or emulated time.

## C ABI

Every fallible C function returns one `beeb_status` by value. Its category maps
to the runtime or bounded-output category, and its fixed diagnostic belongs to
that operation. `OK` is normally the sole status allowed to write a required
out-parameter. The documented exception is `UNDERRUN`: `beeb_drain_audio()`
writes every valid partial sample plus exact copied, shortfall, demand, overrun,
and underrun accounting. Other non-OK paths preserve caller outputs. There is
no shared last-error slot and no sentinel that must be interpreted using a
later call.

`beeb_create_with_profile()` copies and validates an explicit profile before it
publishes anything. Only successful Model B construction writes and registers
an opaque token. `beeb_create()` is a deliberate canonical Model B convenience
routed through that same path; invalid explicit input never reaches it as a
fallback. `beeb_get_machine_profile()` copies runtime-owner truth into a complete
caller value only after the serialized query succeeds.

Profile validation is a successful observational operation even when its owned
result classifies the value as malformed, unknown, incompatible or recognised
but unavailable. Construction maps the first three classifications to
`INVALID_ARGUMENT` and Model B+ 64K to `UNAVAILABLE`; every rejection preserves
the caller's handle output byte for byte. The identity assignments, bounded
envelope, precedence and future-option non-claims are owned by
[Machine Target Profiles](target-profile.md).

The token is published only after both the runtime and outer handle state exist.
It is a registry key, not an object that entry points dereference. `ActiveCall`
looks it up under the registry mutex, retains the shared `HandleState`, and
increments the active-call count before touching `MachineRuntime`. This
indirection closes the raw-pointer race in which destroy could otherwise free a
mutex while another call was entering it.

The first `beeb_destroy()` caller marks the state as destroying, which makes new
entries return `unavailable`. It waits for admitted calls, invokes the runtime's
drain-and-join shutdown, removes the registry entry, and releases the token.
Concurrent destroy callers already inside wait for the same completion. A
caller must not use the pointer after destroy returns.

## Payloads and outputs

ROM and disc functions copy caller bytes into their runtime command before the
call completes. Machine profile, CPU, lifecycle, safe-point, and fault results
are plain value aggregates. The profile is an in-process semantic value, not a
persisted byte layout. Audio renders into a caller buffer only after validation
and successful owner completion.

`beeb_get_frame()` returns a complete caller-owned RGBA value. No completed
frame is a successful value with `available == 0`, null storage, and zero
metadata; it is not a failure. Every successful frame carries an opaque
`release_context` passed back unchanged to `beeb_frame_release()`, which releases
the vector storage and clears the aggregate. Callers read but never free `rgba`
directly. A failed frame call leaves the caller's aggregate untouched.

The maintained macOS demo keeps workflow state in the host model: OS and
language imports are typed roles, the language ROM uses fixed sideways bank 12,
physical keys are translated into owner-serialized `setKey` calls, and display
refresh consumes owned completed frames. Run, Pause, Reset and BREAK remain
separate host actions; reset and BREAK invalidate the presentation epoch before
later output is shown. This path is automated and does not claim ROM-backed
visual acceptance until the named-host observation is recorded.

`beeb_dequeue_frame()` allocates its small release context before it asks the
owner to consume the oldest frame from the capacity-three FIFO. The returned
pixel vector moves into that context without a second allocation. Allocation
failure and `EMPTY` both leave the aggregate and FIFO accounting untouched.
`beeb_drain_audio()` copies FIFO samples into caller storage and reports
recoverable `UNDERRUN` with valid partial output. `beeb_get_output_diagnostics()`
copies one owner-consistent, non-mutating value containing depths, capacities,
demand, exact flow counters, and latest output status. The standalone
`beeb_calculate_emulation_rate()` helper compares two such values and one host
interval without touching a machine or storing host time.

After reset, C and Swift consumers cannot receive retained pre-reset media:
frame dequeue is empty and audio drain produces an empty typed underrun until
new emulated execution publishes output. Diagnostics keep runtime-lifetime
identities and include the discarded reset depths in exact frame-drop and
audio-overrun accounting.

No C++ exception crosses C. Adapter allocation failures become
`resource_exhausted`; contained standard or unknown failures become
`internal_failure`. Outputs remain untouched on every non-OK path except the
documented valid partial audio drain returned with `UNDERRUN`.

## Swift wrapper

`BeebMachine` owns exactly one opaque C token and is `@unchecked Sendable`
because the underlying C runtime accepts concurrent calls. It deliberately has
no `NSLock`: adding another lock would hide the runtime's FIFO acceptance order
and duplicate lifecycle serialization. Each public method performs one
synchronous C operation and immediately copies its successful output.

`BeebStatusCategory` preserves the C category. `BeebError.coreStatus` retains
that category and the operation-owned diagnostic, while Swift-only validation
uses input-specific malformed, unknown, incompatible and recognised-unavailable
cases before construction. Each profile error owns the original raw candidate
and copied diagnostic. Lifecycle and fault values are queried from the runtime
rather than cached in Swift. Frame bytes are copied into `Data` before the C
frame is released; CPU, safe-point, fault, and audio results are likewise
independently owned Swift values.

The continuous-output mappings preserve recoverable pressure instead of
flattening it into generic failure. Empty frame dequeue returns no value; an
audio underrun throws `BeebError.audioPressure` carrying the valid owned
`BeebAudioDrain`; diagnostic observations are owned `Sendable` values. Swift's
emulation-rate helper delegates to the pure C calculation and does not create a
timer or production loop.

Concurrent tasks retain the object through each method call. Deinitialization
runs only after the final strong reference is gone and performs blocking C
destroy. Public callbacks are not part of this boundary, so neither Swift nor C
invokes host code under runtime synchronization.

Supported hosts may enter C 0.2 concurrently. They must not add an outer lock
or dispatch queue to impose a competing order: the runtime owner's acceptance
FIFO is the sole serialization authority. Command-line instruction tracing is
separate from this boundary and is available only in standalone functional CPU
mode, never BBC runtime mode.

## Adding a boundary operation

Define the C contract first: legal lifecycle states, null behavior, copied input
ownership, success-only output writes, result lifetime, and side effects. Route
the implementation through one `MachineRuntime` command and contain every
exception. Then add a Swift value/error mapping without a host-side lock or
mirrored mutable state. Boundary tests must cover nulls, failure output
preservation, concurrent entry, and recovery before publishing the symbol.
If a recoverable non-OK status carries valid output, document that exception
explicitly at every language layer and test both the partial value and the
untouched-output cases.
