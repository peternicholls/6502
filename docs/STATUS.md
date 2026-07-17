# Implementation status

This document records verified emulator implementation state. Technical
priority belongs to the [core roadmap](CORE_ROADMAP.md); the wider application
is governed separately by the [product strand](product/README.md).

## Verified

| Area | Evidence | Status |
| --- | --- | --- |
| NMOS 6502 legal instruction set | All 151 opcodes decode; targeted semantics and cycle tests | Green |
| Independent CPU conformance | Klaus functional image stable success trap `$3469`, 96,241,664 cycles | Green |
| Binary arithmetic | Exhaustive `256 × 256 × carry` ADC and SBC | Green |
| Valid BCD arithmetic | Exhaustive `100 × 100 × carry` ADC and SBC | Green |
| NMOS decimal flags | Visual6502 difficult ADC/SBC vectors | Green |
| Interrupt/stack quirks | IRQ/NMI, BRK/RTI, JSR/RTS, pushed status, NMOS D preservation | Green |
| Basic BBC boot | OS 1.20 + BASIC II smoke test reaches the BASIC idle loop and renders startup | Green smoke test |
| Core build | GCC 13, C++20, warnings promoted to errors | Green |
| Swift package | BeebKit tests and full package build on Apple Swift 6.2 | Green |
| C1 runtime ownership | One owner, capacity-64 FIFO, safe-point lifecycle, exact replay, structured C/Swift recovery | Green; local TSan N/A |
| C2 bounded output | Owned frame/audio queues, allocation-safe C transfer, exact C/Swift replay and concurrency evidence, 10,000-frame lifetime transfer, 60-second sustained measurement | Green; supported CI requires TSan |
| Xcode project | Shared macOS, iOS Simulator, and test schemes over the local package and existing demo source | Green clean-checkout contract |
| C0 aggregate evidence | `make verify-c0`: all 11 macOS groups; explicit portable profile passes with Swift groups N/A | Green |
| Lawful exact references | Ten identical clean-room runs; exact CPU state plus 320×200 bitmap and 480×500 Mode 7 PPMs | Green |
| Code documentation | Generated public Doxygen + DocC references; branch-aware internal named-abstraction checks; separate public/internal debt baselines at zero | Green within enforced scope |

## C0 foundation evidence

