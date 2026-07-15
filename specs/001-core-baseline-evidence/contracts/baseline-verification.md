# Contract: C0 Baseline Verification

## Entry points

- `make verify-c0`: complete profile for the current supported host.
- `scripts/verify-c0.sh [--profile portable|macos|all]`: script-level entry for
  tests and CI. Default profile is detected conservatively.

## Required behavior

1. Remove and recreate only `.build/c0/run/`.
2. Record source revision, dirty-tree state, environment, and selected profile.
3. Execute every applicable required evidence group even after a group fails.
4. Capture detailed output separately for each group.
5. Print an ordered plain-text summary containing group ID, status, and detail
   path; failures also contain a short diagnosis.
6. Exit zero only when every applicable required group passes and every
   non-applicable group is explicitly permitted by the selected profile.
7. Never write `Tests/Fixtures/C0/`.

## Initial evidence groups

| ID | Portable | macOS | Proof |
| --- | --- | --- | --- |
| `cpp-behavior` | required | required | `make test` |
| `sanitizers` | required | required | `make sanitize` |
| `version-sync` | required | required | release version consistency |
| `c-boundary` | required | required | recoverable negative C ABI tests |
| `swift-boundary` | N/A | required | `swift test` and `swift build` |
| `fixture-provenance` | required | required | complete lawful manifest |
| `cleanroom-boot` | required | required | exact approved state |
| `bitmap-reference` | required | required | byte-exact PPM |
| `mode7-reference` | required | required | byte-exact PPM |
| `cpp-docs` | required | required | Doxygen coverage/markup/link gate |
| `swift-docs` | N/A | required | DocC warning/link gate |
| `docs-landing` | partial | required | generated links resolve for built sites |

## Failure contract

- Missing required tool: `unexpected-skip`, overall failure, installation hint.
- Command failure: `fail`, retain captured output and command.
- Reference mismatch: `fail`, report expected and observed digest plus paths;
  do not modify either file.
- Unsupported requested profile: usage failure before group execution.
- Signal/interruption: retain generated details, mark incomplete, exit nonzero.

Diagnostic content must be readable without ANSI colour. Colour may be an
optional enhancement only when disabled automatically for non-terminal output.
