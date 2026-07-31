<!-- SPECKIT START -->
Active Spec Kit feature: specs/005-model-b-workflow
Its scope must trace to a named row or gate in
docs/product/MACHINE_DELIVERY_PLAN.md, the sole forward programme authority.
Completed and archived material cannot add scope.
Read the current implementation plan at specs/005-model-b-workflow/plan.md
<!-- SPECKIT END -->

## Git Checkpoints

- MUST commit each task after its verification succeeds and before beginning
  another task.
- MUST commit phase-completion changes before beginning the next phase. The
  final task commit may also be the phase checkpoint when its message explicitly
  records phase completion; do not create an empty duplicate commit.
- MUST keep completed-task changes out of later task commits. Preserve unrelated
  user changes and stage only files belonging to the completed task or phase.
- Commits MUST use the repository Lore format: an intent-first subject, useful
  decision context, and applicable `Constraint`, `Rejected`, `Confidence`,
  `Scope-risk`, `Directive`, `Tested`, and `Not-tested` trailers.
