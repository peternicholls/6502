<!-- SPECKIT START -->
No Spec Kit feature is currently active.
docs/product/MACHINE_DELIVERY_PLAN.md is the sole forward programme authority.
Select a named bounded slice from it, then create its feature branch and verify
.specify/feature.json before running plan, tasks or implementation workflows.
Use only current vision, architecture, status and implementation constraints as
support. Completed and archived material cannot add scope.
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
