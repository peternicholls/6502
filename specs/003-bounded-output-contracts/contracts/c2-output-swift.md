# C2 Swift Contract

`BeebKit` maps successful output into Swift-owned values or explicitly scoped
views whose lifetime is documented. C statuses map to typed `BeebError`
categories, including empty output, pressure, lifecycle, and production
failure. The wrapper adds no redundant lock; `MachineRuntime` remains the
synchronization boundary.
