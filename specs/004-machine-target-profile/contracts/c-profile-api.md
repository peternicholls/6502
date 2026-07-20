# C Machine-Profile API Contract

The C header declares fixed-width profile component and fixed-capacity profile
aggregates plus schema/capacity and known Model B/Model B+ 64K constants. It
does not expose C++ storage, flexible arrays or borrowed profile pointers.

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

All C++ exceptions remain contained. Profile input is copied before use; query
output is caller-owned plain data. Concurrent queries use normal live-handle
admission and runtime-owner ordering. Destroy/query lifetime rules remain those
of the existing C boundary.
