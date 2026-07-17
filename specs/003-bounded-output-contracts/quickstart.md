# C2 Bounded Output Quickstart

Run from `/Users/peternicholls/Dev/6502` on a clean worktree.

## Focused contract evidence

```sh
make test
bash Tests/C2/test-output-contract.sh
bash Tests/C2/test-output-lifetime.sh
bash Tests/C2/test-output-replay.sh
bash Tests/C2/test-output-races.sh
swift test
```

The focused scripts must cover empty, full, sustained production, retained
view lifetime, lifecycle failure, deterministic replay, and concurrent C/Swift
consumer pressure. Each script records its exact count and failure status.

## Bounded-production measurement

Use the named deterministic workload from the C2 test fixture for at least 60
emulated seconds and 10,000 completed frames. Record configured capacities,
maximum observed queue depth, produced/consumed counts, pressure counts, and
whether any invalid view or allocation growth occurred. The result belongs in
`.build/c2/` and is not committed.

## Wider validation

```sh
make sanitize
swift build
make docs-check
bash Tests/C2/test-documentation.sh
git diff --check
```

If local ThreadSanitizer is unavailable, report `N/A` and require the
supported CI lane as C1 did; do not treat that as a local pass.
