# Contract: C Runtime API 0.2

## Value shapes

`beeb_status` is returned by value and contains `beeb_status_code code` plus a
256-byte fixed UTF-8 diagnostic buffer. `BEEB_STATUS_OK` is the sole success
code and has an empty message. Outputs are written only on success. Status codes
map one-to-one to the eight `RuntimeStatusCode` categories.

`beeb_create(beeb_machine** out_machine)` returns status and writes the owned
opaque handle only on success; `beeb_destroy` performs blocking shutdown and
release. Runtime-state, fault-detail, start, pause, reset, bounded run, run-to-frame,
media/input, CPU/frame/audio functions all return `beeb_status`; values use
validated out-parameters. Bounded execution is accepted only while paused.

`beeb_frame` contains an explicit availability flag, dimensions, frame number,
an allocated RGBA pointer, and byte count. No completed frame is a successful
unavailable-value result with a null pointer and zero size. A successful
available frame is caller-owned and released with `beeb_frame_release`; failure
does not modify the caller's frame object. Audio remains a caller-provided
buffer that is written synchronously only after its pointer, count, and sample
rate are validated.

## Rules

- Null handle/output/payload violations return `invalid_argument` without dereference.
- No sentinel doubles as success-with-no-value and failure.
- Input is copied before it may outlive the call. Frame/audio/state outputs are caller-owned.
- Every call is synchronous to its safe-point completion and may be made from
  any thread except re-entrantly from the owner.
- `reentrant_call` is reserved for private owner-thread producers. Current C
  hosts cannot install an owner callback, but the runtime rejects the path
  deterministically so a future internal producer cannot deadlock.
- No exception crosses C. Unknown exceptions become `internal_failure`.
- `beeb_destroy` may overlap calls already inside the API and waits safely; no
  call is valid after destroy returns.

This intentionally replaces the 0.1 sentinel/`beeb_last_error` shapes. The
release is versioned 0.2.0 and all repository consumers migrate atomically.
