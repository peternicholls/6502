# Swift Machine-Profile API Contract

`BeebMachineProfile` and its component values are public immutable `Sendable`,
`Equatable` values. They expose canonical `.modelB` and `.modelBPlus64K`
constructors while retaining raw identifiers/versions so later values do not
require a closed two-case identity enum. Human-readable labels distinguish the
two committed profiles without asserting implementation support.
The canonical values preserve base codes `0x00000001` and `0x00000002`
respectively at component version 1; no Swift-only alternate identity exists.

`BeebMachine` has a designated profile-aware initializer and retains the
existing no-argument Model B convenience. Its profile property queries the C
runtime and returns an independently owned Swift value. Model B+ 64K,
malformed, unknown and incompatible construction failures map to typed Swift
errors with actionable localized descriptions; no error path produces a
machine or silently substitutes Model B.

Support-status enum translation uses an `@unknown default` containment path so
a later C status cannot be interpreted as support. The wrapper adds no lock or
mutable profile cache and does not borrow C aggregate storage.

Swift preserves the same malformed, unknown, incompatible and
recognised-unavailable precedence as C++ and C. Raw future-option fixtures have
no invented display name. The sixteen-entry boundary remains representable in
owned Swift values, and a rejected seventeen-entry or multi-defect request
cannot replace the active `BeebMachine`.
