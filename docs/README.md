# Documentation index

This directory has two connected but independently governed strands: the wider
product and the portable emulator core.

## Product strand

The [product documentation](product/README.md) defines who the application
serves and the Machine, Media and Editor experiences:

- [Product vision](product/VISION.md)
- [Product roadmap](product/ROADMAP.md)
- [Legacy decision register](product/LEGACY_DECISIONS.md)

## Emulator core strand

The core strand defines the reusable machine foundation independently of the
wider application's UX and commercial aspirations:

- [Core roadmap](CORE_ROADMAP.md) — technical capability sequencing.
- [Implementation status](STATUS.md) — verified current behavior and fidelity
  gaps.
- [Architecture](ARCHITECTURE.md) — current boundaries and timing direction.
- [References](REFERENCES.md) — primary technical sources and legal boundaries.

The product strand may request an outcome from the core. The core strand owns
how that outcome is safely implemented. Neither roadmap can claim completion;
`STATUS.md` requires implementation evidence.

## Project operations

- [Project constitution](../.specify/memory/constitution.md) defines the
  non-negotiable architecture, evidence and delivery rules.
- [Feature specifications](../specs/README.md) explains how product, core and
  cross-strand slices move through Spec Kit.
- [Releasing](RELEASING.md) defines versioning and release procedure.
- [Changelog](../CHANGELOG.md) records user- and developer-visible changes.

## Historical material

[Product Design Documents Archive](Archive/) contains design work
imported from an abandoned version of the project. It remains useful research,
but it is not an implementation specification. Consult the legacy decision
register before carrying any of its technical decisions forward.

## Change rules

- Product intent changes belong in `product/VISION.md`.
- User-facing priority changes belong in `product/ROADMAP.md`.
- Emulator technical priority changes belong in `CORE_ROADMAP.md`.
- Completed or verified capability changes belong in `STATUS.md` and the
  changelog.
- Technical boundary changes belong in `ARCHITECTURE.md` and should identify
  the product capability they enable.
- A legacy idea adopted by the current project must first receive an explicit
  disposition in `product/LEGACY_DECISIONS.md`.
- Avoid duplicating detailed requirements across documents; link to the
  authoritative source instead.
