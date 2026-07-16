# Contributing

Beeb6502 keeps the emulation core deterministic, dependency-free and separate
from host UI and file access. Changes should preserve the boundaries described
in `docs/ARCHITECTURE.md` and advance a capability in either the core or product
roadmap. The two documentation strands and their authority are defined in
`docs/README.md`; legacy product documents are reference material, not active
specifications.

## Development checks

Run the checks relevant to every changed layer:

```sh
make test all
make sanitize
make test-c1
make thread-sanitize
make docs-check
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

## Code standards

- Use C++20 without third-party runtime dependencies in `BeebCore`.
- Keep host concerns behind the C ABI; never allow a C++ exception to cross it.
- Use `MachineRuntime`/the C ABI as the host boundary; concurrent calls are
  serialized by the runtime owner and must not access `BBCMicro` directly.
- Add a regression test before fixing an emulator correctness defect.
- Compile warning-free under `-Wall -Wextra -Wpedantic -Werror`.
- Follow `.clang-format` and `.editorconfig`; avoid formatting unrelated code.
- Document fidelity limits instead of implying unsupported cycle accuracy.
- Do not commit proprietary ROMs, character generators or user media.

Public API changes require a changelog entry. Version changes follow
`docs/RELEASING.md` and use the repository's Lore commit format.
