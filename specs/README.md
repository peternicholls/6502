# Feature specifications

This directory contains current Spec Kit work only. There is no active feature:
`.specify/feature.json` is empty.

Completed C0-C2 runs are isolated under [`completed/`](completed/). They are
frozen evidence, not templates or forward authority.

## Start a feature

1. Select a **NEXT** or **TODO** row from the sole
   [Machine delivery plan](../docs/product/MACHINE_DELIVERY_PLAN.md).
2. Confirm its dependencies in the plan and
   [implementation constraints](../docs/IMPLEMENTATION_CONSTRAINTS.md).
3. Classify it as `product`, `core` or `cross-strand`.
4. Create one bounded feature directory with `/speckit-specify`.
5. Verify `.specify/feature.json` names that exact directory.
6. Run clarify when needed, then plan, tasks, analyze and implement in order.

Use only the context the slice needs:

- [vision](../docs/product/VISION.md) for durable intent;
- [architecture](../docs/ARCHITECTURE.md) for current boundaries;
- [status](../docs/STATUS.md) for verified behavior and gaps; and
- [Archive](../docs/Archive/) for labelled research only.

If a proposed feature has no delivery-plan row or gate, amend the plan before
creating the feature. Never create one umbrella specification for a sequence
container or the whole product.

## Lifecycle

| State | Meaning |
| --- | --- |
| Draft | Requirements are incomplete. |
| Ready | Spec, plan and tasks are consistent and dependencies are met. |
| Active | Implementation has begun and the feature pointer names it. |
| Complete | Acceptance evidence passes, current docs are updated and the directory moves under `completed/`. |

Each coding feature includes regression tests, public/internal documentation
impact, relevant generated-documentation checks and Lore-formatted task
commits. Verify and commit each task before starting the next. The governing
rules are in the [project constitution](../.specify/memory/constitution.md).