The clean macOS exit run and its exact command statuses are recorded in
[CORE_BASELINE.md](CORE_BASELINE.md#c0-exit-gate-evidence). The supported local
commands are:

```bash
make test             # 27/27
make sanitize         # 24/24 quick profile
swift test            # 7/7 XCTest cases
swift build
make verify-c0        # 11/11 macOS evidence groups
make docs-check       # complete Doxygen and DocC profile
make test-c0          # all six focused contract scripts
```

The portable aggregate profile is `make verify-c0 C0_PROFILE=portable`; its C,
C++, provenance, exact-reference, and Doxygen groups pass, while the two
Swift-specific groups are explicitly not applicable. CI retains both direct
jobs and complete portable/macOS aggregate jobs.

## C1 pre-release candidate evidence

The clean detached candidate at revision
`64e8ef6db48c09565b2c99d2def2a5493bba2191` passed the complete local macOS gate
on 2026-07-15. This checkpoint deliberately precedes the 0.2.0 version and final
roadmap synchronization tasks; the completion candidate is rerun after those
changes rather than treating this intermediate revision as the release.

| Command | Exact result |
| --- | --- |
| `make test` | 42/42 tests passed, including C 0.2, runtime replay, 10,000-command, and shutdown overlap cases |
| `make sanitize` | 37/37 quick tests passed under UndefinedBehaviorSanitizer |
| `make thread-sanitize` | N/A: the local Apple-clang ThreadSanitizer probe is not executable on this host |
| `swift test` | 7/7 XCTest cases passed, including task-group lifecycle/input/observation concurrency |
| `swift build` | Complete package and demo build passed |
| `make verify-c0` | All 11 macOS groups passed; exact state, bitmap, and Mode 7 references remained unchanged |
| `make test-c1` | All six aggregate groups passed; the injected-failure runner proved later groups are not masked |
| `make docs-check` | Strict Doxygen and Swift-DocC macOS profile passed |
| `git status --short` / `git diff --check` | No output in the detached candidate |

ThreadSanitizer is not counted as a pass. The normal mixed-command and bounded
shutdown races ran in both `make test` and `make test-c1`; supported CI must
still execute the real ThreadSanitizer binary.

## C1 completion candidate evidence

After the 0.2.0 and roadmap changes, clean detached revision
`5ed22995b0c79e708a02403bdb5799f623cf4cfd` reran every distinct command in the
C1 quickstart on 2026-07-15:

- `make test`: 42/42 passed;
- `make sanitize`: 37/37 quick tests passed;
- `swift test`: 7/7 passed, and `swift build` completed;
- `make verify-c0`: all 11 macOS groups passed with exact references unchanged;
- focused runtime contract, replay, and race scripts passed 10, 2, and 2 tests
  respectively;
- `make test-c1`: all six groups passed, including public-boundary,
  documentation-negative, and aggregate-failure-propagation coverage;
- `make docs-check` and `Tests/C1/test-documentation.sh` passed;
- `make thread-sanitize` reported N/A after the unsupported local probe;
- `git status --short`, `git diff --check`, and the tracked `.build` query
  produced no output.

The phase-closing commit changes only this evidence ledger and the completed
task marker. Its committed revision is checked again for generated
documentation and a clean tree; no empty checkpoint commit is created.

## C1 remediation completion evidence

The Phase 12 remediation candidate built from `f38ea42` plus the scoped T076
changes passed the complete local macOS gate on 2026-07-17. A final read-only
code review returned APPROVE with no CRITICAL or HIGH findings; its two medium
documentation/traceability notes were corrected before the phase commit.

| Command | Exact result |
| --- | --- |
| `make test` | 49/49 tests passed |
| `make sanitize` | 44/44 quick tests passed under UndefinedBehaviorSanitizer |
| `make thread-sanitize` | N/A: ThreadSanitizer is not supported by the local compiler/host combination; supported Linux CI remains strict |
| `swift test` | 10/10 XCTest cases passed |
| `swift build` | Complete package and demo build passed |
| `make verify-c0` | All 11 macOS evidence groups passed with exact references unchanged |
| `make test-c1` | All six groups passed, including ten normal 10,000-command race repetitions and the replay digest suite |
| `make docs-check` / `make format-check` | Strict generated documentation and formatting gates passed |
| Clang static analysis | Every `Sources/BeebCore/src/*.cpp` translation unit completed without a diagnostic |
| `git diff --check` / `git ls-files .build` | No output |
| `git status --short` | Before the phase commit, only the scoped T076 source, test, script, specification, and documentation paths were listed |

The retained fault boundary now keeps legal completed instructions and their
device ticks, while allocation/internal failures restore the whole-command
checkpoint. Replay evidence includes hidden CPU interrupt latches, ROM/media,
VIA timers and edges, CRTC adjustment state, sound phase/noise state, and FDC
transfer state. Destructive tools require their own ownership marker before
removing an existing `.build` directory. C and Swift evidence covers resource
recovery and shutdown unavailability, and fault/reset recovery runs while
observers contend for the runtime FIFO.

## C2 completion evidence

The `003-bounded-output-contracts` candidate passed its measurement gate on
2026-07-17 before any C2 completion claim was added. The committed harness
writes the run record to ignored `.build/c2/measurements/latest.txt`; the
reproducible workload and pass/fail thresholds remain tracked in
`Tests/C2/test-output-measurement.sh`.

| Evidence | Exact result |
| --- | --- |
| Owned-frame lifetime | 10,000 frames produced and 10,000 consumed; zero drops; the retained first value remained unchanged |
| Queue pressure stress | 10,000 frame and 10,000 audio publications; maximum depths reached and never exceeded 3 frames / 4,096 samples; exact consumed+dropped/overrun accounting |
| Sustained duration | 10 emulated warm-up seconds followed by 120,000,060 measured cycles, exceeding the required 60 emulated seconds |
| Runtime conservation | 46,666,683 frames produced = 71 consumed + 46,666,610 dropped + 2 retained; 3,360,001 samples produced = 286,720 consumed + 3,073,281 overrun + 0 retained |
| Memory | RSS grew 32,768 bytes after warm-up, within the 16,777,216-byte tolerance |
| Host-observed rate | Expected 2.0, observed 2.0, relative error 0; the helper left runtime diagnostics unchanged |
| Xcode delivery | All three shared schemes listed; macOS and generic iOS Simulator builds succeeded; the test scheme ran 17 tests with zero failures; ignored local user state did not make the contract fail or rewrite maintained project metadata |
| Independent build paths | The Xcode contract also passed `swift build`, `swift test`, and `make test` without source duplication or user/signing metadata |

The complete local macOS exit matrix then passed on 2026-07-17:

| Command | Exact result |
| --- | --- |
| `make test` | 54/54 tests passed |
| `make sanitize` | 49/49 quick tests passed under UndefinedBehaviorSanitizer |
| `make thread-sanitize` | N/A: ThreadSanitizer is unsupported by the local compiler/host combination; supported CI remains required |
| `make format-check` | Passed |
| `make test-c1` | All six C1 groups passed, including ten normal race repetitions |
| `make test-c2` | All nine C2 groups passed; the final measurement rerun reported 32,768 bytes RSS growth and zero rate error |
| `swift test` / `swift build` | 17/17 XCTest cases passed; the package and demo built |
| `make test-c2-xcode` | All shared schemes listed; macOS and generic iOS Simulator builds passed; 17/17 test-scheme cases passed; maintained project metadata was unchanged |
| `make docs-check` | Complete Doxygen and Swift-DocC generation/link validation passed |
| `git diff --check` | No output |

The generated-link validator was also corrected during the exit run: it now
checks the entire DocC/Doxygen tree in one Perl process instead of launching a
parser per HTML file. The existing broken-link negative fixture and both C0/C2
documentation suites pass with unchanged validation semantics. C2 is complete;
local ThreadSanitizer remains N/A rather than a pass.

The post-review remediation makes the C frame hand-off failure-atomic: storage
for its opaque release context is secured before the runtime dequeues a frame,
and an allocation failure leaves the queue, counters, and caller output
unchanged. The release context now authenticates the exact byte pointer and
length being released instead of asking callers to free core-owned storage.
Fresh-runtime replay and real producer/consumer contention cross both the C ABI
and Swift actor boundary. CI runs the portable C2 aggregate with
`C2_REQUIRE_TSAN=1` on its supported Linux toolchain, while Apple-only Xcode
evidence remains in the macOS lane. Local unsupported ThreadSanitizer results
therefore remain N/A without weakening the supported CI requirement. The exit
rerun also exposed and removed two scheduler-timing assumptions in earlier C1
evidence: destroy overlap now holds a call after actual boundary admission, and
the sustained lifecycle test waits for an execution-slice event rather than
mistaking setup-command ledger entries for production.
Reset is now an explicit output epoch boundary: retained pre-reset media and
fractional audio timing are cleared, while runtime-lifetime identities remain
monotonic and discarded depths enter the existing drop/overrun terms. Fresh
C++, C, and Swift tests prove post-reset dequeue/drain behavior and both exact
conservation equations.

## Release state

Version 0.3.0 is the current unreleased development candidate for the additive
C2 public contracts. The earlier 0.2.0 C1 candidate also remains recorded as
`Unreleased`; live verification on 2026-07-16 found no remote 0.2.0 tag or
release. This branch created no tag or published release. Recheck remote state
before following [RELEASING.md](RELEASING.md); do not treat the prior remote
observation as current release proof.

## C1 verified outcome

C1 is complete at the architectural and public-boundary level:

- `MachineRuntime` is the only supported owner of aggregate BBC machine state;
- start, pause, reset, media, input, observation, fault, and shutdown commands
  share one bounded FIFO and complete at documented safe points;
- sustained work uses deterministic 2,048-cycle minimum slices with no host
  wall-clock input, and captured concurrent interleavings replay exactly;
- C 0.2 returns operation-owned structured statuses and caller-owned frames;
- Swift preserves typed categories and diagnostics without a redundant lock;
- accepted-before-shutdown work drains, new entries reject, and destruction
  waits calls already inside the C boundary.

This C1 evidence is now consumed by the C2 owner-only output implementation;
the separate measured-candidate section owns those newer output claims. C1
still does not claim snapshot persistence or bus-cycle fidelity.

Approved evidence is clean-room and byte exact:

| Observation | SHA-256 | Verified scope |
| --- | --- | --- |
| Mode 7 CPU/frame state | `f13c6b64ec7ed26bd392800061d4ed869fb9d2c9a2e907d3d22533280ca3180a` | Registers, 100,002-cycle execution delta, frame 12 at 480×500 |
| Bitmap frame | `5882cedf1a0939ab8e77144fd73bc713ae13a5b2e2faece7110c5fb40faf4f00` | Complete 320×200 PPM for the named bitmap workload |
| Mode 7 frame | `c4c9884af9187ab1178f63480962b6921b98a87e1e674371c40904d505fcc994` | Complete 480×500 PPM for the named clean-room Mode 7 workload |

The generated public reference covers all 12 exported C/C++/C ABI header
surfaces, all public `BeebKit` symbols, and four focused conceptual guides.
Representative generated pages include `beeb_c.h`, `MachineRuntime`, runtime
ownership, and the host boundary. Doxygen remains public-reference oriented; it
does not claim generated private-member completeness.

Changed private/internal named abstractions are instead enforced by the
branch-aware source gate using `DOCS_BASE=<base>`. The tracked public and
internal debt baselines are independently zero. That means no reviewed debt is
currently recorded within either enforced scope, not that every private field
is generated or requires prose. The only reviewed exclusions are SwiftPM's
non-declaration module-map wiring and self-evident fields, accessors, and
delegators without an independent responsibility boundary. Generated output
remains ignored under `.build/docs/`.

The descriptive throughput comparison used five 100,000-cycle Mode 7 samples
on an Apple M2 Ultra with Apple clang 17.0.0 and the release `-O2` Make build.
Median throughput was 12,801,075 emulated cycles/second, with a
12,455,100–14,541,515 range. This is comparison context only—not a compatibility,
latency, release, or product-performance guarantee.

## Hardware fidelity

| Device | Implemented | Still required |
| --- | --- | --- |
| 6502 | Legal opcodes, flags, decimal mode, interrupts, cycles | Per-bus-cycle micro-operations; optional undocumented opcodes |
| Memory | Model B RAM, OS ROM, 16 sideways banks, I/O overlay | Sideways RAM write policy; 1 MHz bus wait-state phasing |
| 6522 | Ports/DDRs, T1/T2, IFR/IER, CA1/CB1, PB7 timer output | Shift modes, complete CA2/CB2 handshake/pulse modes, latches |
| 6845 | Register masks, raster/frame counters, display geometry | Sync widths, interlace, cursor output, exact variants and rupture effects |
| Video ULA | Serializer modes, palette and flash | Exact cursor/clock edge behaviour |
| Bitmap video | 1/2/4-bit interleave, mode-sized output, wrap | Cycle raster, mid-line changes, rigorous hardware-scroll cases |
| Mode 7 | Text, colours, flash, contiguous/separated mosaics | Double-height pairs, hold/release, conceal, precise control-code latency |
| SN76489 | Tone/noise registers, deterministic sample generation and bounded continuous output | Band-limiting, exact LFSR variant confirmation, host audio-device integration |
| 8271 | SSD/DSD sector read/write, seek, status, special registers, NMI bytes | Full command/timing/error model, formatting, deleted sectors, flux |
| Keyboard | Matrix injection through System VIA | Complete host-to-BBC key map and all IC32 nuances |
| Cassette | — | 6850, Serial ULA, UEF chunks, WAV edge decoder, motor timing |

## Current core focus

C0 baseline evidence, C1 runtime ownership, and C2 bounded output/Xcode
delivery are complete. C3 session continuity is the default next core source.
Bus-cycle implementation remains queued until C3 fixes
the version-1 snapshot invariant, so it cannot accidentally invalidate public
session state.

The [core roadmap](CORE_ROADMAP.md) owns technical sequence; the tables above
own verified hardware-fidelity status. The C0 evidence does not close any gap in
the hardware-fidelity table. A phase is not Active until an approved Spec Kit
feature enters implementation.
