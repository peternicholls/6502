# Research: Machine Target Profile

## Decision: Separate historical machine identity from implementation support

**Rationale**: Acorn material distinguishes the BBC Microcomputer Model B, BBC
Model B+ 64K and 128K variants, and the BBC Master Series. It also treats Tube,
Econet, ADFS and second processors as capabilities or configurations rather
than proof that a base machine implementation supports them. The application
therefore carries a profile identity and a separate support result.

**Primary references**: [Acorn BBC Microcomputer Service Manual](https://chrisacorns.computinghistory.org.uk/docs/Acorn/Manuals/Acorn_BBCSMOct85_Sec1.pdf), [Acorn BBC Micro B+ brochure](https://chrisacorns.computinghistory.org.uk/docs/Acorn/Misc/Acorn_BBCMicro%2B.pdf), [Acorn Application Note 031](https://chrisacorns.computinghistory.org.uk/docs/Acorn/AN/031.txt), and [Acorn Master Series brochure](https://chrisacorns.computinghistory.org.uk/docs/Acorn/Brochures/Acorn_APP83_TheMasterSeries.pdf).

**Alternatives considered**: Infer supported devices from a marketing model
name (rejected because optional and variant-specific capabilities would be
overstated); use generic `BBC Micro`/`BBC Master` labels (rejected because they
collapse materially different configurations).

## Decision: Commit only Model B and Model B+ 64K constants now

**Rationale**: The delivery plan commits these two base identities. B+ 128K,
Master-family revisions, Tube, Econet, ADFS and other expansions must be
representable but remain reserved until their own product case, primary
references and fixtures exist. Raw stable identifiers permit later constants
without changing the shape or meaning of version-1 values.

**Alternatives considered**: Publish constants for every named future option
now (rejected because it would prematurely freeze unresearched taxonomy);
encode all options in one closed enum (rejected because later additions would
change a supposedly closed boundary).

## Decision: Use a bounded value of raw component identifiers and versions

**Rationale**: A schema-versioned base component plus at most 16 explicitly
versioned expansion components is owned, deterministic and safe to copy across
C and Swift. Raw 32-bit identifiers can preserve and reject unknown values;
canonical ordering and duplicate rejection give one equality representation.
The bound prevents hostile or corrupt future inputs from causing unbounded work
and is sufficient for the reserved expansion directions without claiming any.

**Alternatives considered**: Imported extensible C enum as the identity
(rejected because identity also needs component versions and unknown raw-value
round trips); strings (rejected because encoding, normalization and fixed C
storage complicate equality); an unbounded vector (rejected because future
snapshot/input validation must remain bounded).

## Decision: Keep profile data value-semantic across C and Swift

**Rationale**: Swift guidance distinguishes extensible enums and explicit
shared-reference types. A machine profile is neither an object handle nor
borrowed storage, so plain fixed C aggregates and owned Swift structs provide
the clearest lifetime. Swift switches over support-status enums include
`@unknown default`; raw profile component IDs remain representable even when
unknown.

**References**: [Swift Evolution SE-0192: Non-Exhaustive Enums](https://github.com/swiftlang/swift-evolution/blob/main/proposals/0192-non-exhaustive-enums.md), [Swift Book — Statements](https://docs.swift.org/swift-book/documentation/the-swift-programming-language/statements/), and [Swift.org C++ interoperability](https://www.swift.org/documentation/cxx-interop/).

**Alternatives considered**: Borrow a C profile pointer (rejected because
lifetime would escape the operation); model a profile as a shared reference
(rejected because immutable identity has no independent object lifetime).

## Decision: Validate before construction and query through the runtime owner

**Rationale**: Pure validation classifies a value before any machine mutation.
Only canonical Model B reaches construction in this slice. A profile query is
serialized as an owner command, matching existing lifecycle/state observations
without introducing cached host truth or a second lock. Model B+ 64K returns a
typed recognised-but-unavailable result, never a Model B machine.

**Alternatives considered**: Construct Model B for unimplemented profiles
(rejected as forbidden fallback); expose a borrowed `BBCMicro` field (rejected
by runtime ownership); cache active identity independently in C or Swift
(rejected because the runtime must remain authoritative).

## Decision: Retain explicit Model B convenience construction

**Rationale**: Existing `beeb_create()` and `BeebMachine()` callers already
mean Model B. They remain documented conveniences that pass the canonical Model
B value to the new designated profile-aware construction path. Invalid or
unsupported explicit input never reaches this default.

**Alternatives considered**: Remove the existing entry points (rejected as
unnecessary source breakage); allow a missing/invalid explicit value to default
(rejected because that would be silent fallback).

## Decision: Make requested and active profile distinct in the host

**Rationale**: The selector may show Model B+ 64K as a committed target while
construction remains unavailable. The application reports the requested
profile, typed support result and unchanged active Model B separately. This
makes rejection observable and prevents the selection control from becoming a
false completion claim.

**Alternatives considered**: Hide B+ until implementation (rejected because
the feature must demonstrate its distinct identity); change the active label
before construction succeeds (rejected because it would misrepresent runtime
state); show reserved future profiles in the picker (rejected as unscheduled
product scope).

## Decision: Use native labelled selection and accessibility semantics

**Rationale**: A native SwiftUI `Picker` preserves platform keyboard and
assistive behavior without recreating control semantics. Its visible label
remains present; stable accessibility identifiers support deterministic
inspection; the requested value, active value and rejection use accessibility
values or structured content rather than colour or disabled styling alone.
Manual acceptance uses VoiceOver and Accessibility Inspector. An XCUITest
target is not added solely for this bounded slice because the repository has no
existing UI-test target and direct observation is already mandatory.

**References**: Apple documentation for [Picker](https://developer.apple.com/documentation/swiftui/picker), [accessibility labels](https://developer.apple.com/documentation/swiftui/view/accessibility%28label%3A%29), [accessibility values](https://developer.apple.com/documentation/swiftui/view/accessibility%28value%3A%29), [accessibility identifiers](https://developer.apple.com/documentation/swiftui/view/accessibility%28identifier%3A%29), [keyboard focus](https://developer.apple.com/documentation/swiftui/focusstate), [accessibility testing](https://developer.apple.com/documentation/accessibility/performing-accessibility-testing-for-your-app), and [Accessibility Inspector](https://developer.apple.com/documentation/accessibility/accessibility-inspector).

**Alternatives considered**: Custom selector chrome (rejected because it would
duplicate keyboard and accessibility behavior); auto-moving VoiceOver focus on
every selection (rejected unless observation shows the status is otherwise
missed); a new UI-test target (deferred because it adds project structure not
needed to prove this small control, while identifiers preserve that later
option).

## Decision: No persistence or mutable reconfiguration in this slice

**Rationale**: A profile is chosen at machine creation and remains immutable
for that runtime. Later snapshot and lifecycle specifications will persist it
inside their own bounded versioned envelope. Changing a running machine would
create rollback and device-reset semantics not selected by row 1.

**Alternatives considered**: Add an in-place `setProfile` command (rejected
because failure-atomic whole-machine replacement is a separate product flow);
write preferences or snapshots now (rejected as later delivery-plan rows).
