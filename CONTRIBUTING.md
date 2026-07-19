# Contributing

Beeb6502 keeps the emulation core deterministic, dependency-free and separate
from host UI and file access. Changes should preserve the boundaries described
in `docs/ARCHITECTURE.md`. Every new feature must trace to a named slice or gate
in `docs/product/MACHINE_DELIVERY_PLAN.md`, the sole forward programme
authority. `docs/IMPLEMENTATION_CONSTRAINTS.md` supplies non-prioritizing
technical requirements. Completed evidence lives under `docs/completed/` and
`specs/completed/`; archived material is research only. Neither may add scope.

## Development checks

Run the checks relevant to every changed layer:

```sh
make test all
make sanitize
make verify-c0
make test-c1
make test-c2
make thread-sanitize
make docs-check
make format-check
swift test
swift build
```

`make test` treats warnings as errors and verifies that the compiled version,
`VERSION` and `CHANGELOG.md` agree. `make sanitize` runs the non-exhaustive test
set under AddressSanitizer and UndefinedBehaviorSanitizer on Linux, and under
UndefinedBehaviorSanitizer on Apple hosts where the dynamic AddressSanitizer
runtime is not reliable. Override `SANITIZERS` when validating another
toolchain. The clean-room smoke test is available with:

```sh
make demo-rom
.build/cpp/beeb-headless --os .build/cpp/cleanroom-demo.rom \
  --cycles 100000 --frame .build/cpp/cleanroom-demo.ppm
```

## Apple project checks

Open `Beeb6502.xcodeproj`, not `Package.swift`, for interactive Apple app work.
The committed shared schemes must remain usable without requiring `xcuserdata`,
signing credentials, absolute checkout paths, or pre-existing derived data.
Ignored local user state is permitted; tracked or unignored state is not. Verify
all three maintained Xcode entry points and the independent build paths with:

```sh
make test-c2-xcode
```

Linux CI runs `C2_REQUIRE_TSAN=1 make test-c2-portable`; this covers every C2
group except the Xcode-only contract and fails if the executable TSan runtime is
unavailable. Local unsupported TSan remains `N/A`, never a pass.

For user-facing work, automated checks are necessary but not sufficient. Build
and launch the maintained application, execute the feature's documented journey
on each claimed platform/device or simulator, and record the observed visual,
audio and interaction result. A passing unit suite alone cannot close product
acceptance.

Do not add package-owned `BeebCore` or `BeebKit` sources directly to Xcode
targets. The project consumes those products from the local package. Continue
to run `swift build`, `swift test`, and the relevant Make gates independently;
the Xcode project does not replace either build surface.

## Code standards

- Use C++20 without third-party runtime dependencies in `BeebCore`.
- Keep host concerns behind the C ABI; never allow a C++ exception to cross it.
- Use `MachineRuntime`/the C ABI as the host boundary. Hosts may call that
  boundary concurrently; the runtime owner establishes FIFO order, so hosts
  must not add a second serialization layer or access `BBCMicro` directly.
- Add a regression test before fixing an emulator correctness defect.
- Compile warning-free under `-Wall -Wextra -Wpedantic -Werror`.
- Follow `.clang-format` and `.editorconfig`; avoid formatting unrelated code.
- Document fidelity limits instead of implying unsupported cycle accuracy.
- Do not commit proprietary ROMs, character generators or user media.

Public API changes require a changelog entry. Version changes follow
`docs/RELEASING.md` and use the repository's Lore commit format.
