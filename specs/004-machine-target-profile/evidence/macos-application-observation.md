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

## User Story 2: Model B+ 64K rejection and Model B recovery

This journey used source commit
`fca9bb002696524cc8186e8be563316986c81f17` on the same host, operating system
and Xcode toolchain recorded above. The application was rebuilt with:

`xcodebuild -project Beeb6502.xcodeproj -scheme BeebDemo-macOS -destination platform=macOS -derivedDataPath .build/target-profile/t023-derived CODE_SIGNING_ALLOWED=NO build`

The build reported `** BUILD SUCCEEDED **`; its retained log is
`.build/target-profile/t023-macos-build.log`. BeebDemo was quit, not merely
closed, before launching the rebuilt product from the recorded launch path.

The complete Model B+ request, assistive-technology inspection and Model B
recovery ran from 17:28:36 to 17:34:44 BST (6 minutes 8 seconds). In the native
picker, `End` highlighted `BBC Model B+ 64K` and `Return` confirmed it. The
visible and captured accessibility state then reported:

- picker/requested profile: `BBC Model B+ 64K`
- active profile: `BBC Microcomputer Model B`
- rejection: `BBC Model B+ 64K is recognised, but machine support is not yet
  available. Active profile remains: BBC Microcomputer Model B`
- emulator placeholder: `BBC MICRO`

The rejection appeared 5 ms after the final `Return` event. The live
accessibility tree exposed a `Machine profile` pop-up button with value
`BBC Model B+ 64K` and identifier `machine-profile-picker`. Its requested-state
region carried identifier `requested-machine-profile` and exposed the B+
request, unchanged Model B active identity and full recognised-unavailable
message together. The visible active label remained independently Model B; no
B+ runtime, fallback label or unresponsive state was observed.

VoiceOver was started for the B+ state and the BeebDemo window was traversed
with the VoiceOver next-item chord. The simultaneous accessibility capture
reported the same B+ picker value, requested identity, rejection and retained
Model B text. As in User Story 1, no spoken-audio recording was made; this claim
is limited to the live VoiceOver navigation and captured accessibility output.

Accessibility Inspector 26.3 targeted `BeebDemo (22436)`. Its inspection view
identified the exact BeebDemo application and focused window. The 736 ms audit
reported four warnings, recorded rather than suppressed:

- low contrast for the `BBC MICRO` placeholder text;
- a parent/child mismatch in `NSThemeWidgetZoomMenuRemoteView`;
- missing useful information on the generic SwiftUI window-hosting view; and
- a missing equivalent action warning on SwiftUI's native pop-up adaptor.

None of the warnings identified the requested, active or rejection text as
missing. The pop-up warning is retained as a known Inspector discrepancy:
keyboard selection succeeded in both directions and the live accessibility
tree exposed the control's pop-up role, label, current value and stable
identifier.

Finally, the picker was reopened, `Home` highlighted
`BBC Microcomputer Model B` and `Return` confirmed it. Recovery took 1.060
seconds and restored matching Model B picker, requested and active values while
clearing the B+ rejection. This closes the User Story 2 application journey.

## Final acceptance observation

The final journey used the aggregate acceptance commit
`b15da72d2f302388bb79df4d795f264ceffecc81`. It was rebuilt with:

`xcodebuild -project Beeb6502.xcodeproj -scheme BeebDemo-macOS -destination platform=macOS -derivedDataPath .build/target-profile/t031-derived CODE_SIGNING_ALLOWED=NO build`

The build reported `** BUILD SUCCEEDED **`; its retained log is
`.build/target-profile/t031-macos-build.log`. The previously running BeebDemo
was quit before the rebuilt product was launched from
`/Users/peternicholls/Dev/6502/Build/Products/Debug/BeebDemo.app`. The observed
acceptance process was PID 34936 and its executable resolved to that exact app
bundle.

The complete final journey ran on 2026-07-21 from 02:01:58 to 02:11:07 BST
(9 minutes 9 seconds). The initial live accessibility tree reported the
canonical Model B picker, requested identity and active identity together. The
native picker was focused by keyboard, opened with `Space`, moved to
`BBC Model B+ 64K` with `End`, and confirmed with `Return`. Confirmation and
the resulting accessibility observation completed in 800 ms.

The rebuilt application then exposed:

- picker/requested profile: `BBC Model B+ 64K`;
- active profile: `BBC Microcomputer Model B`;
- rejection: `BBC Model B+ 64K is recognised, but machine support is not yet
  available. Active profile remains: BBC Microcomputer Model B`;
- stable picker identifier: `machine-profile-picker`;
- stable requested-state identifier: `requested-machine-profile`; and
- emulator placeholder: `BBC MICRO`.

VoiceOver was running from
`/System/Library/CoreServices/VoiceOver.app/Contents/MacOS/VoiceOver` as PID
23012. The BeebDemo window was traversed three times with the VoiceOver
next-item chord while the simultaneous accessibility capture continued to
expose the B+ picker value, requested identity, full rejection and unchanged
Model B active identity. No spoken-audio recording or transcript was captured;
the claim remains limited to the live traversal, running VoiceOver process and
captured accessibility output.

Accessibility Inspector 26.3 targeted `Mac Studio > BeebDemo (34936)`. A fresh
timed audit completed in 1.537 seconds and reported three warnings:

- `NSThemeWidgetZoomMenuRemoteView` was not an accessibility child of its
  parent element;
- SwiftUI's generic AppKit window-hosting view was missing useful accessibility
  information; and
- SwiftUI's pop-up adaptor was missing accessibility action support equivalent
  to click/tap input.

The earlier low-contrast warning did not recur in this audit. None of the three
reported warnings identified the requested, active or rejection text as
missing. The pop-up action warning remains an Inspector discrepancy: the same
native control completed keyboard selection in both directions and its live
accessibility node retained the label, value and stable identifier.

For recovery, the focused picker was reopened with `Space`, moved to
`BBC Microcomputer Model B` with `Home`, and confirmed with `Return`. Recovery
and observation completed in 815 ms. Picker, requested and active identities
again matched canonical Model B and the rejection cleared; no B+ runtime,
fallback identity, stale rejection or unresponsive state was observed.

During the observation, an unrelated documentation-only user commit
`6ee8dc2d897b70dec4a3ead95418ded9619d72c0` was added above the acceptance
commit. It changed only original-guide assets and `docs/REFERENCES.md`; it did
not alter or rebuild the already launched acceptance binary. This final record
therefore continues to identify `b15da72` as the tested source checkpoint.
