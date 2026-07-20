# Core Machine-Profile Contract

`MachineTargetProfile` is an immutable schema-versioned value consisting of one
raw base component and zero to sixteen raw expansion components. Each component
has a stable 32-bit identifier and explicit 16-bit version. Canonical values
sort expansions, reject duplicates and zero unused storage. Raw identifiers are
not a closed C++ enum.

Pure validation distinguishes supported, recognised-unavailable, unknown,
incompatible and malformed values without constructing or mutating a machine.
Only canonical Model B without expansions is supported for construction in this
slice. Model B+ 64K is recognised but unavailable. No other value falls back to
Model B.

`BBCMicro` and `MachineRuntime` retain the successful profile for their entire
lifetime. A runtime profile query is serialized through the existing owner and
returns an owned value at a normal safe point. It neither advances emulated time
nor exposes a machine/device reference. Direct construction with any
non-supported value fails before machine state becomes observable.
