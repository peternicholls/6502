# Contract: Code Documentation

## Audience and information levels

1. **Public contract comments** answer what a caller may rely on: purpose,
   inputs/results, ownership/lifetime/nullability, errors, threading, side
   effects, and invariants where applicable.
2. **Implementation rationale** answers why non-obvious emulator code has its
   shape: hardware behavior, timing model, observable effect, reference, and
   invariant. It does not paraphrase statements or repeat names and types.
3. **Conceptual guides** explain relationships spanning symbols or languages:
   architecture, emulated-time/device advancement, public host boundary,
   evidence and testing, and how to navigate the generated reference.
4. **Private/internal named abstractions** explain a class, struct, protocol,
   interface, enum, or important type alias to future maintainers: its purpose,
   responsibility boundary, important invariants, and applicable ownership,
   lifetime, threading, collaboration, or extension constraints.

## Source conventions

- Use `///` comments on supported C++, C, and Swift declarations.
- Prefer a short summary sentence, then contract details only when applicable.
- Put public contracts in public declarations, not solely in implementation
  files. Avoid copying the same prose into declaration and definition.
- Link complex code to one authoritative guide instead of duplicating a large
  explanation.
- Include primary hardware/source references when behavior depends on one.
- Remove or rewrite stale comments in the same change as the behavior.
- Document private/internal named types and interfaces, but do not document
  private getters, trivial delegators, obvious loops, or members merely to
  increase a coverage number.

## Generation

- `make docs`: generate all documentation supported by the current host into
  `.build/docs/` and create `.build/docs/index.html`.
- `make docs-check`: perform strict C/C++ generation everywhere and complete
  C/C++ plus Swift generation on the macOS profile; validate landing and
  internal links; validate the documentation-debt ratchet.
- Doxygen output: `.build/docs/cpp/index.html`.
- DocC static output: `.build/docs/swift/index.html` (or a deterministic
  equivalent linked by the landing page).
- Conceptual guides are included in generated navigation rather than copied as
  an unrelated third site.

## Initial required coverage

- Every supported declaration in `Sources/BeebCore/include/beeb_c.h`.
- Every supported public declaration in `Sources/BeebKit/`.
- Every public C++ type/function in `Sources/BeebCore/include/beeb/`, or an
  explicit internal-only classification in the C0 coverage manifest.
- Focused explanations for CPU execution, aggregate instruction/device timing,
  machine ownership/boundaries, exact frame evidence, and the C-to-Swift error
  and lifetime model.

## Quality gate

The gate fails on:

- malformed documentation markup or generator warnings configured as errors;
- unresolved internal symbol/topic links;
- a missing landing-page destination for an output claimed as built;
- a new or changed required public surface without contract documentation;
- a new or changed private/internal named abstraction without useful developer
  documentation;
- a changed complex surface with neither useful rationale nor a reviewed N/A;
- any addition or scope expansion in the debt baseline; or
- generated documentation checked into an authoritative source location.

The gate does not fail solely because an unchanged private helper lacks a
comment when that scope is either clear from code or recorded as existing debt.

## Feature-workflow requirement

Every later coding feature must carry the following trace:

`spec documentation impact -> plan generator/source/debt decision -> story task
beside code -> make docs-check -> generated-page review`

A documentation N/A is valid only when no public contract, private/internal
named abstraction, non-obvious behavior, or conceptual relationship changes,
and the feature artifact states why.
