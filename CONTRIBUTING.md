# Contributing

Beeb6502 keeps the emulation core deterministic, dependency-free and separate
from host UI and file access. Changes should preserve the boundaries described
in `docs/ARCHITECTURE.md`.

## Development checks

Run the checks relevant to every changed layer:

```sh
make test all
swift test
swift build
```

`make test` treats warnings as errors and verifies that the compiled version,
`VERSION` and `CHANGELOG.md` agree. The clean-room smoke test is available with:

```sh
make demo-rom
.build/beeb-headless --os .build/cleanroom-demo.rom \
  --cycles 100000 --frame .build/cleanroom-demo.ppm
```

## Code standards

- Use C++20 without third-party runtime dependencies in `BeebCore`.
- Keep host concerns behind the C ABI; never allow a C++ exception to cross it.
- Serialize access to a `BeebMachine` instance at the host boundary.
- Add a regression test before fixing an emulator correctness defect.
- Compile warning-free under `-Wall -Wextra -Wpedantic -Werror`.
- Follow `.clang-format` and `.editorconfig`; avoid formatting unrelated code.
- Document fidelity limits instead of implying unsupported cycle accuracy.
- Do not commit proprietary ROMs, character generators or user media.

Public API changes require a changelog entry. Version changes follow
`docs/RELEASING.md` and use the repository's Lore commit format.
