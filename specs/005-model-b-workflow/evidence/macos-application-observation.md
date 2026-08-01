# macOS application observation

Status: accepted for the development-only vertical slice; ROM bytes remain outside
the repository.

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

- The built macOS application resolved both remembered bookmarks, installed MOS
  as the OS image and BASIC 2.00 in fixed bank 12, reset, and reached the
  `Firmware ready — BASIC-ready` state in the running host.
- The observed window retained the Model B identity, separate Run/Pause/Reset/
  BREAK controls, keyboard-focus state, and the recoverable no-ROM path.
- Physical-key mapping, completed-frame epochs, control ownership and recovery
  behavior are covered by the focused aggregate and were rechecked with the
  accepted ROM-backed launch.
- Accessibility APIs remain unavailable in this unsigned local session; the
  visual window and process observation are therefore the direct evidence.

## Not claimed

No ROM bytes were created or added to the repository. The later audio-inclusive
M1 gate remains outside this feature.
