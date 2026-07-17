# C2 C ABI Contract

New or changed C entry points return structured operation statuses and write
output only on success or a documented partial-success result. Frames are
caller-owned RGBA values whose opaque release context is allocated before a
destructive dequeue and released explicitly. Pixel-vector ownership moves from
the runtime result without a second post-dequeue allocation, so resource
failure leaves queue depth and consumed accounting unchanged. Audio is copied into
caller-provided Float32 storage and reports copied count plus exact shortfall.
No output aliases runtime queue or producer storage.

Diagnostics expose total emulated cycles, frame/audio depths and capacities,
audio demand, and exact pressure counters. A host helper accepts two snapshots
and a positive host interval to calculate emulation-rate ratio; host time never
enters core state. No C++ exception crosses the boundary, and empty, overrun,
underrun, paused, faulted, and shutting-down outcomes remain distinguishable.
After `beeb_reset`, dequeue reports empty and audio drain reports an empty
underrun until new emulated progress produces output. Diagnostics retain
monotonic identities/counters and include the reset-discarded depths in
frame-drop/audio-overrun accounting.
