# Firmware Onboarding Contract

## User-facing contract

The application presents separate OS and language-ROM assignments for the
active Model B profile. A user chooses a file, sees the role being assigned and
receives either an active assignment or actionable rejection. The product never
changes the source file and never substitutes another ROM after failure.

## Validation and ownership

| Role | Accepted shape | On success | On failure |
| --- | --- | --- | --- |
| OS | Exactly 16 KiB Model B MOS bytes | Copy bytes into the runtime OS ROM and retain a host bookmark for later authorised source access. | Preserve prior assignment and report the role/recovery action. |
| Language | One through 16 KiB sideways-ROM bytes in an explicit available Model B bank | Copy bytes into that bank and retain a host bookmark for later authorised source access. | Preserve prior assignment and report the role, bank or source-access problem. |

The host creates a read-only security-scoped bookmark only after successful user
selection. Every resolved access balances start and stop. A stale bookmark is
refreshed; failed resolution or access changes only host availability and asks
the user to reselect the source.

## Boundary rules

- The C++ core receives byte values, never URLs, bookmarks, paths or permissions.
- C and Swift continue to surface typed owned status values.
- Firmware never selects a profile or claims B+ compatibility.
- Public errors state the affected role and recovery action without exposing a
  sensitive path unnecessarily.
