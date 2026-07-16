# Inline Documentation Audit Catalogue

**Audit date**: 2026-07-16

**Branch**: `002-runtime-ownership`

**Verdict**: Request changes

**Scope**: Inline documentation only. The audit reviewed all 53 tracked C,
C++, Swift, and shell code files, including `Package.swift`, plus the module
map and the documentation-gate build configuration. Correctness, formatting,
test coverage, architecture changes, and general prose documentation are out
of scope.

The judgement authority is `docs/CODE_DOCUMENTATION.md`, Constitution
Principle VIII, and
`specs/001-core-baseline-evidence/contracts/code-documentation.md`. A task is
listed only when a public contract omits an applicable guarantee, a
private/internal named abstraction lacks its required developer-facing
boundary, a non-obvious hardware/timing/state/lifetime decision lacks rationale,
or an existing comment is stale or merely translates syntax.

`make docs-check` passes on the audited macOS tree. That proves the currently
generated public markup and links, but not the manual private/internal and
implementation-rationale requirements recorded below.

This catalogue records work; it does not authorize implementation.

## Gate and public-contract tasks

- [ ] **IDC-001 Make the private/internal documentation requirement enforceable.**
  `Doxyfile:14` excludes private declarations and
  `Tests/Fixtures/C0/documentation-debt.txt:21` explicitly excludes private
  header sections, although `docs/CODE_DOCUMENTATION.md:45-49` and the
  constitution require named internal abstractions. Define the enforceable
  scope, include changed production private/internal declarations in the gate
  or a reviewed inventory, and add a negative fixture for an undocumented
  named abstraction. Continue exempting obvious fields, accessors, loops, and
  delegators.

- [ ] **IDC-002 Correct and complete the C 0.2 declaration contracts.**
  `Sources/BeebCore/include/beeb_c.h:162-168` says a zero-byte sideways ROM is
  accepted, while the implementation rejects empty images. Correct the range
  to 1...16,384. Across operation declarations at lines 109-234, replace the
  generic `Operation-scoped status` return prose where necessary with the
  applicable validation, lifecycle, execution, resource, and unavailable
  outcomes so callers can tell legal states and failure categories without
  reading C++ implementation code.

- [ ] **IDC-003 State concurrency and callback contracts for public device types.**
  The class contracts in `Sources/BeebCore/include/beeb/crtc6845.hpp:8`,
  `disc_image.hpp:10-14`, `intel8271.hpp:13-23`, `sn76489.hpp:9-21`,
  `teletext_renderer.hpp:19-30`, `via6522.hpp:8-41`, and
  `video_ula.hpp:8-18` do not say whether instances are synchronized or which
  owner may mutate them. Add the applicable single-owner/reuse contract. For
  VIA and 8271 callbacks, also state when and on which calling thread they run,
  whether the object owns them, and how callback exceptions propagate.

- [ ] **IDC-004 Replace the CPU private-interface TODO with useful maintainer documentation.**
  `Sources/BeebCore/include/beeb/cpu6502.hpp:92-136` leaves an explicit TODO
  over the complete private interface. Document only the meaningful groups:
  borrowed bus lifetime, addressing helpers and page-cross reporting, stack and
  flag invariants, arithmetic helpers, and the single `finish()` timing
  boundary. Do not comment each register field or obvious helper.

- [ ] **IDC-005 Document the 8271 private state vocabulary.**
  `Sources/BeebCore/include/beeb/intel8271.hpp:56-96` contains undocumented
  internal named enums `Status` and `Transfer`. Explain the host-visible status
  bit responsibility, the mutually exclusive transfer phases, and the state
  invariants collaborators rely on; keep individual obvious storage members
  undocumented.

- [ ] **IDC-006 Complete the Swift input contracts.**
  `Sources/BeebKit/BeebMachine.swift:311-322` documents errors but omits DocC
  parameter contracts for `setKey(column:row:pressed:)` and
  `setBreak(pressed:)`. Add the 0...15 coordinate bounds, press/release meaning,
  and FIFO serialization/no-lifecycle-transition guarantee where applicable.

## Internal named-abstraction tasks

