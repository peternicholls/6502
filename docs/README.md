# Documentation

This directory separates current authority, current engineering reference,
completed evidence and historical material.

## Start here

| Need | Document | Authority |
| --- | --- | --- |
| What happens next | [Machine delivery plan](product/MACHINE_DELIVERY_PLAN.md) | Sole forward programme authority |
| What works now | [Implementation status](STATUS.md) | Sole verified-state authority |
| How the system is divided | [Architecture](ARCHITECTURE.md) | Current boundary contract |
| What future core work must preserve | [Implementation constraints](IMPLEMENTATION_CONSTRAINTS.md) | Technical constraints; no priority or schedule |
| Why the product exists | [Product vision](product/VISION.md) | Durable intent; no delivery commitments |

No other document may select work, claim completion or redefine a delivery
gate.

## Engineering reference

- [Code documentation standard](CODE_DOCUMENTATION.md)
- [Conceptual code guides](code/)
- [Primary references and legal boundaries](REFERENCES.md)
- [Release procedure](RELEASING.md)
- [Project constitution](../.specify/memory/constitution.md)
- [Spec Kit workflow](../specs/README.md)

## Separated history

- [Completed evidence](completed/) contains verified C0-C2 ledgers and links to
  frozen completed feature runs. It cannot select future work.
- [Archive](Archive/) contains superseded planning and abandoned-project
  material. It is research only.

## Change rule

Update one owner only:

- direction or gates: `product/MACHINE_DELIVERY_PLAN.md`;
- verified behavior: `STATUS.md`;
- current boundaries: `ARCHITECTURE.md`;
- durable product intent: `product/VISION.md`;
- future technical invariants: `IMPLEMENTATION_CONSTRAINTS.md`;
- completed evidence: `completed/`; and
- superseded material: `Archive/`.

Link to the owner instead of repeating its prose.
