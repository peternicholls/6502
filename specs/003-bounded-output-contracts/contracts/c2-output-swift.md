# C2 Swift Contract

`BeebKit` maps successful frame and audio output into independently owned Swift
values before releasing C storage. C statuses map to typed `BeebError`
categories, including empty output, pressure, lifecycle, and production
failure. Diagnostics preserve exact depths, demand, and counters. A pure host
helper calculates emulation-rate ratio from two snapshots and a positive
duration. The wrapper adds no redundant lock; `MachineRuntime` remains the
synchronization boundary.
Reset never exposes retained pre-reset output: video dequeue is empty and audio
drain carries an empty underrun until new production. Swift diagnostics preserve
the core's monotonic identities and exact reset-discard accounting.