- [ ] **IDC-007 Document the demo's internal UI boundaries.**
  Add declaration documentation for the conditional `PlatformImage` alias,
  `EmulatorModel`, `ContentView`, and `BeebDemoApp` at
  `Sources/BeebDemo/main.swift:7,10,14,98,135`. Capture compile-time platform
  selection, `@MainActor` confinement, machine/timer ownership, model/view
  responsibility, and root-scene ownership without narrating SwiftUI syntax.

- [ ] **IDC-008 Document the Swift test-suite boundary.**
  `Tests/BeebKitTests/BeebMachineTests.swift:5` defines an undocumented named
  test type. Add one declaration summary covering Swift wrapper validation,
  typed error mapping, lifecycle/concurrency, and owned observations; individual
  test methods do not need boilerplate comments.

- [ ] **IDC-009 Document the C++ test harness and decimal-vector schema.**
  `Tests/test_main.cpp:33-71,281-320` leaves `RAMBus`, `TestFailure`, the
  important `Test` alias, and local `Vector` schema undocumented. Explain the
  fake bus's captured time/write evidence, the assertion boundary, the test
  registry role, and the difficult NMOS decimal-vector provenance and purpose.

- [ ] **IDC-010 Document the C1 replay evidence aggregates.**
  Add distinct summaries for `C1ReplaySignature`, `C1CapturedReplay`, and
  `C1ReplayOutcome` at `Tests/test_main.cpp:914-986`, including owned data and
  which deterministic capture/replay comparison each type represents.

- [ ] **IDC-011 Document evidence-tool configuration and ownership types.**
  `Tools/beeb-evidence/main.cpp:20-31,46` leaves `Output`, `Output::Kind`,
  `Options`, and the important `Machine` alias undocumented. State output-kind
  and path pairing, validated CLI configuration, and C-handle adoption/release.

- [ ] **IDC-012 Document headless-host ownership and its functional-test bus.**
  Add contracts for the `Machine` alias and `FlatBus` at
  `Tools/beeb-headless/main.cpp:37,102-107`: the alias adopts/releases one C
  handle; the bus is a side-effect-free 64 KiB adapter for functional images
  and intentionally advances no devices.

- [ ] **IDC-013 Document the clean-room ROM builder abstraction.**
  `Tools/make-demo-rom/main.cpp:13-76` needs a declaration contract for
  `ROMBuilder`: fixed 16 KiB C000 ROM purpose, cursor/address invariant,
  patch-offset convention, bounds behavior, and returned-buffer lifetime.

## Production rationale tasks

- [ ] **IDC-014 Record the C-handle admission and destruction invariants near the code.**
  The short summaries around `HandleState`, `ActiveCall`, and destruction at
  `Sources/BeebCore/src/beeb_c.cpp:88-176,211-253` omit the registry-before-state
  lock order, stable-token/no-dereference rule, admitted-call retention,
  first-destroy ownership, shared concurrent-destroy completion, and rejection
  behavior. Link the region to `docs/code/host-boundary.md` and state those
  lifetime/concurrency invariants once.

- [ ] **IDC-015 Document NMOS CPU edge semantics and their authority.**
  `Sources/BeebCore/src/cpu6502.cpp:60-65,126-170` implements the indirect-JMP
  page-wrap behavior and NMOS decimal ADC/SBC flags. The ADC comment is partial,
  SBC is unexplained, and neither area names its evidence. Link the primary
  6502/Visual6502 authority and state the observable page-wrap and N/V/Z/C rules.

- [ ] **IDC-016 Add one authoritative CPU transition and opcode-timing rationale.**
  Branch penalties, interrupt/reset stack behavior, opcode/cycle tables, and
  RMW dummy writes at `Sources/BeebCore/src/cpu6502.cpp:203-233,244-885` are
  hardware-derived. Explain these invariants near dispatch and link the timing
  authority; the current `dummy write` comment at line 271 merely names the
  action and omits its bus-visible consequence. Do not narrate every case.

- [ ] **IDC-017 Document the CRTC register mask/read-visibility table.**
  `Sources/BeebCore/src/crtc6845.cpp:14-25` contains hardware masks and readable
  register rules without an authority or consequence. Link a 6845 reference
  and explain why writes are masked and unsupported reads return zero. The
  existing `endScanline()` timing-model rationale is adequate.

