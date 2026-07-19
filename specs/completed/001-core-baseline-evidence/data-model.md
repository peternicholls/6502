# Phase 1 Data Model: Core Baseline Evidence

C0 persists test evidence and documentation policy as files, not runtime
objects or user data. Formats are intentionally human-readable and strict
enough for scripts to validate.

## EvidenceGroup

Represents one required category in an aggregate baseline run.

| Field | Meaning | Rule |
| --- | --- | --- |
| `id` | Stable machine-readable name | Unique, lowercase kebab-case |
| `title` | Human-readable scope | Non-empty |
| `profile` | `portable`, `macos`, or `all` | Determines applicability only |
| `command` | Existing or C0 verification command | Recorded before execution |
| `status` | `pass`, `fail`, `unexpected-skip`, `not-applicable` | Only `pass` and declared `not-applicable` permit success |
| `detail_path` | Captured output under `.build/c0/` | Must not point at an approved reference |
| `failure_summary` | Stable text diagnosis | Required for fail/skip |

Required groups cover behavioral tests, sanitizers, version synchronization,
C ABI failures, Swift boundary behavior, clean-room boot, bitmap reference,
Mode 7 reference, fixture provenance, and documentation quality. Swift-specific
groups are declared not applicable only outside their supported profile.

## BaselineRun

Aggregate result produced by `verify-c0.sh`.

| Field | Meaning | Rule |
| --- | --- | --- |
| `schema` | Record format | `beeb-c0-baseline-v1` |
| `source_revision` | Git revision or explicit dirty-tree marker | Required |
| `environment` | OS, architecture, toolchain profile | Required |
| `started_at` | UTC start timestamp | Informational, not part of deterministic signature |
| `groups` | Ordered `EvidenceGroup` results | Every applicable required group exactly once |
| `overall` | `pass` or `fail` | Derived; never independently assigned |

### State transitions

`created -> running -> pass|fail`. Individual group failure does not stop the
run; the final state is `fail` if any required group is not successful.

## RedistributableFixture

| Field | Meaning | Rule |
| --- | --- | --- |
| `id` | Named workload | Stable across reference history |
| `source_paths` | Generator and tracked inputs | Repository-relative |
| `redistribution_basis` | Why all bytes may be distributed | Required prose |
| `generation_command` | Clean reproduction command | Must not use private media |
| `generator_version` | Source revision and fixture format | Required |
| `output_identity` | Size and cryptographic digest | Derived |
| `coverage` | Behavior intentionally exercised | Required; limitations explicit |

## ApprovedReference

| Field | Meaning | Rule |
| --- | --- | --- |
| `id` | Stable state/bitmap/Mode 7 identity | Unique |
| `fixture_id` | Source `RedistributableFixture` | Must resolve |
| `kind` | `state`, `bitmap-ppm`, `mode7-ppm` | Closed set for C0 |
| `path` | Tracked expected file | Under `Tests/Fixtures/C0/` |
| `requested_cycles` | Workload request | Required for execution evidence |
| `actual_cycles` | Completed-instruction result | Required and may exceed requested |
| `signature` | Exact state text or file digest | Derived from tracked content |
| `generation_command` | Explicit update command | Must differ from normal verify |
| `coverage` | What a match proves and does not prove | Required |
| `review_note` | Intended reason for last replacement | Required on update |

### Invariants

- Normal verification has no write path to approved references.
- State and image identity is exact, not tolerant or perceptual.
- A changed reference is invalid until provenance, ten-run determinism, and a
  human-reviewed diff are present.

## MeasurementRecord

| Field | Meaning | Rule |
| --- | --- | --- |
| `schema` | Record format | `beeb-c0-measurement-v1` |
| `label` | Interpretation | Must say descriptive comparison, not guarantee |
| `workload_id` | Named fixture/workload | Required and resolvable |
| `source_revision` | Code identity | Required |
| `environment` | OS, architecture, CPU, compiler, build mode | All required |
| `requested_cycles` | Per-sample workload | Positive integer |
| `samples` | Elapsed time and actual emulated cycles | At least five complete samples |
| `median` | Median throughput and elapsed time | Derived |
| `range` | Minimum and maximum | Derived |
| `validity` | `valid` or `invalid` plus reasons | Derived |

Interrupted, zero-duration, incomplete, or under-sampled records are invalid and
cannot replace the documented comparison baseline.

## DocumentationSurface

| Field | Meaning | Rule |
| --- | --- | --- |
| `id` | Stable symbol or topic identity | Generator-qualified where needed |
| `kind` | `public-api`, `complex-implementation`, `concept-guide` | Closed set for C0 |
| `language` | `cpp`, `c`, `swift`, or `cross-language` | Required |
| `source` | Header/source comment or Markdown guide | Tracked path |
| `audience` | Host integrator, core contributor, or both | Required |
| `required_detail` | Applicable contract/rationale fields | Must follow constitution principle VIII |
| `output` | Generated destination and anchor | Must resolve after generation |
| `status` | `covered`, `internal-only`, or `debt` | Public supported APIs cannot be `debt` at C0 exit |

### Required-detail rule

Public surfaces document purpose plus applicable parameters/results, ownership,
lifetime, nullability, errors, threading, side effects, and invariants. Complex
implementation surfaces explain hardware rationale, observable consequences,
and constraints. Self-evident mechanics are omitted.

## DocumentationDebtItem

| Field | Meaning | Rule |
| --- | --- | --- |
| `id` | Stable debt identifier | Unique and reviewable |
| `surface` | Exact unchanged internal scope | Required |
| `reason` | Why C0 can defer it | Must not be “no time” alone |
| `risk` | Effect on maintainability | `low`, `medium`, or `high` |
| `trigger` | Change that requires repayment | Required |
| `owner_phase` | Earliest likely phase | Advisory, not permission to increase debt |

### Ratchet

The validated baseline is a set of IDs and scopes. A docs check fails if an item
is added, broadened, or if changed code matches an item's trigger without that
item being removed and the surface documented. Removing debt is always valid.
