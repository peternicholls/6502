# C2 C ABI Contract

New or changed C entry points return structured operation statuses and write
output only on success or a documented partial-success result. Every frame or
audio view names its format, byte/sample count, ownership, lifetime, release or
copy rule, nullability, thread-safety, and lifecycle failures.

The ABI never exposes a pointer to concurrently mutated active storage and no
C++ exception crosses the boundary. Empty, overrun, underrun, paused, faulted,
and shutting-down outcomes remain distinguishable.
