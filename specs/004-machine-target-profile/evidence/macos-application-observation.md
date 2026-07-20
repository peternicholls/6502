# macOS Application Observation

This record captures direct application observations for the machine-target
profile feature. Automated tests and successful builds are recorded elsewhere;
they do not substitute for the journeys below.

## Observation environment

- Date and time: 2026-07-20, 05:05-05:10 BST
- Source commit: `e32bd3d9d4c8dad1c4b97a4fe01a30be7851d4a1`
- Host: Mac Studio (`Mac14,14`), Apple M2 Ultra, 128 GB, arm64
- Operating system: macOS 26.5.2 (25F84)
- Toolchain: Xcode 26.3 (17C519), macOS 26.2 SDK
- Build invocation: `xcodebuild -project Beeb6502.xcodeproj -scheme BeebDemo-macOS -destination platform=macOS -derivedDataPath .build/target-profile/observation-derived.iUjXWM CODE_SIGNING_ALLOWED=NO build`
- Build result: `** BUILD SUCCEEDED **`; retained log:
  `.build/target-profile/t016-macos-build.log`
- Launch path: `/Users/peternicholls/Dev/6502/Build/Products/Debug/BeebDemo.app`

## User Story 1: Model B identity

The application launched normally and remained responsive. The complete
keyboard selection operation took 906 ms: the native `Machine profile` pop-up
button was focused, `Home` selected its first choice and `Return` confirmed
`BBC Microcomputer Model B`.

After confirmation, the visible application state reported:

- `Machine profile`: `BBC Microcomputer Model B`
- `Requested profile`: `BBC Microcomputer Model B`
- `Active profile`: `BBC Microcomputer Model B`
- emulator placeholder: `BBC MICRO`

The live macOS accessibility snapshot reported the selector as a pop-up button
labelled `Machine profile`, with value `BBC Microcomputer Model B` and stable
identifier `machine-profile-picker`. The requested/active/status region exposed
`Requested machine profile`, the Model B requested value and the visible
`Active profile: BBC Microcomputer Model B` status together. No colour or
motion was needed to distinguish the state.

VoiceOver was running (`com.apple.VoiceOver`, PID 5038) while the BeebDemo
window was traversed with the VoiceOver next-item chord. The simultaneous
accessibility capture exposed the same selector role, label and Model B value,
and the same requested/active Model B text. No spoken-audio recording or
transcript was captured; the assertion here is limited to the live VoiceOver
navigation session and captured accessibility output.

Accessibility Inspector 26.3 selected `Mac Studio > BeebDemo (3603)` as its
device/process target. Its audit completed in 1.223 seconds and left the warning
outline empty. The Inspector target and audit result were observed in its native
UI, independently of the BeebDemo source checks.

Result: the requested identity, active runtime identity and selected native
control all agreed on Model B. No mismatch, fallback or unresponsive state was
observed. This closes the Model B-only application journey; Model B+ rejection
and recovery are intentionally deferred to User Story 2.
