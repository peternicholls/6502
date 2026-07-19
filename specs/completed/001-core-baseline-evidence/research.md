# Phase 0 Research: Core Baseline Evidence

## Decision 1: Preserve the core and add an external evidence layer

**Decision**: Build `Tools/beeb-evidence` and strict orchestration scripts around
the existing public CPU-state and frame surfaces. C0 does not add a production
core API, private counter, object dump, or micro-operation trace.

**Rationale**: C0 exists to lock observable behavior before architectural work.
Changing the observed system while defining its baseline weakens the evidence.
The current C ABI already exposes version, loading, execution, CPU state, frame
bytes, and recoverable errors.

**Alternatives considered**:

- Serialize internal C++ objects: rejected because it couples the baseline to
  layout and would pre-empt the later snapshot/versioning design.
- Add per-device debug APIs: rejected because C0 does not need them and C6 owns
  inspection capability.
- Reuse only the headless CLI: rejected because it emits a useful PPM and state
  summary but does not define multiple canonical workloads or machine-readable
  evidence records cleanly.

## Decision 2: One aggregate verifier, exhaustive reporting

**Decision**: `make verify-c0` invokes `scripts/verify-c0.sh`. The script cleans
`.build/c0`, attempts every environment-applicable evidence group, preserves
each result, prints a stable text summary, and exits unsuccessfully if any
required group fails or is skipped unexpectedly.

**Rationale**: A fail-fast shell chain hides the full state of the foundation.
Maintainers need one command and one result without losing the underlying test
or sanitizer failure.

**Alternatives considered**:

- Replace existing test commands with a new test framework: rejected because
  existing commands already express their domains and a framework adds no C0
  value.
- Stop at the first failure: rejected because it makes the baseline slower to
  diagnose and can mask an independent boundary failure.

## Decision 3: Exact tracked visual references with an explicit update path

**Decision**: Track binary PPM references under `Tests/Fixtures/C0/` and compare
them byte-for-byte. Normal verification is read-only. Only
`scripts/update-c0-reference.sh`, invoked through an explicitly named maintainer
target, can replace them; it regenerates provenance and presents the diff for
review.

**Rationale**: The current frame size is manageable, PPM is already emitted,
and exact files allow direct visual inspection and byte-level diagnostics
without a new image or hash dependency.

**Alternatives considered**:

- Store only SHA-256 values: rejected because it proves identity but makes a
  visual change opaque during review.
- Add PNG encoding and image-diff libraries: rejected because exact PPM output
  already satisfies C0 and new codecs do not improve fidelity evidence.
- Auto-update on mismatch: rejected because a defect could bless itself.

## Decision 4: Extend the clean-room fixture for named bitmap and Mode 7 workloads

**Decision**: Extend the existing redistributable demo-ROM generator with named
deterministic workloads and use the evidence tool to record requested cycles,
actual completed-instruction cycles, CPU state, frame metadata, and output.

**Rationale**: The existing generator is lawful and deterministic. The machine
finishes the current instruction when running for a requested cycle budget, so
both requested and actual cycle counts are contractually significant.

**Observed starting signature**:

- generated ROM SHA-256:
  `8e83440f60a5286714b98ac12444ab21ffd27894b9241d090b7c6ae10262f24e`
- requested cycles: `100000`
- actual state: `PC=$C0AF A=$00 X=$1E Y=$00 SP=$FF P=$26 cycles=100009`
- frame: `12`, `480x500`, ROM bank `0`
- PPM SHA-256:
  `c4c9884af9187ab1178f63480962b6921b98a87e1e674371c40904d505fcc994`

These values are research observations, not approved references until the
implementation's fixture provenance and ten-run reproducibility evidence pass.

## Decision 5: Keep performance evidence descriptive and separate

**Decision**: `make measure-c0` runs a named workload at least five times and
writes a structured text record containing workload identity, source revision,
toolchain/OS/CPU context, individual elapsed-time and emulated-cycle samples,
median, range, validity, and an explicit non-guarantee label.

**Rationale**: Later phases need comparable throughput evidence, but a local
wall-clock number is not a compatibility test or a product performance promise.
Separating measurement from correctness avoids making variable performance a
normal verification failure.

**Alternatives considered**:

- Set a CI speed threshold in C0: rejected because shared runners and hardware
  differ, and no user-facing performance objective has been established.
- Add a benchmarking library: rejected because the existing harness plus a
  strict script can collect the required five-sample record.

## Decision 6: Use Doxygen for C/C++ and DocC for Swift

