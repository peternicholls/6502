# Evidence and Testing

C0 separates runtime code from the tools that prove its observable behavior.
The `beeb-evidence` executable is a headless host of the supported C ABI; it is
not linked into `BeebCore` or `BeebKit`.

## Evidence levels

- Focused unit and boundary tests protect the behavior currently changing in
  the CPU, device, machine, C ABI, Swift or host layer.
- Named deterministic workloads produce a CPU-state record and complete bitmap
  or Mode 7 frame.
- Approved references compare the state bytes and PPM frame bytes exactly.
- Slice aggregates run the focused evidence plus affected wider regressions.
- Milestone and release aggregates run the complete maintained build,
  sanitizer, version, provenance, reference and documentation groups with
  visible group results.

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

## Proportionate verification

Use three verification levels:

1. **Task loop:** run the failing focused test and immediately adjacent boundary
   checks. Do not rerun unchanged platform builds, sanitizers, documentation or
   full historical aggregates after every edit.
2. **Slice closure:** run the feature aggregate, affected cross-language and
   regression suites, changed platform builds, documentation checks and static
   checks. Run a sanitizer when memory, ownership, concurrency, parsing or
   untrusted-input risk makes it relevant.
3. **Milestone or release closure:** run the complete maintained automated
   matrix and the full documented M1-M5 or release application journey. Later
   milestones add their host-specific journey without weakening earlier
   portable evidence.

Every behavior still starts with evidence that fails for the expected reason.
The economy comes from choosing the smallest useful test and from not repeating
unchanged evidence at every task or phase.

For user-facing slices, build and launch the maintained application and observe
the interaction that changed. Repeat the complete end-to-end journey only when
closing its milestone, unless a wider manual pass is needed to investigate a
failure. Accessibility checks follow the same rule: exercise the affected
control or flow at slice closure and the full journey at the milestone.

The planned terminal host adds two evidence surfaces over the production
runtime. Interactive checks prove raw keyboard/display/control behavior with no
GUI chrome. Noninteractive checks submit bounded machine commands and emit
deterministic text, state/frame digests, diagnostics and exit status. These
checks complement C++ unit/evidence runs and are especially useful for ROM and
complete-machine regression coverage, but they do not replace direct AppKit or
mobile interaction evidence.

Visual desktop work records representative window sizes,
resize/full-screen transitions, active-raster aspect and sharpness, focus
capture/release, toolbar/footer/keyboard-drawer state, Settings safety
interlocks and accessibility behavior. UI/UX review happens in the slice that
changes the experience rather than as an undifferentiated final-polish phase.

Timing, fidelity, performance and compatibility measurements are required only
when the slice makes or changes such a claim. Each measurement still names its
fixture, host/toolchain, observation interval and tolerance. Unsupported local
ThreadSanitizer remains `N/A`; the supported strict CI result is authoritative
instead of being imitated by redundant local stress runs.
