# Core Machine-Profile Contract

`MachineTargetProfile` is an immutable schema-versioned value consisting of one
raw base component and zero to sixteen raw expansion components. Each component
has a stable 32-bit identifier and explicit 16-bit version. Canonical values
sort expansions, reject duplicates and zero unused storage. Raw identifiers are
not a closed C++ enum.

The permanent version-1 base codes are `0x00000001` for BBC Microcomputer Model
B and `0x00000002` for BBC Model B+ 64K. No expansion code is assigned. Later
work may allocate new codes but cannot reuse or reinterpret these pairs.

Pure validation distinguishes malformed, unknown, incompatible,
recognised-unavailable and supported values in that precedence without
constructing or mutating a machine. Count 16 is structurally valid; count above
16 rejects before fixed expansion storage is inspected. Only canonical Model B
without expansions is supported for construction. Model B+ 64K is the only
recognised-unavailable identity. Unassigned future-option fixtures are unknown,
not hidden constants. No value falls back to Model B.

`BBCMicro` and `MachineRuntime` retain the successful profile for their entire
lifetime. A runtime profile query is serialized through the existing owner and
returns an owned value at a normal safe point. It neither advances emulated time
nor exposes a machine/device reference. Direct construction with any
non-supported value fails before machine state becomes observable.

The value is not a persisted byte format. Its semantic fields may feed a later
snapshot encoder, but this contract makes no promise about object layout,
padding, byte order, truncation or trailing bytes.
