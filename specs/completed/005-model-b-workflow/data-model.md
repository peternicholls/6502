# Data Model: Running Model B Workflow

## FirmwareAssignment

| Field | Meaning | Validation / ownership |
| --- | --- | --- |
| `role` | `operatingSystem` or `language` | Exactly one assignment per role in the active Model B session. |
| `profile` | Explicit requested Model B identity | Must remain Model B; firmware never infers a profile. |
| `bank` | Fixed sideways-ROM destination | `12` for the M1 language assignment; host rejects any other or unavailable destination. |
| `bookmark` | Opaque host access token | Host-owned, read-only and resolved only for bounded access; absent without recoverable permission. |
| `displayName` | User-visible source label | Derived safely from source metadata; never classifies compatibility. |
| `availability` | `unassigned`, `available`, `stale`, `inaccessible`, `rejected` | Supplies typed recovery text. |
| `privateBytes` | Runtime copy of accepted ROM bytes | Core-owned after import; no borrowed host storage or source mutation. |

## ModelBWorkflowState

| Field | Meaning | Invariant |
| --- | --- | --- |
| `activeProfile` | Current runtime profile | Model B only; rejected B+ requests never replace it. |
| `firmwareState` | Pair of firmware assignments | Failed candidate leaves the last accepted pair intact. |
| `runState` | Paused, running or recoverable failure | Derived from owner-serialized runtime status; no competing host lifecycle. |
| `inputFocus` | Whether documented keys target the machine | No key event bypasses the runtime owner. |
| `presentationEpoch` | Current output epoch/frame identity | Reset or BREAK cannot present stale output as current. |
| `diagnostic` | User-facing workflow state | Actionable, accessible and derived from typed outcomes. |

## State transitions

```text
unassigned --select/validate--> available --install/reset--> BASIC-ready
available --source stale/inaccessible--> recovery-needed --reselect--> available
available --rejected candidate--> available
BASIC-ready --run--> running --pause--> paused
running or paused --reset/BREAK--> BASIC-ready
any state --recoverable failure--> recovery-needed --correct action--> prior valid state
```

The runtime owns machine mutation and observations. The host owns only
presentation, bookmark and diagnostic values.
