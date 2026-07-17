# C2 C++ Contract

`MachineRuntime` remains the sole owner of `BBCMicro` and publishes output only
after a complete instruction and aggregate device tick. The frame FIFO retains
three immutable owned RGBA values, dequeues oldest-first, and drops the oldest
unconsumed value on overflow. The audio ring retains 4,096 mono Float32 samples
at 48,000 Hz, targets 2,048 available samples, and drops oldest samples on
overflow.

Dequeue transfers an owned C++ value; no result aliases queue storage.
Diagnostics report total cycles, depths/capacities, demand, and exact pressure
counters. They never mutate machine state. No consumer callback runs while
runtime synchronization is held, and allocation or conversion failure becomes
an operation-owned status.
