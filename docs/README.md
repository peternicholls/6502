# Documentation index

This directory has one forward programme direction with supporting product,
core, evidence, architecture and historical documents.

## Sole delivery authority

The [Machine application delivery plan](product/MACHINE_DELIVERY_PLAN.md) is the
explicit and only authority for next work, ordering, priority, delivery gates,
machine-profile commitments and promotion of future options. Every new Spec Kit
feature starts there. No other document may create a parallel roadmap.

Its [dated delivery status ledger](product/MACHINE_DELIVERY_PLAN.md#delivery-status--2026-07-19)
is the required starting point for distinguishing work **DONE** from work
**ACTIVE**, **NEXT**, **TODO** or merely **RESERVED**.

## Product strand

The [product documentation](product/README.md) defines who the application
serves and the Machine, Media and Editor experiences:

- [Product vision](product/VISION.md) — durable intent and principles only.
- [Product capability catalogue](product/ROADMAP.md) — possible user-facing
  scope without delivery order.
- [Legacy decision register](product/LEGACY_DECISIONS.md) — interpretation of
  historical material.

## Emulator core strand

The core strand defines the reusable machine foundation independently of the
wider application's UX and commercial aspirations:

- [Core phase catalogue](CORE_ROADMAP.md) — technical dependencies,
  invariants and bounded decompositions for selected delivery-plan slices.
- [Implementation status](STATUS.md) — verified current behavior and fidelity
  gaps.
- [Architecture](ARCHITECTURE.md) — current boundaries and timing direction.
- [References](REFERENCES.md) — primary technical sources and legal boundaries.

The delivery plan may select an outcome that needs product and core work. The
supporting documents explain intent, boundaries and evidence but cannot change
programme direction. `STATUS.md` alone records verified implementation.

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

- Delivery order, priority, gates and committed profiles change only in
  `product/MACHINE_DELIVERY_PLAN.md`.
- Durable product intent changes belong in `product/VISION.md` but do not enter
  delivery until the plan is amended.
- Product capability descriptions belong in `product/ROADMAP.md`; technical
  dependencies and invariants belong in `CORE_ROADMAP.md`. Neither selects
  work.
- Completed or verified capability changes belong in `STATUS.md` and the
  changelog.
- Technical boundary changes belong in `ARCHITECTURE.md` and should identify
  the product capability they enable.
- A legacy idea adopted by the current project must first receive an explicit
  disposition in `product/LEGACY_DECISIONS.md`.
- Avoid duplicating delivery requirements or sequencing; link to the Machine
  application delivery plan instead.
