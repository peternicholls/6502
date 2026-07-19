# Contract: C0 Measurement Record

## Entry point

`make measure-c0` invokes `scripts/measure-c0.sh` for the canonical clean-room
workload. Optional output path and sample-count arguments may be exposed for
tests, but fewer than five samples always produce an invalid record.

## Text format

UTF-8 `key=value` lines with samples grouped by numeric suffix:

```text
schema=beeb-c0-measurement-v1
label=descriptive comparison baseline; not a product guarantee
workload=<stable-id>
source_revision=<git-revision-and-dirty-marker>
os=<name-and-version>
architecture=<machine-architecture>
cpu=<processor-description-or-unknown>
compiler=<compiler-and-version>
build_mode=release
requested_cycles=<positive-integer>
sample_count=<integer>
sample.1.actual_cycles=<positive-integer>
sample.1.elapsed_ns=<positive-integer>
sample.1.cycles_per_second=<positive-number>
...
median_cycles_per_second=<derived-number>
min_cycles_per_second=<derived-number>
max_cycles_per_second=<derived-number>
valid=<true|false>
invalid_reason=<empty-or-semicolon-separated-reasons>
```

## Validation

A record is valid only if:

- the schema, non-guarantee label, workload, revision, environment, compiler,
  and build mode are present;
- at least five consecutive samples completed;
- each duration and cycle value is positive;
- requested workload identity is constant across samples;
- median/minimum/maximum values can be recomputed from the samples; and
- the script was not interrupted.

An invalid record is still written for diagnosis but exits unsuccessfully and
cannot be promoted as the documented comparison baseline. Correctness evidence
must pass separately; measurement never substitutes for it.
