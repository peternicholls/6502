# Quickstart: Machine Target Profile

This is the implementation and acceptance path for the feature. Commands run
from the repository root on branch `004-machine-target-profile`.

## 1. Confirm the planning boundary

```bash
test "$(git branch --show-current)" = "004-machine-target-profile"
test "$(jq -r .feature_directory .specify/feature.json)" = "specs/004-machine-target-profile"
git status --short
```

Model B is the only constructible profile. Model B+ 64K must remain a distinct
recognised-but-unavailable identity. No proprietary firmware or media is
required for this feature.

## 2. Work test-first by boundary

For each task, add the focused assertion, run it and record that it fails for
the expected missing behavior before editing production code. Then implement
the smallest contract and rerun the focused gate. Commit that verified task in
Lore format before starting the next task.

The focused aggregate is:

```bash
make test-machine-target-profile
```

It must cover canonical values, malformed/unknown/incompatible values, B+
recognised-unavailable behavior, the exact 16-valid/17-malformed expansion
boundary, multi-defect classification precedence, no fallback, output-canary
preservation, C++/C/Swift round trips, runtime query ordering and unchanged
Model B replay. Unassigned future-option fixture codes remain local test data
and must not become public constants or user-facing names.

## 3. Run the wider automated gates

```bash
make test
make sanitize
swift test
swift build
make test-c1
make test-c2-portable
make test-c2-xcode
make format-check
DOCS_BASE=develop make docs-check
git diff --check
```

Local ThreadSanitizer remains `N/A` where the host/toolchain is unsupported;
strict supported Linux CI remains authoritative. A failure in any maintained
macOS, iOS Simulator or test build blocks acceptance even though direct UI
observation is macOS-only for this slice.

## 4. Build, launch and observe the application

Record the exact macOS version, Mac model/architecture, Xcode version, commit
and time in `specs/004-machine-target-profile/evidence/macos-application-observation.md`.

1. Build the checked-in `BeebDemo-macOS` scheme from a clean derived-data path.
2. Launch the built application normally.
3. With keyboard navigation enabled, focus the machine-profile selector.
4. Select `BBC Microcomputer Model B`.
5. Confirm the requested and active profile labels both say Model B and the
   application remains responsive.
6. Enable VoiceOver and use Accessibility Inspector to confirm the selector,
   selected value, active profile and status have stable identifiers and are
   announced meaningfully.
7. Select `BBC Model B+ 64K`.
8. Confirm the application reports that the identity is recognised but machine
   support is not yet available, retains the Model B active-profile label and
   does not start or label a fallback machine as B+.
9. Return to Model B and confirm recovery is possible entirely through the UI.

This Model B+ 64K selection is the feature's one unsupported application
selection. Do not add unknown or reserved future options to the picker merely
to satisfy automated fixture coverage.

Capture observed text and interaction results. Do not record a generic “looks
good”; discrepancies and known limits remain explicit. Unit tests, screenshots
or build success alone do not close this journey.

## 5. Close the feature only after acceptance

After every task and phase is committed and all evidence passes:

1. Update `docs/STATUS.md` only with verified target-profile behavior.
2. Advance `VERSION`, `Sources/BeebCore/include/beeb/version.h` and
   `CHANGELOG.md` together to development candidate 0.4.0; run
   `make check-version` and the Swift version test.
3. Change the selected row state in
   `docs/product/MACHINE_DELIVERY_PLAN.md` and expose the next row only if the
   delivery-plan gate is genuinely satisfied.
4. Clear `.specify/feature.json`.
5. Move `specs/004-machine-target-profile/` intact to `specs/completed/`.
6. Run the tooling, documentation and diff gates again and commit the non-empty
   phase-completion change.
