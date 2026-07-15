# Contract: C Runtime API 0.2

## Value shapes

`beeb_status` is returned by value and contains `beeb_status_code code` plus a
fixed UTF-8 diagnostic buffer. `BEEB_STATUS_OK` is the sole success code and has
an empty message. Outputs are written only on success.

Lifecycle functions create/destroy the opaque handle and query runtime state.
Start, pause, reset, run-budget, media/input, CPU/frame/audio, and state-query
functions all return `beeb_status`; values use validated out-parameters.

## Rules

- Null handle/output/payload violations return `invalid_argument` without dereference.
- No sentinel doubles as success-with-no-value and failure.
- Input is copied before it may outlive the call. Frame/audio/state outputs are caller-owned.
- Every call is synchronous to its safe-point completion and may be made from
  any thread except re-entrantly from the owner.
- No exception crosses C. Unknown exceptions become `internal_failure`.
- `beeb_destroy` may overlap calls already inside the API and waits safely; no
  call is valid after destroy returns.

This intentionally replaces the 0.1 sentinel/`beeb_last_error` shapes. The
release is versioned 0.2.0 and all repository consumers migrate atomically.
