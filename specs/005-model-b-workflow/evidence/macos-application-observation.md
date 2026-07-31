# macOS application observation

Status: blocked pending lawful user-provided Model B ROMs and a visible app window.

## Automated preparation

- Target: `BeebDemo-macOS`, Debug, unsigned local build.
- Build: `xcodebuild -project Beeb6502.xcodeproj -scheme BeebDemo-macOS -destination 'platform=macOS' -derivedDataPath .build/model-b-workflow/xcode CODE_SIGNING_ALLOWED=NO build` — passed.
- Launch attempt: `open -n Build/Products/Debug/BeebDemo.app` — process remained running.
- Accessibility query: the process exposed zero windows in the local session, so no UI interaction evidence was recorded.
- ROM search: no lawful `.rom` or `.bin` files were present under `/Users/peternicholls`.

## Not claimed

The OS/language import, fixed bank-12 BASIC boot, physical-key sequence, visible
`BEEB6502` output, two-frame observation, control actions, and relaunch recovery
remain unobserved. No proprietary ROM bytes were created or added to the
repository. T008 therefore remains open.
