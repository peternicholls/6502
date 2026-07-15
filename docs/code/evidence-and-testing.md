# Evidence and Testing

C0 separates runtime code from the tools that prove its observable behavior.
The `beeb-evidence` executable is a headless host of the supported C ABI; it is
not linked into `BeebCore` or `BeebKit`.

## Evidence levels

- Unit and boundary tests protect CPU, device, machine, C ABI, and Swift
  behavior.
- Named deterministic workloads produce a CPU-state record and complete bitmap
  or Mode 7 frame.
- Approved references compare the state bytes and PPM frame bytes exactly.
- The aggregate verifier runs the relevant build, sanitizer, version,
  provenance, reference, and documentation groups with visible group results.

Approved C0 fixtures are synthetic or clean-room. No operating-system ROM,
commercial disc, or proprietary character-ROM bytes belong in the repository.
The reference updater is an explicit, guarded maintenance flow; the ordinary
verifier is read-only.

## Reproducing evidence

Use `make test-c0` for focused shell contracts and `make verify-c0` for the
aggregate result. Use `make verify-c0-references` to regenerate into temporary
storage and compare against tracked references. Generated runs live under
`.build/c0/` and are never authoritative.

## Reviewing a behavior change

Start with the narrow unit or boundary test, then update the named workload only
if the public observation changes intentionally. Review state and full-frame
diffs before invoking the guarded reference updater. Record why the observation
changed; never accept a reference merely to make the verifier green.
