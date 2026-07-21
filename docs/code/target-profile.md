# Machine Target Profiles

The machine target profile is the immutable identity requested when a runtime
is created. It answers which machine and optional components the caller asked
for; it does not claim that this release implements that machine. Keeping
identity separate from support prevents an unavailable or invalid request from
silently acquiring Model B behavior.

## Assigned identity and support

Schema version 1 assigns two permanent base identifier/version pairs:

| Base identity | Identifier | Component version | Support |
| --- | --- | --- | --- |
| BBC Microcomputer Model B | `0x00000001` | 1 | supported |
| BBC Model B+ 64K | `0x00000002` | 1 | recognised but unavailable |

Assigned pairs are never reused or reinterpreted. Model B+ 64K can therefore
cross C++, C, Swift and the application as a distinct identity even though its
machine behavior is not implemented. Only canonical Model B construction can
publish a runtime in this release. Model B+ 64K produces an explicit
recognised-unavailable rejection and never falls back to Model B.

No expansion identifier is assigned by this feature. B+ 128K, Master-family,
Tube, Econet, ADFS, storage and peripheral options are delivery-plan names, not
public profile constants. A non-zero raw fixture labelled with one of those
names remains `unknown`; transporting its raw value neither reserves that code
nor promises future support, picker exposure or compatibility.

Assigned base identifiers retain their human-readable Model B or Model B+ 64K
name even when the surrounding schema, version or expansions make the complete
profile invalid. The typed support category and diagnostic explain that
rejection without obscuring which assigned base the caller requested.

## Bounded canonical envelope

The version-1 value owns one base component, a declared expansion count and
exactly sixteen fixed expansion slots. Identifiers are unsigned 32-bit values;
component and schema versions are unsigned 16-bit values. A canonical envelope
has non-zero schema, identifier and component versions, zero reserved fields,
strictly ascending identifier/version expansion pairs without duplicates, and
zero-filled unused slots.

The validator reads the declared count before obtaining or indexing expansion
storage. Counts 0 through 16 are structurally admissible. A count above 16 is `malformed` without inspecting any expansion slot. The validator never
truncates, sorts, deduplicates, clears reserved fields or otherwise normalizes a
caller value; preserving raw identity is necessary for specific rejection and
forward-safe diagnostics.

## Classification and diagnostics

Validation is pure and uses one deterministic precedence for a multi-defect
value:

1. `malformed`: zero required fields, count above 16, non-zero reserved or
   unused fields, or non-canonical expansion ordering and duplicates;
2. `unknown`: a structurally usable schema, identifier or component version has
   no assigned contract;
3. `incompatible`: assigned components appear in an invalid role or
   combination, including a base identifier in an expansion slot;
4. `recognisedUnavailable`: canonical Model B+ 64K;
5. `supported`: canonical Model B.

Every non-supported result owns an actionable diagnostic. Unknown-identifier
diagnostics include the raw hexadecimal value and do not invent a name from the
delivery plan. Support is deliberately not part of identity: a later release
may implement an already assigned profile without changing its permanent pair.

## Failure atomicity across boundaries

`BBCMicro` validates before device wiring, memory initialization or reset.
`MachineRuntime` validates before allocating its implementation or starting the
owner thread. Failed construction therefore cannot expose partially initialized
machine state.

The C validator copies a successful classification into caller-owned storage.
Nulls and adapter failures leave that output untouched. C construction maps
malformed, unknown and incompatible values to `INVALID_ARGUMENT`, maps canonical
Model B+ 64K to `UNAVAILABLE`, and writes an opaque handle only after supported
Model B construction and registry publication both succeed.

Swift copies the raw profile and C diagnostic into owned values. Its malformed,
unknown, incompatible and recognised-unavailable errors retain the original
candidate. The application constructs a candidate before replacing its running
machine, so any rejection leaves the active profile, safe point and emulated
state unchanged. The no-argument convenience constructor means explicitly
"canonical Model B"; it is never a recovery path for a rejected explicit
profile.

## In-memory value, not persistence

The C and C++ aggregates are in-process semantic carriers, not a serialized or persisted byte format. Their padding, byte order and physical layout have no
storage contract. This feature defines no snapshot encoding, preference
encoding, truncation behavior or trailing-byte rule. A later snapshot-envelope
feature must define those rules explicitly and encode the semantic fields; it
must not persist raw object bytes or the release-dependent support result.

When extending the profile contract, allocate permanent identifiers through the
delivery programme, add canonical values and role rules, preserve the existing
classification precedence, and prove failure-output preservation at C and
owned error mapping in Swift. More than sixteen expansions requires a new
schema version and an explicit compatibility decision.

Post-acceptance observations that may inform later, separately selected work are
recorded in the non-authoritative
`docs/reviews/target-profile-engineering-review.md`.
