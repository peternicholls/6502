# Production runtime evidence contract

The automation records three categories separately:

1. **Automated production-path evidence**: Swift/C++ tests exercise the runtime
   and copy owned values.
2. **Local-ROM evidence**: user-owned ROM runs, which are never committed and
   may only be performed on the developer machine.
3. **Human observation**: physical keyboard typing, visual display behavior,
   accessibility and recovery interaction.

Automated evidence may close only category 1. It must never change category 2 or
3 to a pass by inference.

## Failure semantics

- Invalid input returns a typed error and leaves active state unchanged.
- Empty output is an explicit failure for a test requiring a frame/audio result.
- A failed runtime command must identify its operation and preserve the last
  valid safe boundary.
- All artifacts are temporary or ignored unless the evidence note records only
  metadata, hashes and commands.
