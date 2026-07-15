# C0 approved fixture provenance

This directory contains the reviewed, redistributable evidence used to detect
observable changes in the current emulator foundation. Normal verification is
read-only. Generated candidates belong under `.build/c0/`, not here.

## Origin and redistribution

Both workloads are generated entirely by
[`Tools/make-demo-rom/main.cpp`](../../../Tools/make-demo-rom/main.cpp). The
generator and its emitted 6502 programs were written for this project and use
no BBC Micro firmware, character generator, game, disc, tape, or user media.
The generated 16 KiB ROMs therefore have the same redistribution basis as the
project source. Generated ROM files remain build artifacts and are not tracked.

## Workloads

### `mode7`

```bash
.build/cpp/make-demo-rom --workload mode7 .build/c0/mode7.rom
.build/cpp/beeb-evidence --rom .build/c0/mode7.rom \
  --workload mode7 --cycles 100000 \
  --output state:.build/c0/mode7-state.txt \
  --output frame:.build/c0/mode7.ppm
```

The program configures a conventional 40-column Mode 7 display, clears its
screen memory, writes two colour-controlled strings, and enters a stable idle
loop. It covers aggregate CPU/device advancement, CRTC frame production, Video
ULA teletext selection, clean-room glyph rendering, public CPU state, and PPM
frame export.

It does not establish full Mode 7 compatibility. Double height, hold/release,
conceal, exact control latency, cursor behavior, and mid-frame effects remain
outside this reference.

### `bitmap`

```bash
.build/cpp/make-demo-rom --workload bitmap .build/c0/bitmap.rom
.build/cpp/beeb-evidence --rom .build/c0/bitmap.rom \
  --workload bitmap --cycles 100000 \
  --output state:.build/c0/bitmap-state.txt \
  --output frame:.build/c0/bitmap.ppm
```

The program configures a deterministic 320×200 one-bit display, fills two
screen regions with complementary byte patterns, and enters a stable idle loop.
It covers aggregate CPU/device advancement, bitmap address generation, the
one-bit serializer, default logical palette mapping, public frame access, and
PPM export.

It does not establish complete bitmap-mode or hardware-scroll compatibility,
cycle-raster accuracy, palette changes, flash timing, cursor output, or
mid-line ULA behavior.

## Approved files

- `approved-state.txt`: exact Mode 7 CPU/frame state after the named cycle
  request.
- `mode7.ppm`: exact Mode 7 frame bytes from the same run.
- `bitmap.ppm`: exact bitmap frame bytes from the bitmap run.
- `manifest.txt`: generation, coverage, cycle, byte-count, and SHA-256 identity
  for every approved file.

Approval requires ten identical clean generations. Requested cycles and actual
completed-instruction cycles are recorded separately; they may differ because
the machine finishes the current instruction.

## Verification and replacement

`make verify-c0-references` generates candidates outside this directory and
compares state and PPM bytes exactly. A mismatch is evidence to investigate,
not permission to update the reference.

Intentional replacement uses only:

```bash
make update-c0-reference REFERENCE=<id> REASON="<review rationale>"
```

The update command is disabled in CI, requires ten identical candidates,
changes only the named reference and derived manifest fields, and prints the
Git diff command for review. Reviewers must confirm that the behavior change is
intended, the stated coverage remains accurate, and no proprietary bytes have
entered the fixture.

## Review history

- 2026-07-15 — Initial C0 state, bitmap, and Mode 7 references approved after
  ten byte-identical ROM, state, and frame generations for each workload.
