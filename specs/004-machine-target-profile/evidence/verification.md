# Machine Target Profile Verification

## Acceptance environment

- Completed: 2026-07-20 18:11 BST
- Source commit under test: `81c6f8a39098d5a3b492888015a43e5b8c708a4b`
- Host: Darwin 25.5.0, arm64
- C++ compiler: Apple clang 17.0.0 (`clang-1700.6.4.2`)
- Swift: 6.2.4 (`swiftlang-6.2.4.1.4`)
- Xcode: 26.3 (`17C519`)

## Required command results

| Command | Result |
| --- | --- |
| `make test-machine-target-profile` | PASS: aggregate-runner, application-build, C/C++/Swift boundaries, value contract and documentation groups |
| `make test` | PASS: 58/58 native tests, including all four target-profile tests and both C1 race tests |
| `make sanitize` | PASS: 53/53 quick tests under UndefinedBehaviorSanitizer; Darwin default is `SANITIZERS=undefined` |
| `swift test` | PASS: 21/21 BeebKit tests |
| `swift build` | PASS: complete debug package build |
| `make test-c1` | PASS: all six groups; normal race suite passed 10/10 repetitions, two race tests per repetition; local ThreadSanitizer N/A as described below |
| `make test-c2-portable` | PASS: all eight portable groups; 60-second measurement reported `valid=true`; local ThreadSanitizer N/A as described below |
| `make test-c2-xcode` | PASS: maintained Xcode project group |
| `make format-check` | PASS |
| `DOCS_BASE=develop make docs-check` | PASS: strict Doxygen and DocC generation plus branch-relative abstraction/rationale scan |
| `git diff --check` | WORKTREE EXCEPTION: correctly reported two trailing spaces in the unrelated, preserved user edit to `docs/REFERENCES.md` (lines 11 and 35) |
| `git diff --check develop...HEAD` | PASS: the complete committed feature range has no whitespace errors |

The worktree exception is not part of this feature and was not modified or
staged. The untracked user directory `docs/Original Guides and Manuals/` was
likewise preserved. Task-scoped staged diffs are checked again before every
feature commit.

## Bounded-envelope and precedence evidence

The C++/C matrix and Swift boundary test exercise the same owned raw values:

- an ordered declared count of 16 is structurally admissible and reaches
  `unknown` because this feature assigns no expansion identifiers;
- a declared count of 17 reaches `malformed` before expansion storage is
  obtained or indexed, while C and Swift preserve the raw declared count;
- duplicate, unsorted, reserved and non-zero unused fields reach `malformed`;
- an unassigned raw base or component version reaches `unknown`, and its
  hexadecimal identifier is retained in the diagnostic;
- assigned base identifiers in expansion roles reach `incompatible`;
- multi-defect fixtures prove `malformed` before `unknown`, `unknown` before
  `incompatible`, and `incompatible` before recognised-unavailable Model B+;
- canonical Model B+ 64K remains distinct and recognised unavailable, while
  canonical Model B remains the only supported construction.

Every rejected C construction preserved a non-null output canary byte for byte.
Swift errors retained the complete original candidate and owned diagnostic.
The pre-existing active Model B identity, CPU/safe-point observations and
machine digest remained unchanged. A 32-query concurrent C profile-read race
against destroy returned either an owned Model B value or a typed rejection
without modifying failed outputs.

## ThreadSanitizer disposition

Both local race scripts reported `N/A`, not PASS: ThreadSanitizer is not
supported by the Apple `c++` driver/runtime on this host. Their unsanitized
stress gates passed, including C1's ten repetitions and the C2 concurrent
producer/consumer gate. Supported Linux CI is authoritative: `.github/workflows/ci.yml`
runs on `ubuntu-latest` with `C1_REQUIRE_TSAN=1` for C1 and
`C2_REQUIRE_TSAN=1` for C2, so an unsupported or failing sanitizer is a hard CI
failure rather than a waived pass.

## Acceptance conclusion

The three stories pass together. Identity transport does not imply B+ machine
behavior; Model B+ and invalid values never fall back, never publish partial
construction, and never mutate the active Model B session. The remaining tasks
are the final rebuilt application observation, synchronized 0.4.0 completion
documents, and feature archival.
