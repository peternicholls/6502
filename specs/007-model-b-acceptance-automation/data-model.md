# Data model: Model B acceptance automation

## Production input sequence

| Field | Rule |
| --- | --- |
| Matrix column | Integer 0...15; invalid values are rejected before runtime mutation. |
| Matrix row | Integer 0...15; invalid values are rejected before runtime mutation. |
| Pressed | Boolean press/release state; every test press has a matching release. |
| Order | FIFO order owned by `MachineRuntime`; host wall time is irrelevant. |

## Owned output observation

| Field | Rule |
| --- | --- |
| Safe point | Later completed-instruction/device boundary than the initial observation. |
| Frame | Independently owned RGBA bytes with a monotonic frame number. |
| Audio | Owned sample/result metadata from the production wrapper. |
| Diagnostics | Owned pressure and production counters; no borrowed storage. |

## Firmware preservation observation

| Field | Rule |
| --- | --- |
| Active profile | Remains Model B after candidate rejection. |
| Active safe point | Is not replaced by candidate validation. |
| Valid ROM behavior | Reset and bounded execution remain usable. |
| Candidate | Invalid size/role data is rejected with a typed error. |
