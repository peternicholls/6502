# macOS application observation

Status: reopened. Direct boot, frame, control, profile and accessibility
observations passed on 2026-08-01, but the required typed-program result and a
recoverable import failure have not both been directly observed. ROM bytes remain
outside the repository.

## Automated preparation

- Target: `BeebDemo-macOS`, Debug, unsigned local build.
- Build: `xcodebuild -project Beeb6502.xcodeproj -scheme BeebDemo-macOS -destination 'platform=macOS' -derivedDataPath .build/model-b-workflow/xcode CODE_SIGNING_ALLOWED=NO build` — passed.
- Launch: `open -n Build/Products/Debug/BeebDemo.app` — process remained running.
- Window inspection: CoreGraphics reported an onscreen `BeebDemo` window at
  900×700; a window capture visibly showed the Model B profile, separate Run,
  Pause, Reset and BREAK controls, keyboard focus active, and the typed
  no-ROM recovery status.
- The local accessibility query returned zero elements, so button activation
  through System Events was not used as evidence; the window capture is the
  direct visual observation.
- ROM search: no lawful `.rom` or `.bin` files were present under `/Users/peternicholls`.

## Development-only ROM inputs

- Source: [MDFS BBC BASIC](https://mdfs.net/Software/BBCBasic/BBC/) and the
  [Acorn MOS BBC 1.20 ROM index](https://mdfs.net/System/ROMs/AcornMOS/BBC_120/).
- Inputs were downloaded to `/tmp/beeb6502-model-b-roms/` and copied only into
  ignored `.build/model-b-workflow/` staging paths. They were never added to Git.
- `MOS120.rom`: 16,384 bytes, SHA-256
  `2d9fea69017864f6962704481829f95fee08446c8c3a13826d5d4e44000ac9de`.
- `BASIC200.rom`: 16,384 bytes, SHA-256
  `45bd55dc0f6f0f8f1fe9e2481de7def206565eec8f600ba3068b849ca4132079`.
- The unsigned development host uses plain bookmarks; scoped-bookmark options
  require sandbox/signing entitlements that are intentionally absent here.

## Direct acceptance

- A fresh unsigned Xcode build passed and launched from
  `Build/Products/Debug/BeebDemo.app`. The application resolved both remembered
  bookmarks, installed `MOS120.rom` as the OS image and `BASIC200.rom` in fixed
  bank 12, and visibly reached `BBC COMPUTER 32K` and `BASIC`.
- The accessibility tree exposed the Model B profile picker, requested and active
  Model B identities, ROM assignments, machine image, and separate identified
  Run, Pause, Reset and BREAK controls. Keyboard focus reported active.
- Continuous presentation advanced from frame 274 to frame 428 in epoch 1.
  Pause remained `Paused` across two observations 700 ms apart; Run resumed at
  frame 618 in epoch 2. BREAK moved presentation from epoch 1 to epoch 2 and
  visibly returned to BASIC; Reset subsequently moved it to epoch 3 and returned
  to BASIC. These are direct window and accessibility observations.
- The visible status initially rendered the cycle count as `,llu cycles`. The
  format string was corrected to portable `%llu` and protected by the focused
  frame contract; a fresh direct observation of the corrected text remains part
  of the open acceptance rerun.

## Acceptance still required

- Directly type `10 PRINT "BEEB6502"`, Return, `RUN`, Return through the physical
  key bridge and visibly observe `BEEB6502` plus two successive frame identities.
- Attempt one invalid or inaccessible OS/language assignment and directly verify
  that the working assignments and active session remain unchanged while an
  actionable diagnostic is shown.
- Recheck the corrected cycle-count status in the rebuilt application.

The 2026-08-01 session ended when the workstation locked before these remaining
observations could be completed. Automated keyboard ordering, rejected firmware,
failure atomicity and stale-output coverage passed, but automated evidence does
not close T008 or T010.

## Not claimed

No ROM bytes were created or added to the repository. This record does not claim
feature closure or the later audio-inclusive M1 gate.

## Additional ROM library staging — 2026-08-01

The [TobyLobster BBC Micro ROM Library](https://tobylobster.github.io/rom_library/)
was added as an external development fixture source. Its metadata archive is
staged at `.build/model-b-workflow/rom-library/output/metadata.json`, with the
library's 706 BBC Micro entries marked known-good and 16 KiB represented in
`.build/model-b-workflow/rom-library/bbc-micro-good-16k.tsv`. The corresponding
703 hosted images were downloaded under
`.build/model-b-workflow/rom-library/bbc-micro-good-16k/`, keyed by their
published SHA-1. Three metadata entries returned HTTP 404 from the library's
media paths and were not fabricated or substituted.

All 703 downloaded images matched their manifest SHA-1 values. The entire
`.build/` staging tree is ignored by Git; no ROM binary is part of the
repository or any commit. These fixtures expand the local acceptance options but
do not alter the 005 compatibility or provenance claims.
