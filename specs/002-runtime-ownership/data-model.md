# Phase 1 Data Model: Phase C1 Runtime Ownership

## MachineRuntime

| Field | Rule |
| --- | --- |
| `machine` | Owner-thread-only `BBCMicro` |
| `state` | `paused`, `running`, `faulted`, `shutting_down` |
| `queue` | FIFO, maximum 64 accepted incomplete commands |
| `next_sequence` | Monotonic per-runtime acceptance identity |
| `owner` | One joinable execution context |
| `fault` | Empty except in `faulted`; cleared only by successful reset |

Transitions: `create -> paused`; `paused + start -> running`; `running + pause
-> paused`; `paused|running + execution failure -> faulted`; `faulted + reset
-> paused`; any live state + shutdown -> shutting_down -> destroyed`.

## RuntimeCommand

Kinds: `start`, `pause`, `reset`, `run_cycles`, `run_until_frame`,
`load_os_rom`, `load_sideways_rom`, `mount_disc`, `set_key`, `set_break`,
`runtime_state`, `cpu_state`, `frame`, `render_audio`, and `shutdown`. Each has
an acceptance sequence, copied payload, completion, and result. Mutable
transactions are atomic at one safe point. Observations return owned values.
Bounded execution is an explicit owner command rather than an approximation
using sustained start/pause transitions.

## ExecutionSlice

| Field | Rule |
| --- | --- |
| `sequence` | Shares the owner ledger order |
| `requested_cycles` | Fixed 2,048 for sustained execution |
| `actual_cycles` | Whole-instruction result, never less on success |
| `safe_point` | Cycle count plus completed-command sequence |

An execution slice is created only while running and only when no command is
pending at the selection point.

## SafePoint

Value identity: completed CPU cycle count, frame number, runtime state, and
latest completed ledger sequence. It is observable only after the instruction
and all aggregate device advancement complete. It is not a persisted snapshot.

## RuntimeStatus

Codes: `ok`, `invalid_argument`, `invalid_state`, `execution_failed`,
`resource_exhausted`, `unavailable`, `reentrant_call`, `internal_failure`.
The fixed message is an operation-scoped value; success has an empty message.
No borrowed shared diagnostic exists.

## Invariants

- Only the owner touches `BBCMicro`; synchronization protects queue/lifecycle,
  never shared direct machine access.
- Accepted queue sequence is total and monotonic; accepted commands complete
  once, FIFO, unless shutdown returns their documented unavailable result.
- Input buffers are copied before submission returns control to mutable caller
  storage; outputs are values or owned buffers.
- Reset/load validation failure leaves state unchanged. Execution failure is
  recorded at the last complete safe point.
- Destroy is safe against calls already inside the API, but the handle is
  invalid after destroy returns.
