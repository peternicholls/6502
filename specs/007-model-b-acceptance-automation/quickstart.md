# Quickstart: Model B acceptance automation

Run from the repository root:

```sh
swift test --filter BeebMachineTests/testProduction
make test-runtime-acceptance
make test-model-b-workflow
```

The focused Swift tests use generated in-memory ROM fixtures. The aggregate
script additionally runs the portable headless fixture and checks that output
artifacts are non-empty and deterministic. No proprietary ROM is required.

These commands do not close the remaining human gates. A developer must still
launch `BeebDemo-macOS` with user-owned ROMs and observe physical typing,
visible BASIC output, invalid-import recovery and assistive behavior.
