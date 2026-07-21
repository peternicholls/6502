# Host Profile Selection and Observation Contract

The maintained application presents only the two committed profile choices:
`BBC Microcomputer Model B` and `BBC Model B+ 64K`. The requested choice,
active runtime profile and latest support/rejection result are separate state.
The active label changes only after candidate construction succeeds.

Selecting Model B creates or retains a Model B runtime and visibly reports the
same profile queried from that runtime. Selecting Model B+ 64K visibly reports
that the identity is recognised but machine support is not yet available,
retains any active Model B runtime and never labels Model B execution as B+.
Reserved future options are not user-selectable in this slice.
Model B+ 64K therefore supplies the single unsupported application selection
required by acceptance. Unknown, malformed and unassigned future-option raw
values are automated boundary fixtures only; the application never invents
names for them.

The selector is a native labelled picker. The picker, active-profile label and
rejection text carry stable accessibility identifiers and meaningful
accessibility labels/values. Interactive selection is keyboard reachable; state
is not conveyed by colour or motion alone. The acceptance record names the
macOS version, hardware, toolchain, build invocation, application launch path,
choices made and observed visible/VoiceOver/Accessibility Inspector/recovery
results. Automated tests and a successful build are necessary but cannot
replace this observation.
