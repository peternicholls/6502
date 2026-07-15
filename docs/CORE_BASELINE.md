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

## Aggregate verification

Run the current C0 baseline from the repository root:

```bash
make verify-c0
```

The command selects `macos` on Darwin and `portable` elsewhere. An explicit
profile is available for CI and diagnosis:

```bash
scripts/verify-c0.sh --profile portable
scripts/verify-c0.sh --profile macos
```

Each group writes a separate log under `.build/c0/run/`. The ordered text
summary reports `pass`, `fail`, `unexpected-skip`, or `not-applicable` for every
group, then derives one overall result. A failed group does not stop later
groups, so the summary exposes independent problems in one run.

| Group | Portable | macOS | Current evidence |
| --- | --- | --- | --- |
| `cpp-behavior` | Required | Required | `make test`, currently 27 of 27 |
| `sanitizers` | Required | Required | `make sanitize`, currently 24 of 24 in the quick profile |
| `version-sync` | Required | Required | `make check-version` |
| `c-boundary` | Required | Required | C ABI recovery cases in the quick C++ suite |
| `swift-boundary` | Not applicable | Required | `swift test` (7 cases) and `swift build` |
| `fixture-provenance` | Required | Required | Complete lawful manifest and tracked identities |
| `cleanroom-boot` | Required | Required | Exact approved Mode 7 CPU/frame state |
| `bitmap-reference` | Required | Required | Byte-exact 320×200 PPM |
| `mode7-reference` | Required | Required | Byte-exact 480×500 PPM |

Generated-documentation groups join this table only when their later C0 tasks
provide passing evidence. Until then, the aggregate result proves US1 and US2
plus the existing behavioral, safety, version, and public-boundary foundation;
it is not the complete C0 exit gate.

### Failure and recovery

- `fail` retains the command output in the named group log and makes the final
  result unsuccessful.
- `unexpected-skip` means a required command is unavailable and also fails the
  run; install or restore the named tool before retrying.
- `not-applicable` is permitted only by the selected profile, currently for the
  Swift boundary on a portable host.
- Exit status `130` or `143` is reported as an interrupted failure while later
  declarative groups still run.
- Unsupported profiles are rejected before any group executes.

Ordinary verification removes and recreates only `.build/c0/run/`. It has no
reference-update command and writes nothing under `Tests/`. The focused
`make test-c0` suite snapshots tracked test content to enforce this boundary.

## Approved clean-room evidence

The tracked provenance and exact files live under
[`Tests/Fixtures/C0/`](../Tests/Fixtures/C0/README.md). Both named workloads
produced byte-identical ROM, state, and PPM outputs across ten consecutive clean
runs before approval.

| Evidence | Approved SHA-256 | Bytes | Cycle result |
| --- | --- | ---: | --- |
| Mode 7 ROM (generated, not tracked) | `8e83440f60a5286714b98ac12444ab21ffd27894b9241d090b7c6ae10262f24e` | 16,384 | Request 100,000; executed 100,002 |
| Exact Mode 7 state | `f13c6b64ec7ed26bd392800061d4ed869fb9d2c9a2e907d3d22533280ca3180a` | 176 | `PC=$C0AF`, frame 12, 480×500 |
| Exact Mode 7 PPM | `c4c9884af9187ab1178f63480962b6921b98a87e1e674371c40904d505fcc994` | 720,015 | Same named run |
| Bitmap ROM (generated, not tracked) | `087579af73a39eb8efab09dc2c3ae5b26ebddfe32bc32ed0c0db443de5fd89e1` | 16,384 | Request 100,000; executed 100,000 |
| Exact bitmap PPM | `5882cedf1a0939ab8e77144fd73bc713ae13a5b2e2faece7110c5fb40faf4f00` | 192,015 | Frame 17, 320×200 |

The Mode 7 execution delta is 100,002 cycles. The pre-C0 research output showed
the CPU's cumulative counter as 100,009 because reset accounts for seven cycles
before the requested run. Both observations are consistent; the approved
manifest records the execution delta and exact post-run register/frame state.

These references prove deterministic behavior only for the documented
workloads and current aggregate timing. They do not prove complete Mode 7,
bitmap, CRTC, ULA, cursor, palette, hardware-scroll, or mid-frame compatibility.
Those limitations remain in [STATUS.md](STATUS.md).

### Replacement history and policy

- 2026-07-15: initial state, bitmap, and Mode 7 references approved after ten
  identical executions per workload.

`make verify-c0-references` is read-only. An intentional change must use
`make update-c0-reference REFERENCE=<id> REASON="<review rationale>"`, which is
disabled in CI, regenerates ten candidates, updates only the named reference
and its derived manifest fields, and prints the fixture diff for review. A
mismatch never authorizes replacement by itself.

## Interpretation

This record distinguishes the known starting behavior from C0 exit evidence.
It is not a compatibility claim, a performance promise, or permission to bundle
proprietary firmware or media. Later sections will be added only when their
commands and failure cases are implemented and verified.
