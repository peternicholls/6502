# Contract: C0 Fixture and Approved References

## Tracked layout

`Tests/Fixtures/C0/README.md` is the provenance record and explains all files.
`manifest.txt` provides stable machine-readable identities. Exact expected
state and PPM files are tracked beside it.

## Manifest format

The manifest is UTF-8, one `key=value` per line, keys sorted within a record,
and blank lines separate records. Values cannot contain newlines. Each record
contains:

- `schema=beeb-c0-reference-v1`
- `id`
- `kind`
- `fixture`
- `path`
- `generator`
- `generation_command`
- `redistribution_basis`
- `coverage`
- `requested_cycles`
- `actual_cycles`
- `bytes`
- `sha256`

The human README describes limitations and review history that would be
unhelpfully encoded into single-line values.

## Verification

1. Generate the named fixture and evidence into `.build/c0/candidate/`.
2. Validate that every tracked reference has one complete manifest record and
   every record resolves to a tracked file.
3. Compare state text and PPM bytes exactly.
4. Report reference ID, expected/observed digest, and candidate path on mismatch.
5. Repeat each approved workload ten times in the determinism acceptance test.

## Update

Reference replacement is possible only through
`scripts/update-c0-reference.sh --reference <id> --reason <text>`.

The update flow must:

- refuse a missing/blank reason and a dirty generated candidate;
- generate ten identical candidates from clean tracked inputs;
- replace only the named reference and its derived manifest fields;
- preserve origin, redistribution basis, and coverage unless explicitly edited
  and reviewed;
- show `git diff -- Tests/Fixtures/C0/` and instruct the maintainer to review it;
- never run in CI or from `make verify-c0`.

No update operation implies that the new output is correct. Review evidence is
part of acceptance.
