# C2 C++ Contract

`MachineRuntime` remains the sole owner of `BBCMicro` and publishes output only
after a complete instruction and aggregate device tick. Frame and audio
producers transfer immutable or operation-owned values into finite queues.

Each queue defines capacity, empty/full behavior, overflow policy, and whether
the operation transfers ownership or returns an immutable value. Diagnostics
are snapshots and never mutate machine state. No consumer callback runs while
runtime synchronization is held, and allocation or conversion failure becomes
an operation-owned status.
