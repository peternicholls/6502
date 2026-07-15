# Releasing Beeb6502

Beeb6502 uses Semantic Versioning. While the major version is zero, minor
versions may change source-level APIs; every breaking change must be called out
in `CHANGELOG.md`. Patch versions remain backward compatible.

The version has three synchronized representations:

- `VERSION`, used by release tooling;
- `BEEB_VERSION_*` in `Sources/BeebCore/include/beeb/version.h`, used by hosts;
- the matching release heading in `CHANGELOG.md`.

`make check-version` verifies these representations through the public
`beeb-headless --version` command.

## Release checklist

1. Move completed entries from `Unreleased` into a dated version section.
2. Update `VERSION` and `beeb/version.h` to the same semantic version.
3. Run `make clean`, then `make test all demo-rom`.
4. Run `swift test` and `swift build` on a supported Apple host.
5. Run the clean-room demo and inspect its generated frame.
6. Commit using the repository's Lore commit format.
7. Create an annotated `vMAJOR.MINOR.PATCH` tag and publish release notes from
   the changelog section.
8. Add a fresh empty `Unreleased` section for subsequent work.

Never attach proprietary Acorn ROMs or user disc images to a release.
