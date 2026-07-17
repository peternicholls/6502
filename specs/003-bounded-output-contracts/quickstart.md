# C2 Bounded Output Quickstart

Run from `/Users/peternicholls/Dev/6502` on a clean worktree.

## Focused contract evidence

```sh
make test
bash Tests/C2/test-output-contract.sh
bash Tests/C2/test-output-lifetime.sh
bash Tests/C2/test-output-replay.sh
bash Tests/C2/test-output-races.sh
bash Tests/C2/test-output-measurement.sh
swift test
```

The focused scripts must cover empty, full, sustained production, retained
owned-value lifetime, lifecycle failure, deterministic replay, and concurrent C/Swift
consumer pressure. Each script records its exact count and failure status.

## Bounded-production measurement

Use the named deterministic workload for at least 60 emulated seconds after a
10-second warm-up, plus a 10,000-item queue stress fixture. Record maximum frame
depth (must be at most 3), maximum audio depth (must be at most 4,096), exact
produced = consumed + dropped + retained accounting, and process RSS growth
(must be at most 16 MiB after warm-up). Retained outputs must remain unchanged.
The result belongs in `.build/c2/` and is not committed.

Verify synthetic host observations calculate the expected emulation-rate ratio
within 0.1% and that taking observations does not alter core state.

## Xcode project elevation

```sh
xcodebuild -project Beeb6502.xcodeproj -list
xcodebuild -project Beeb6502.xcodeproj -scheme BeebDemo-macOS -destination 'platform=macOS' build
xcodebuild -project Beeb6502.xcodeproj -scheme BeebDemo-iOS -destination 'generic/platform=iOS Simulator' CODE_SIGNING_ALLOWED=NO build
xcodebuild -project Beeb6502.xcodeproj -scheme Beeb6502-Tests -destination 'platform=macOS' test
swift build
swift test
```

The project and shared schemes must work from a clean checkout without
`xcuserdata`, an absolute checkout path, signing credentials, or existing
derived data. Swift Package Manager remains independently usable.

## Wider validation

```sh
make sanitize
make thread-sanitize
make format-check
swift build
make docs-check
bash Tests/C2/test-documentation.sh
git diff --check
```

If local ThreadSanitizer is unavailable, report `N/A` and require the
supported CI lane as C1 did; do not treat that as a local pass.
