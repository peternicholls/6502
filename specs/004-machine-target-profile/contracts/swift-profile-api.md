# Swift Machine-Profile API Contract

`BeebMachineProfile` and its component values are public immutable `Sendable`,
`Equatable` values. They expose canonical `.modelB` and `.modelBPlus64K`
constructors while retaining raw identifiers/versions so later values do not
require a closed two-case identity enum. Human-readable labels distinguish the
two committed profiles without asserting implementation support.

`BeebMachine` has a designated profile-aware initializer and retains the
existing no-argument Model B convenience. Its profile property queries the C
runtime and returns an independently owned Swift value. Model B+ 64K,
malformed, unknown and incompatible construction failures map to typed Swift
errors with actionable localized descriptions; no error path produces a
machine or silently substitutes Model B.

Support-status enum translation uses an `@unknown default` containment path so
a later C status cannot be interpreted as support. The wrapper adds no lock or
mutable profile cache and does not borrow C aggregate storage.
