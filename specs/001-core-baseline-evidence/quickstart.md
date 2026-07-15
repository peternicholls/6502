# Quickstart: Implement and Verify C0

This is the executable acceptance path the implementation must make valid. Run
from the repository root on the feature branch.

## 1. Confirm the behavioral baseline

```bash
make test
make sanitize
swift test
swift build
```

Expected C0 result: 27 C++ tests, 24 sanitizer quick-profile tests, and seven
Swift XCTest cases pass, and the package builds. The pre-C0 observation was
26/23/four; that historical comparison remains in `docs/CORE_BASELINE.md`.

## 2. Run all C0 evidence

```bash
make verify-c0
```

Expected: every required group is listed, detailed logs are under
`.build/c0/run/`, the final result is successful, and
`Tests/Fixtures/C0/` remains unchanged.

On Linux, Swift-specific groups may be explicitly not applicable. On the macOS
CI profile, all C++, C, Swift, reference, and documentation groups must pass.

## 3. Verify fixture determinism and immutable references

```bash
Tests/C0/test-fixture-evidence.sh
git diff --exit-code -- Tests/Fixtures/C0
```

Expected: ten generated runs have identical named boot state, bitmap PPM, and
Mode 7 PPM signatures. A negative test alters a candidate by one byte, observes
a failed comparison naming expected and observed evidence, and confirms the
approved reference was not overwritten.

Reference replacement is never part of normal verification. For an intentional
future behavior change, a maintainer uses the explicit update flow and reason:

```bash
scripts/update-c0-reference.sh --reference bitmap-reference \
  --reason "intentional bitmap behavior change"
git diff -- Tests/Fixtures/C0
```

Valid IDs are `bitmap-reference`, `mode7-reference`, and `mode7-state`. The
command intentionally changes tracked evidence. The safe updater contract and
its expected failures are validated without modifying approved files with:

```bash
Tests/C0/test-reference-update.sh
```

## 4. Record comparison performance

```bash
make measure-c0
Tests/C0/test-measurement-record.sh
```

Expected: a record under `.build/c0/measurements/` contains at least five raw
samples, median, minimum, maximum, workload/revision/environment identity, and
the descriptive non-guarantee label. The measurement contract test must also
prove that four samples, zero duration, missing environment, and interruption
produce invalid records.

## 5. Generate and inspect code documentation

```bash
make docs
make docs-check
test -f .build/docs/index.html
```

Expected on macOS: the landing page links to C/C++ and C ABI Doxygen output,
Swift `BeebKit` DocC output, and conceptual architecture/timing/host-boundary/
evidence guides. On a portable Linux profile, the C/C++ site and applicable
conceptual links are complete and the landing page accurately marks Swift docs
as requiring the macOS documentation profile.

Review representative pages for:

- `CPU6502` execution purpose and timing invariants;
- `BBCMicro` ownership and device-advance behavior;
- C ABI machine/frame/error ownership and lifetime;
- Swift `BeebMachine` threading and failure contracts; and
- the timing-model and evidence/testing guides.

Then run `Tests/C0/test-documentation.sh`, which uses a temporary fixture to
prove that an undocumented changed public symbol, invalid markup, broken link,
and increased debt each fail without modifying production source.

Open `.build/docs/index.html` in a browser for the required representative-page
review; that manual presentation step is intentionally not a headless gate.

## 6. Final repository gates

```bash
make test
make sanitize
swift test
swift build
make verify-c0
make docs-check
make test-c0
git diff --check
```

Before C0 is complete, update `docs/STATUS.md`, `docs/ARCHITECTURE.md`,
`docs/CORE_ROADMAP.md`, and `CHANGELOG.md` with only the evidence actually
delivered. Generated `.build/` content must remain untracked.
