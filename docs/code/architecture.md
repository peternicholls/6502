# Core Code Architecture

The runtime core is a deterministic library. It owns emulated state and exposes
host boundaries; it does not own a UI, filesystem policy, event loop, clock, or
product workflow. Wider product direction lives under `docs/product/` and must
not leak into the core by convenience.

## Layers

1. `CPU6502` performs one complete processor transition against `Bus`.
2. `BBCMicro` implements that bus, owns RAM, ROMs, media, and devices, and
   advances devices from the CPU time consumed.
3. `beeb_c.h` converts exceptions and C++ lifetimes into an explicit C ABI.
4. `BeebKit` owns the C handle, maps typed results, and returns Swift-owned values.
5. Headless tools and tests drive those same supported boundaries to produce
   observable evidence.

The direction is one-way: host code depends on the ABI, the ABI depends on the
core, and no documentation or evidence tool enters the runtime dependency
graph.

## Ownership map

`BBCMicro` owns every device. `CPU6502` only borrows its `Bus`. Disc and ROM
loads copy caller bytes. `MachineRuntime` owns the `BBCMicro`; the C handle owns
that runtime, and Swift owns the opaque C handle.
References and spans returned by the C++ layer are borrowed views. C 0.2
operations instead document each output explicitly: scalar aggregates are
written to caller-provided storage, and `beeb_get_frame()` returns a caller-
owned allocation released with `beeb_frame_release()`.

For boundary details, continue with [The Host Boundary](host-boundary.md). For
device advancement, see [The Timing Model](timing-model.md). For observable
proof, see [Evidence and Testing](evidence-and-testing.md).

## Change rule

Keep public contract comments next to declarations. Put cross-component
relationships in these guides. Put a short rationale link next to non-obvious
timing, transition, and buffer code; do not narrate self-evident statements.