- [ ] **IDC-018 Document DFS image geometry and interleave provenance.**
  `Sources/BeebCore/src/disc_image.cpp:8-26` derives 10x256 geometry,
  40/80-track bounds, and SSD/DSD offsets from a format convention. Replace the
  formula-paraphrase comment with a format/reference link and the accepted
  geometry/interleave invariant.

- [ ] **IDC-019 Add an 8271 protocol and aggregate-timing rationale.**
  `Sources/BeebCore/src/intel8271.cpp:29-261` needs one section-level link and
  explanation covering register side effects, parameter/SPECIFY handling,
  command/result codes, side selection, sector bitfields, NeedData/NMI order,
  and the fixed 64-cycle byte cadence. Distinguish preserved observable behavior
  from the current model's fidelity limits.

- [ ] **IDC-020 Document aggregate BBC hardware wiring and rendering rules.**
  Add focused rationale links at `Sources/BeebCore/src/machine.cpp:22-29,76-133,
  183-279` for System VIA/keyboard/IC32/sound collaboration, address decoding
  and mirroring, open-bus and ROM-write behavior, bitmap MA/wrap/bitplane/flash
  mapping, and keyboard synthesis. The existing evidence link explains why
  RGBA is compared, not why the hardware algorithm has its shape.

- [ ] **IDC-021 Document the sound-chip protocol and approximation limits.**
  `Sources/BeebCore/src/sn76489.cpp:19-77` needs an authoritative rationale for
  latch/data writes, tone-zero coercion, noise reset, amplitude table, clock
  divisors, tone-2 noise rate, LFSR choice, and mix scale. State units,
  deterministic/audible consequences, and current fidelity limits.

- [ ] **IDC-022 Document teletext provenance and modeled control behavior.**
  At `Sources/BeebCore/src/teletext_renderer.cpp:21-245`, identify the 5x7 glyphs
  as repository-authored clean-room approximations and link the evidence guide.
  Add a focused Mode 7 rationale for address/wrap, line-local control state,
  control-code timing, flash cadence, mosaics, scaling, and deliberate C0
  omissions.

- [ ] **IDC-023 Document VIA register side effects and timer/edge transitions.**
  `Sources/BeebCore/src/via6522.cpp:44-205` needs a 6522 authority and a concise
  explanation of aliases, IFR clearing, PB7, latches, IER polarity, timer
  underflow/reload/one-shot behavior, and CA1/CB1 edge selection, including
  software-visible interrupt and port consequences.

- [ ] **IDC-024 Complete the Video ULA rationale.**
  Partial comments at `Sources/BeebCore/src/video_ula.cpp:12-36` do not name an
  authority or explain active-low palette programming, flash XOR behavior,
  serializer-rate-to-bits-per-pixel mapping, and default observable behavior.
  Add one focused ULA/BBC reference and the relevant invariants.

## Test, evidence, and tooling rationale tasks

- [ ] **IDC-025 Replace syntax-translation comments in hardware fixtures.**
  Comment clusters at `Tests/test_main.cpp:87-180,551-811,1258-1319` mostly
  decode opcodes and register writes (`LDA`, `STA`, `enable CA1`) without saying
  why the fixture has that shape. Replace them with block-level hardware or
  evidence rationale links; retain per-line comments only for non-obvious
  fixture invariants and observable consequences.

- [ ] **IDC-026 Add a section rationale for the C1 concurrency fixtures.**
  The custom safe-point, FIFO, slice, copied-payload, deadline, and shutdown
  setups at `Tests/test_main.cpp:814-1555` need one link to
  `docs/code/runtime-ownership.md` and short notes identifying the observable
  invariant each non-obvious latch/deadline arrangement protects.

- [ ] **IDC-027 Document the evidence file schemas at serialization.**
  `Tools/beeb-evidence/main.cpp:123-155` serializes the canonical CPU state and
  PPM frame but only links the host boundary at file scope. Add a nearby link to
  `docs/code/evidence-and-testing.md` explaining the schema/version invariant,
  exact fields, and why PPM output drops alpha.

