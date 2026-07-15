# Contract: Runtime State Machine

## Command matrix

| Command | Paused | Running | Faulted | Shutting down |
| --- | --- | --- | --- | --- |
| start | enter running | idempotent OK | invalid state | unavailable |
| pause | idempotent OK | enter paused at safe point | invalid state | unavailable |
| reset | reset, remain paused | reset at safe point, become paused | recover to paused | unavailable |
| bounded run / run-to-frame | execute FIFO, remain paused | invalid state | invalid state | unavailable |
| load/input | execute FIFO at safe point | execute FIFO at safe point | invalid state | unavailable |
| state/safe-point/fault/CPU/frame query | execute FIFO at safe point | execute FIFO at safe point | execute FIFO at safe point | unavailable |
| render audio | execute FIFO at safe point | execute FIFO at safe point | invalid state | unavailable |
| shutdown | drain prior accepted work, join | same | same | idempotent for in-flight destroy |

No command implicitly restores a previous running state. FIFO start/pause
commands express intent explicitly. Bounded execution is legal only while
paused and never changes sustained-execution intent.

## Safe point

A command completes only after a whole instruction/interrupt and its aggregate
device tick have completed. Later bus-cycle sequencing may change internals but
must expose the same boundary.

## Queue and destruction

At most 64 commands are incomplete. Submitters wait for capacity without
holding caller callbacks or machine state. Shutdown rejects new submissions,
finishes commands accepted before the shutdown marker, joins the owner, and
then frees the handle. Use after `beeb_destroy` returns is invalid.
