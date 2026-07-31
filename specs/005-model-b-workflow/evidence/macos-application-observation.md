# macOS application observation

Status: blocked pending lawful user-provided Model B ROMs.

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

## Not claimed

The OS/language import, fixed bank-12 BASIC boot, physical-key sequence, visible
`BEEB6502` output, two-frame observation, control activation and relaunch
recovery remain unobserved. No proprietary ROM bytes were created or added to
the repository. T008 therefore remains open pending lawful ROMs.