**Decision**: Use Doxygen for `Sources/BeebCore/include` and selected complex
implementation documentation, and Swift-DocC through the official
`swiftlang/swift-docc-plugin` for `BeebKit`. `scripts/build-docs.sh` generates
both static sites when their supported toolchains are present and creates one
`.build/docs/index.html` landing page. It can run a C/C++-only profile on Linux;
the macOS CI documentation gate builds the complete site.

**Rationale**: Doxygen produces cross-referenced C/C++ documentation, accepts
Markdown and `///` source comments, links declarations to source, and can turn
documentation warnings into failures. DocC is Swift's native system for source
comments, symbol relationships, conceptual Markdown, and distributable static
archives. A thin landing page gives one entry point without forcing either
language through an inferior parser.

**Configuration**:

- Doxygen: `EXTRACT_ALL=NO`, warnings enabled, incomplete/parameter
  documentation warnings enabled, and `WARN_AS_ERROR=FAIL_ON_WARNINGS`.
- DocC: generate documentation for `BeebKit`, transform for static hosting, and
  treat unresolved documentation links and warnings as quality failures.
- Comment form: `///` for public declarations and concise adjacent rationale;
  larger topics use Markdown guides linked with stable names.
- Generated output: `.build/docs/cpp/`, `.build/docs/swift/`, and a generated
  landing page; none is committed.

**Primary references**:

- [Doxygen documentation blocks](https://www.doxygen.nl/manual/docblocks.html)
- [Doxygen configuration and warning controls](https://www.doxygen.nl/manual/config.html)
- [Doxygen feature overview](https://www.doxygen.nl/manual/features.html)
- [Swift-DocC overview](https://www.swift.org/documentation/docc/)
- [Distributing DocC documentation](https://www.swift.org/documentation/docc/distributing-documentation-to-other-developers)

**Alternatives considered**:

- Doxygen for Swift too: rejected because it cannot match DocC's native Swift
  symbol graph and package integration.
- DocC for C/C++: rejected because C/C++ is not its supported source model.
- Handwritten Markdown only: rejected because it cannot enforce public symbol
  coverage or remain reliably linked to declarations.
- Graphviz call graphs: deferred because architecture and timing guides are more
  durable and no current question requires generated call graphs.

## Decision 7: Use a public-surface baseline and changed-code ratchet

**Decision**: C0 documents all supported public C ABI, Swift wrapper, and C++
header surfaces. It also documents representative complex components—CPU
execution/timing, machine/device advancement, host boundary, and evidence
model. Any remaining unchanged internal debt is recorded in
`Tests/Fixtures/C0/documentation-debt.txt` with a stable identifier and scope.
New or changed public or complex code must be documented and cannot add debt.

**Rationale**: Requiring documentation of all public contracts immediately is
bounded and valuable. Requiring prose on every private helper would create
noise and enlarge C0; ignoring debt would allow later phases to evade it.

**Alternatives considered**:

- Require documentation for every symbol: rejected because it rewards volume
  rather than understanding and creates comments that mirror code.
- Validate only that docs build: rejected because syntactically valid empty
  coverage can still leave public contracts unexplained.
- Permit an unbounded baseline file: rejected because additions would hide new
  debt; the check must compare against the reviewed baseline and reject growth.

## Decision 8: Enforce the strategy through Spec Kit and CI

**Decision**: The constitution, feature specification, plan, tasks, checklist,
core roadmap, and CI all carry the documentation requirement. Every coding
feature names documentation impact or gives a concrete N/A. Tasks update docs
with their code, and final verification runs `make docs-check`.

**Rationale**: A style guide alone decays. Enforcement belongs at requirements,
design, implementation, and integration boundaries.

**Alternatives considered**:

- Rely on reviewer memory: rejected because it is inconsistent across phases.
- Add docs only as a final polish task: rejected because contract and invariant
  explanations must be authored alongside the code they describe.

## Current Verification Baseline

Repository inspection and fresh local runs established the planning baseline:

- `make test`: 26 of 26 tests passed.
- `make sanitize`: 23 of 23 non-exhaustive tests passed.
- `swift test`: 4 of 4 tests passed.
- Existing CI runs portable Make targets on Linux and Swift targets on macOS,
  but has no aggregate baseline, exact golden-reference, measurement, or
  generated-documentation gate.
- The current tests include recoverable C-boundary errors and representative
  bitmap/Mode 7 assertions, but not exact end-to-end approved frames.

## Resolved Unknowns

No product or scope clarification remains. During implementation, the exact
official Swift-DocC plugin release is selected from its release history,
verified against Swift tools 5.9 and the current CI Swift toolchain, and pinned
in `Package.swift`/`Package.resolved`; this is a dependency hygiene task, not an
open architecture choice.