- [ ] **IDC-028 Explain how generated demo ROMs produce deterministic evidence.**
  At `Tools/make-demo-rom/main.cpp:105-241`, replace the vague `conventional`
  CRTC note and opcode-only comments with one evidence/hardware rationale for
  reset/vector placement, CRTC tables, screen ranges, ULA values, branch
  patching, and idle traps for each workload.

- [ ] **IDC-029 Document documentation-builder temporary-file ownership.**
  `scripts/build-docs.sh:109-110,296-299` installs conditional `EXIT` traps and
  identifies ownership through a filename pattern. Add a short invariant
  explaining which generated paths each trap owns and why the later trap must
  not discard cleanup of the generated changed-files inventory.

- [ ] **IDC-030 Link the changed-complex-code gate to its governing rule.**
  `scripts/build-docs.sh:240-282` implements a non-obvious diff-status inversion
  plus file-level rationale-marker acceptance. Add a tooling rationale linked
  to `docs/CODE_DOCUMENTATION.md`: changed non-comment code requires either
  `path|N/A: reason` or an accepted rationale marker.

- [ ] **IDC-031 Explain interrupted measurement-record persistence.**
  `scripts/measure-c0.sh:117-141` disables `errexit` so interrupted/incomplete
  samples still become classified evidence instead of disappearing. Add a
  concise rationale beside `set +e`, including the 130/143 interruption rule.

- [ ] **IDC-032 Explain locale-stable reference verification.**
  `scripts/verify-c0-references.sh:3` sets `LC_ALL=C` without recording the
  cross-host invariant. State that manifest ordering/discovery is deliberately
  locale-stable so provenance validation remains deterministic.

- [ ] **IDC-033 Explain failure-retaining aggregate verification.**
  `scripts/verify-c0.sh:172-190` suppresses `errexit` so later groups still run
  and the final status retains every failure/interruption. Add this observable
  aggregation invariant beside `set +e`.

- [ ] **IDC-034 Explain why the TSan support probe must execute.**
  `Tests/C1/test-runtime-races.sh:6-20` compiles and runs a probe because some
  hosts link ThreadSanitizer but cannot start its runtime. Add a short tooling
  rationale above `c1_tsan_supported`; do not narrate the probe commands.

## Coverage record

The following files were reviewed and have no catalogue item:

- `Package.swift`
- `Sources/BeebCore/include/beeb/bus.hpp`
- `Sources/BeebCore/include/beeb/machine.hpp`
- `Sources/BeebCore/include/beeb/runtime.hpp`
- `Sources/BeebCore/include/beeb/version.h`
- `Sources/BeebCore/src/runtime.cpp`
- `Sources/BeebCore/include/module.modulemap`
- `Sources/BeebKit/BeebVersion.swift`
- `scripts/run-klaus.sh`
- `scripts/update-c0-reference.sh`
- `Tests/C0/test-baseline-verifier.sh`
- `Tests/C0/test-demo-rom.sh`
- `Tests/C0/test-documentation.sh`
- `Tests/C0/test-fixture-evidence.sh`
- `Tests/C0/test-measurement-record.sh`
- `Tests/C0/test-reference-update.sh`
- `Tests/C0/testlib.sh`
- `Tests/C1/test-aggregate-runner.sh`
- `Tests/C1/test-documentation.sh`
- `Tests/C1/test-public-boundaries.sh`
- `Tests/C1/test-runtime-contract.sh`
- `Tests/C1/test-runtime-replay.sh`
- `Tests/C1/testlib.sh`

All other reviewed code files are named by at least one catalogue item. The
`Makefile`, `Doxyfile`, and `.github/workflows/ci.yml` were also inspected as
documentation-gate configuration; only the private-scope issue in IDC-001
requires action from that configuration review.

## Recommended execution order

Address contract accuracy first (`IDC-002`, `IDC-006`), then named abstractions
(`IDC-004` through `IDC-013`), then production rationale (`IDC-014` through
`IDC-024`), followed by test/tooling rationale (`IDC-025` through `IDC-034`).
Finish with `IDC-001` so the strengthened gate is calibrated against the
reviewed result rather than encouraging boilerplate comments.

For every task, run `make docs-check`, inspect the affected generated pages
where the declaration is generated, and run `git diff --check`. A task is not
complete if it merely adds prose that restates names, types, or control flow.
