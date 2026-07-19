# Core baseline evidence

**Status:** C0 verified evidence ledger; opening section preserves pre-C0
observation

**Initial observation recorded:** 2026-07-15
**Initial source revision:** `819ee387393437e21a389265d8d9b98f720783d3`
**Status updated:** 2026-07-19

This document begins with the historical observation record taken immediately
before C0 implementation. Values in that opening section were not approved C0
references until the provenance, determinism, and immutable-reference
acceptance tests passed. The later C0 exit section records that completed gate.
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

The then-planned C0 work was required to preserve the direct jobs while adding
an aggregate portable profile and a complete macOS profile. Swift- and
DocC-specific evidence is not applicable to the portable-only profile, but must
pass in the complete profile.

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

These values were deliberately labelled research-only. C0 could approve them
only after named workloads, complete lawful provenance, ten identical runs, and
the separate reference-review path existed.

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

## Descriptive throughput comparison

The initial C0 comparison record was captured from clean revision
`9a59d3679e238083edc7f3cb01defe04549b96a9` with:

- workload: `cleanroom-mode7-100k`;
- host: Apple M2 Ultra, arm64, Darwin 25.5.0;
- compiler: Apple clang 17.0.0 (`clang-1700.6.4.2`);
- build mode: release (`-O2` through the repository Makefile);
- requested cycles per sample: 100,000;
- actual completed-instruction cycles per sample: 100,002.

| Sample | Elapsed nanoseconds | Emulated cycles/second |
| ---: | ---: | ---: |
| 1 | 7,932,000 | 12,607,413 |
| 2 | 6,877,000 | 14,541,515 |
| 3 | 7,334,000 | 13,635,397 |
| 4 | 8,029,000 | 12,455,100 |
| 5 | 7,812,000 | 12,801,075 |

- median: 12,801,075 emulated cycles/second;
- range: 12,455,100–14,541,515 emulated cycles/second.

The valid raw record remains a generated artifact at
`.build/c0/measurements/2026-07-15-apple-m2-ultra.txt`. The values above are a
descriptive comparison baseline only. They are not a product guarantee, release
threshold, latency target, or evidence of hardware compatibility. Later work
must compare the same workload and record its revision, host, compiler, build
mode, individual samples, median, and range; unlike environments are not
directly comparable.

## Browsable code-documentation evidence

The complete macOS profile passed at clean implementation revision
`4d0b9bc368da8b700f35f9fd68c70f0aac2f7213` with:

```bash
make docs-check DOCS_PROFILE=macos
```

The generated site was reviewed from `.build/docs/index.html`. The unified
landing linked both destinations, and the following representative pages were
present, readable, and consistent with the implementation:

| Surface | Reviewed generated page | Contract confirmed |
| --- | --- | --- |
| CPU | `cpp/classbeeb_1_1_c_p_u6502.html` | Instruction-level transition, cycle result, bus timing, illegal-opcode error |
| Machine | `cpp/classbeeb_1_1_b_b_c_micro.html` | Aggregate ownership, serialization requirement, media copies, frame lifetime |
| C ABI | `cpp/beeb__c_8h.html` | Opaque ownership, null/error behavior, input copies, borrowed-frame invalidation |
| Swift | `swift/documentation/beebkit/beebmachine/index.html` | Locked access, `Sendable` basis, thrown errors, C-buffer copy into `Data` |
| Timing guide | `cpp/md_docs_2code_2timing-model.html` | 2 MHz reference, VIA/CRTC conversion, remainder and frame-transition model |
| Evidence guide | `cpp/md_docs_2code_2evidence-and-testing.html` | Tool/runtime separation, exact references, lawful fixtures, read-only verification |

Generator environment:

- Doxygen `1.17.0`, with configured warnings promoted to failure;
- official `swiftlang/swift-docc-plugin` `1.5.0`, pinned exactly in
  `Package.resolved`;
- Apple Swift `6.2.4` (`swiftlang-6.2.4.1.4`);
- DocC bundled with Xcode `26.3` build `17C519` (the bundled executable does not
  expose a standalone `--version` option).

The tracked documentation-debt baseline is zero. Generated HTML remains ignored
and reproducible; source comments and `docs/code/` guides are authoritative.

## C0 exit gate evidence

The complete local macOS gate ran against clean revision
`d42a34b79d8d24165c411742d57b61858d8d6639` on 2026-07-15. Each command ran
independently, wrote a concise log under `.build/c0/exit/`, and recorded its
exact exit status before the overall result was derived.

| Gate | Exit | Concise result |
| --- | ---: | --- |
| `make test` | 0 | 27/27 C++ and C boundary tests passed |
| `make sanitize` | 0 | 24/24 quick-profile tests passed under UBSan |
| `swift test` | 0 | 7/7 XCTest cases passed |
| `swift build` | 0 | Debug package build passed |
| `make verify-c0` | 0 | All 11 macOS evidence groups passed |
| `make docs-check` | 0 | Complete Doxygen and DocC profile passed |
| all `Tests/C0/test-*.sh` via `make test-c0` | 0 | All six contract scripts passed, including deliberate failures |
| `git diff --check` | 0 | No whitespace errors |

The aggregate record at `.build/c0/run/run.txt` reported schema
`beeb-c0-baseline-v1`, `profile=macos`, the same source revision, and
`overall=pass`. These logs are ignored reproducible evidence; the tracked
commands, approved references, and this concise result are authoritative.

## Interpretation

This record distinguishes the known starting behavior from completed C0 exit
evidence. It is not a compatibility claim, a performance promise, or permission
to bundle proprietary firmware or media. Any later evidence section may be
added only when its commands and failure cases are implemented and verified.
