# The Host Boundary

The supported host path is C++ core to C ABI to Swift wrapper. Each layer makes
ownership and failure more explicit without changing emulated behavior.

## C ABI

`beeb_create()` returns an owned opaque handle and `beeb_destroy()` releases it.
The C API does not synchronize a handle; one caller must serialize operations.
No C++ exception crosses the boundary. Operations clear the previous error,
catch failures, and store a fixed-size diagnostic retrievable through
`beeb_last_error()`.

ROM and disc functions copy input bytes. `beeb_get_cpu_state()` returns a value.
`beeb_get_frame_rgba()` instead returns a machine-owned pointer: its width,
height, and frame number describe the same buffer, and the pointer is valid only
until the next machine mutation or destruction.

## Swift wrapper

`BeebMachine` owns exactly one C handle. Its internal lock serializes all public
operations, which is the basis for its `@unchecked Sendable` conformance. Input
`Data` is borrowed only during the C call; the core copies accepted media.

The frame wrapper computes `width * height * 4` while holding the lock and copies
the complete borrowed buffer into a new `Data` before unlocking. CPU state and
audio are likewise returned as Swift-owned values. Validation errors use
specific `BeebError` cases; core diagnostics become `coreFailure`.

## Adding a boundary operation

Define the C contract first: null behavior, ownership, lifetime, error channel,
threading, and side effects. Lock and map it in Swift, copying borrowed output
before unlocking. Add C boundary tests and Swift error/lifetime tests before
publishing the symbol.
