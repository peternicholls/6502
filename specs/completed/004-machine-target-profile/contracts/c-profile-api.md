# C Machine-Profile API Contract

The C header declares fixed-width profile component and fixed-capacity profile
aggregates plus schema/capacity and known Model B/Model B+ 64K constants. It
does not expose C++ storage, flexible arrays or borrowed profile pointers.
The header publishes exact base codes `0x00000001` for Model B and `0x00000002`
for Model B+ 64K, both at component version 1, and publishes no expansion code.

Public operations provide canonical Model B and Model B+ 64K values, validate a
caller value, create a machine from an explicit profile, and copy the active
profile from a live machine. Existing `beeb_create()` remains an explicit Model
B convenience routed through the same designated construction path.

Every fallible operation returns `beeb_status`. Null, malformed, unknown,
incompatible and recognised-unavailable inputs have distinguishable stable
categories or support results and operation-owned diagnostics. Output
aggregates are written only on success. Explicit invalid or unsupported input
never invokes default construction, never registers a handle and never changes
an existing machine.

Validation precedence is malformed, unknown, incompatible, then
recognised-unavailable. A count above sixteen is malformed before any expansion
slot is read; an exact count of sixteen is safe to classify. Failed calls leave
every byte of caller output unchanged. The aggregate is an in-process C value,
not a serialized ABI or snapshot byte layout.

All C++ exceptions remain contained. Profile input is copied before use; query
output is caller-owned plain data. Concurrent queries use normal live-handle
admission and runtime-owner ordering. Destroy/query lifetime rules remain those
of the existing C boundary.
