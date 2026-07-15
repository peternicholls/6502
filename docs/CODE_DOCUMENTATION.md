# Code Documentation Standard

Code documentation is part of the supported contract. It should let a new
contributor answer what a symbol guarantees, why non-obvious emulator logic has
its shape, and where a cross-component decision is explained—without turning
the source into a line-by-line translation.

Generated pages are a disposable view of tracked source comments and guides.
They live under `.build/docs/`; never edit or commit them.

## Choose the right information level

| Surface | Required detail | Keep elsewhere |
| --- | --- | --- |
| Public C ABI | Purpose; ownership and lifetime; nullability; inputs and return/failure values; error channel; threading; side effects; borrowed-buffer validity | C++ implementation mechanics |
| Public C++ | Purpose; parameter and return semantics; ownership/borrowing; exceptions; mutation and timing effects; invariants a caller relies on | Obvious type information and private helper narration |
| Public Swift | Purpose; ownership; concurrency/actor expectation; validation; thrown errors; value or buffer-copy behavior | Repeated C ABI prose |
| Non-obvious implementation | Hardware behavior; timing or transition invariant; observable effect; authoritative guide/reference | A paraphrase of each statement |
| Conceptual guide | Relationships across symbols/languages, one authoritative model, extension constraints | Complete API inventories |

Use `///` on supported C, C++, and Swift declarations. Start with one summary
sentence. Add only the contract dimensions that apply. Doxygen commands use
`@param`, `@return`, and `@throws`; Swift uses DocC parameter, return, and throws
sections.

## Public contract example

Good—this says what the types cannot:

```cpp
/// Copies a disc image into one drive.
/// @param bytes Complete image bytes; the span is not retained.
/// @param drive Drive 0 or 1.
/// @return Whether both drive and image geometry were valid.
bool mount(std::span<const std::uint8_t> bytes, unsigned drive);
```

Bad—this merely renames the function:

```cpp
/// Mounts bytes on a drive.
bool mount(std::span<const std::uint8_t> bytes, unsigned drive);
```

For a borrowed buffer, state the invalidation boundary. For a callback, state
who owns it and when it may run. For a throwing operation, name meaningful
failure categories. Do not promise thread safety unless the implementation
provides it.

## Implementation rationale example

Use a short link beside code whose shape comes from timing, hardware behavior,
state ordering, or a lifetime constraint:

```cpp
// C0-DOC-RATIONALE: docs/code/timing-model.md owns the clock-rate and
// remainder invariant used by this aggregation.
```

Avoid comments such as `// increment the counter`. Self-evident getters,
delegators, loops, and private storage do not need comments solely to raise a
coverage number.

An implementation rationale should answer at least two of these questions:

- Which hardware or boundary behavior requires this shape?
- What observable result or timing changes if the order changes?
- Which invariant must be preserved?
- Which focused guide or primary reference owns the full explanation?

## Conceptual guide ownership

- `docs/code/architecture.md`: layer direction and state ownership.
- `docs/code/timing-model.md`: instruction transitions and device clock domains.
- `docs/code/host-boundary.md`: C and Swift lifetime, errors, and synchronization.
- `docs/code/evidence-and-testing.md`: observable evidence and reference review.

Update one authoritative guide and link to it. Do not duplicate a long hardware
explanation across declarations and source files.

## Local validation

```sh
make docs
make docs-check
Tests/C0/test-documentation.sh
```

`make docs` builds everything supported by the current host. `make docs-check`
uses the strict profile: Doxygen everywhere and Doxygen plus DocC on macOS. The
gate fails for malformed markup, unresolved links, missing required public
contracts, changed complex code without a decision, or documentation-debt
growth. Open `.build/docs/index.html` and inspect representative pages after a
meaningful documentation change.

CI runs the portable profile on Linux and the complete profile on macOS. A
local portable success does not prove the Swift DocC site.

## Documentation impact in every coding feature

Carry one trace through Spec Kit artifacts:

`spec impact -> plan source/generator/debt decision -> task beside code -> docs-check -> rendered-page review`

A documentation `N/A` is valid only when the feature changes no public
contract, non-obvious behavior, or conceptual relationship. Record a concrete
reason in the feature artifact. For changed-file validation, the accepted form
is `path|N/A: reason`; `N/A` with no reason is not a decision.

## Debt ratchet

`Tests/Fixtures/C0/documentation-debt.txt` is an inventory of reviewed,
unchanged internal gaps, not permission to omit documentation from new work.
Its first line is the maximum baseline count:

```text
baseline_count=0
```

Any entry uses this stable form:

```text
DEBT-001|path-or-symbol|severity|repayment trigger|target phase
```

Adding an entry or broadening its scope fails the gate unless the baseline
change is separately reviewed as an explicit governance decision. Ordinary
feature work must keep the count level or reduce it. When touching an inventoried
surface, repay the entry in the same task if its trigger applies; remove the
entry and lower `baseline_count` together.

## Review checklist

- Is the public guarantee next to the declaration?
- Are ownership, lifetime, nullability, errors, concurrency, and side effects
  stated where applicable—not mechanically everywhere?
- Does non-obvious code link to a useful rationale rather than narrate syntax?
- Are links and parameter names valid in generated output?
- Is generated output still ignored and the debt count no higher?
- Did someone inspect the affected rendered pages, not only the exit status?
