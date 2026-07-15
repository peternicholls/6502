# Quickstart: Implement and Verify C1

Run from the repository root on `002-runtime-ownership`.

## 1. Baseline

```bash
make test
make sanitize
swift test
swift build
make verify-c0
```

## 2. Focused runtime contracts

```bash
Tests/C1/test-runtime-contract.sh
Tests/C1/test-runtime-replay.sh
```

Expected: every state/command matrix cell passes; ten ledger replays produce
identical state, frame, cycle, and safe-point signatures.

## 3. Race and shutdown evidence

```bash
Tests/C1/test-runtime-races.sh
```

Expected: the supported ThreadSanitizer build and mixed 10,000-command stress
finish without race, deadlock, lost accepted work, stale diagnostic, or
use-after-destroy report. Unsupported toolchains must report N/A explicitly;
the supported CI profile must run it.

## 4. Public boundaries and docs

```bash
make test
swift test
swift build
make docs-check
Tests/C1/test-documentation.sh
```

Inspect generated runtime, C status, and Swift error pages plus
`docs/code/runtime-ownership.md` links.

## 5. Exit gates

```bash
make test
make sanitize
make thread-sanitize
swift test
swift build
make verify-c0
make test-c1
make docs-check
git diff --check
```

Before C1 completion, synchronize `VERSION`/compiled version/`CHANGELOG.md` at
0.2.0 and update `docs/STATUS.md`, `docs/ARCHITECTURE.md`, and
`docs/CORE_ROADMAP.md` only with verified results. Generated `.build/` content
remains untracked.
