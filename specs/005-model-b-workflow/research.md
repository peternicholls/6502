# Research: Running Model B Workflow

## Decision 1: Treat MOS OS and language ROMs as distinct Model B roles

**Decision**: Require an exactly 16 KiB Model B MOS OS image for the fixed OS
region and one non-empty sideways ROM image up to 16 KiB for the user-selected
language role. The acceptance observation uses lawful, externally supplied
Model B OS and BASIC II-compatible language ROMs; no ROM bytes enter the repo.

**Rationale**: The verified wrapper already enforces these bounds. The Model B
service documentation describes a 16 KiB MOS and language/service ROMs in paged
space. Separate roles produce actionable recovery and stop file names from
inferring profile identity or silently selecting another language.

**Compatibility boundary**: File shape is necessary but not a claim of ROM
identity or provenance. The product accepts the stated role bounds, then direct
macOS acceptance proves the selected OS/language pair by resetting to BASIC and
running `10 PRINT "BEEB6502"` followed by `RUN`. Automated checks use only
synthetic or clean-room data and prove validation and state preservation, not
that a fixture is authentic firmware.

**Alternatives considered**:

- Accept any binary and infer its role from extension or contents — rejected:
  ambiguous and unsafe.
- Bundle a synthetic or proprietary BASIC image — rejected: it cannot prove the
  user-owned firmware journey and proprietary bytes cannot ship.
- Implement B+ firmware rules — rejected: B+ is a later reference-led feature.

**Sources**: [Acorn BBC Microcomputer Service Manual](https://acorn.huininga.nl/pub/docs/manuals/Acorn/BBC%20B/BBC%20Microcomputer%20Service%20Manual.pdf); [current BeebKit contract](../../Sources/BeebKit/BeebMachine.swift).

## Decision 2: Store read-only security-scoped bookmarks, not paths or ROM bytes

**Decision**: The macOS host stores a read-only security-scoped bookmark and
display metadata for each accepted user-selected source. On use, it resolves the
bookmark, refreshes it if stale, balances access acquisition/release, reads the
source into a private machine copy and asks the user to reselect it if access
fails.

**Rationale**: This remembers assignments after relaunch without relying on an
unauthorised path, retaining access indefinitely or making the source mutable.
The core receives bytes only and remains host-agnostic.

**Alternatives considered**:

- Persist absolute paths — rejected: sandbox access is not guaranteed after relaunch.
- Retain access for the whole process — rejected: unbalanced access leaks kernel resources.
- Persist imported ROM bytes in application settings — rejected: unnecessary user-content duplication.

**Sources**: [Apple: Accessing files from the macOS App Sandbox](https://developer.apple.com/documentation/Security/accessing-files-from-the-macos-app-sandbox); [Apple: startAccessingSecurityScopedResource](https://developer.apple.com/documentation/foundation/url/startaccessingsecurityscopedresource%28%29).

## Decision 3: Keep the existing runtime owner as the sole execution path

**Decision**: The host requests bounded runtime progression and consumes owned
frames, but does not mutate `BBCMicro` or drive it from wall-clock time.
Keyboard and controls submit existing serialized commands; reset/BREAK retain
the existing output-epoch rule.

**Rationale**: This adds a host workflow, not another synchronization model.

**Alternatives considered**:

- Give the view direct core access — rejected: violates ownership and races.
- Drive the core from display timestamps — rejected: changes emulated time semantics.

**Sources**: [Architecture](../../docs/ARCHITECTURE.md); [Implementation constraints](../../docs/IMPLEMENTATION_CONSTRAINTS.md).

## Decision 4: Verify proportionately and reserve complete M1 evidence for its gate

**Decision**: Each task starts with focused red evidence and closes with its
affected boundary checks. Feature closure runs the focused workflow aggregate,
affected C1/C2 and target-profile regressions, Swift tests, the changed macOS
build, documentation checks and direct observation. The full M1 audio-inclusive
journey waits for `machine-audio-output`.

**Rationale**: The delivery plan separates task, slice and milestone evidence.
Repeating audio or full historical suites during visual/control work adds delay
without proving changed behavior.

**Alternatives considered**:

- Run every maintained suite after every task — rejected: duplicates unchanged evidence.
- Omit application observation — rejected: cross-strand user-facing work needs it.

**Sources**: [Delivery plan](../../docs/product/MACHINE_DELIVERY_PLAN.md); [Evidence and Testing](../../docs/code/evidence-and-testing.md).
