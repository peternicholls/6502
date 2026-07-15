# Core baseline evidence

**Status:** Pre-C0 observation

**Recorded:** 2026-07-15
**Source revision:** `819ee387393437e21a389265d8d9b98f720783d3`

This document begins as the observation record taken immediately before C0
implementation. Values in this section are not approved C0 references until
the provenance, determinism, and immutable-reference acceptance tests pass.
Verified implementation coverage and known hardware limitations remain
authoritative in [STATUS.md](STATUS.md).

## Pre-C0 verification

| Surface | Command | Observed result |
| --- | --- | --- |
| C++ behavior | `make test` | 26 of 26 tests passed |
| Native sanitizer profile | `make sanitize` | 23 of 23 quick-profile tests passed |
| Swift boundary | `swift test` | 4 of 4 XCTest cases passed |
| Swift package | `swift build` | Debug package build passed |
| Release version | `make check-version` | `VERSION`, CLI, public headers, changelog entry agree on `0.1.0` |

The repository version is `0.1.0`. It is reported by `VERSION`,
`BEEB_VERSION_STRING`, `beeb_version_string()`, `BeebVersion.current`, and
`beeb-headless --version`.

## Supported verification profiles

- **Portable Linux:** GitHub Actions `ubuntu-latest` builds and tests the C++20
  core, headless tool and clean-room ROM, then runs AddressSanitizer and
  UndefinedBehaviorSanitizer.
- **Apple package:** GitHub Actions `macos-latest` runs the Swift package tests
  and builds the SwiftUI demo. The local C0 observation used Apple Swift 6.2.4.

C0 will preserve the direct jobs while adding an aggregate portable profile and
a complete macOS profile. Swift- and DocC-specific evidence is not applicable
to the portable-only profile, but must pass in the complete profile.

## Research-only clean-room signature

The existing redistributable generator and headless runner produced the
following observation for a request of 100,000 emulated cycles:

- ROM SHA-256:
  `8e83440f60a5286714b98ac12444ab21ffd27894b9241d090b7c6ae10262f24e`
- CPU state:
  `PC=$C0AF A=$00 X=$1E Y=$00 SP=$FF P=$26 cycles=100009`
- Frame metadata: frame `12`, `480x500`, sideways ROM bank `0`
- PPM SHA-256:
  `c4c9884af9187ab1178f63480962b6921b98a87e1e674371c40904d505fcc994`

These values are deliberately labelled research-only. C0 may approve them only
after named workloads, complete lawful provenance, ten identical runs, and the
separate reference-review path exist.

## Known limitations at entry

- Current bitmap and Mode 7 tests assert representative pixels or cells, not
  byte-exact end-to-end frames.
- The clean-room ROM has no named bitmap versus Mode 7 workload contract.
- No aggregate baseline command attempts and reports every evidence group.
- No approved-reference manifest or separately reviewed update flow exists.
- No structured five-sample performance record exists.
- Public C/C++ and Swift documentation is not yet generated into a browsable
  site, and documentation debt is not measured.
- Hardware gaps in CPU bus-cycle sequencing, VIA handshake modes, CRTC and ULA
  edge behavior, Mode 7 controls, sound fidelity, 8271 timing, keyboard details,
  and cassette support remain unchanged from [STATUS.md](STATUS.md).

## Interpretation

This record distinguishes the known starting behavior from C0 exit evidence.
It is not a compatibility claim, a performance promise, or permission to bundle
proprietary firmware or media. Later sections will be added only when their
commands and failure cases are implemented and verified.
