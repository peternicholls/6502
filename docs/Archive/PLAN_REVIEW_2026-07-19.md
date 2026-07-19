# Working-Docs & Planning Review — 2026-07-19

**Scope:** Cross-artefact consistency check of the current forward direction:
`docs/product/MACHINE_DELIVERY_PLAN.md` (sole programme authority) and its
supporting documents (`VISION.md`, `ROADMAP.md`, `CORE_ROADMAP.md`,
`ARCHITECTURE.md`, `STATUS.md`, `CORE_BASELINE.md`, `LEGACY_DECISIONS.md`,
`constitution.md`, `CHANGELOG.md`, `RELEASING.md`, `.github/workflows/ci.yml`,
`Makefile`, `specs/`).

**Method:** Read each document, then cross-checked claims between them and
against the repository state (branches, tags, CI, Makefile targets, feature
pointer). SHA-256 lengths and hash consistency were verified programmatically.

Findings are grouped by severity. Each item names the documents in tension and
a concrete suggested fix.

---

## High severity (internal contradiction or broken release claim)

### H1. `CORE_BASELINE.md` is labelled "Pre-C0 observation" while C0 is verified complete

- `docs/CORE_BASELINE.md` line 3: `**Status:** Pre-C0 observation` and
  `**Recorded:** 2026-07-15`.
- The same file then documents the **C0 exit gate evidence**, approved
  clean-room references, descriptive throughput, and browsable-docs evidence —
  all of which are *post-C0* artefacts.
- `docs/STATUS.md`, `docs/CORE_ROADMAP.md` (C0 section), and the constitution
  all treat C0 as verified complete. `docs/CORE_ROADMAP.md` links to
  `CORE_BASELINE.md#c0-exit-gate-evidence` as the authority for the C0 exit
  gate.
- Every other forward/supporting doc is dated `2026-07-19`; `CORE_BASELINE.md`
  is the only one still dated `2026-07-15` with a stale pre-C0 banner.

**Impact:** A reader (human or agent) following the authority chain lands on a
document that declares itself pre-C0 while being cited as C0 exit evidence.
This undermines the "STATUS.md / CORE_BASELINE.md own the evidence" rule.

**Fix:** Update the `Status:` line to reflect that C0 is verified complete and
the file now holds the ratified exit-gate evidence; refresh the recording date.
The pre-C0 observation prose can be retained as a historical subsection.

### H2. Two simultaneous "current development candidate" versions in CHANGELOG

- `CHANGELOG.md` has both:
  - `## [0.2.0] - Unreleased` introduced by "This is the current development
    candidate."
  - `## [0.3.0] - Unreleased` introduced by the same sentence.
- `VERSION` contains `0.3.0`.
- `docs/STATUS.md` §Release state acknowledges both are unreleased but does not
  reconcile the double "current candidate" wording.

**Impact:** Ambiguous which minor is being developed; release-checklist step 1
("Move completed entries from Unreleased into a dated version section") is
ambiguous with two Unreleased sections; `make check-version` only checks that a
heading matching `VERSION` exists, so it cannot detect this drift.

**Fix:** Decide whether 0.2.0 ships as a tagged C1-only release or is folded
into 0.3.0. If folded, collapse the 0.2.0 section into 0.3.0. If kept, drop
the "current development candidate" sentence from the 0.2.0 block and state
that 0.2.0 is pending its own release before 0.3.0 work is eligible.

### H3. CHANGELOG lists `0.1.0` as released (2026-07-15) but no git tag exists

- `CHANGELOG.md`: `## [0.1.0] - 2026-07-15`.
- `git tag -l` returns nothing — no `v0.1.0` (or any) annotated tag exists on
  the repository.
- `docs/RELEASING.md` step 7 requires creating an annotated `vMAJOR.MINOR.PATCH`
  tag and publishing release notes.

**Impact:** The single shipped release claim is not backed by the release
procedure the project mandates. `STATUS.md` §Release state explicitly warns
"Recheck remote state before following RELEASING.md" for 0.2.0/0.3.0, but the
0.1.0 entry reads as a completed release.

**Fix:** Either create the retrospective `v0.1.0` annotated tag at the
matching commit with changelog-derived notes, or reword the 0.1.0 heading to
record that no tag was published and treat the first tagged release as
forwards-looking.

---

## Medium severity (cross-document inconsistency or traceability gap)

### M1. C3 parallelism rule conflicts with the delivery plan's strict ordering

- `docs/CORE_ROADMAP.md` §Phase C3 *Parallelism*: "Its slices may run beside
  the remaining M1 host work after `machine-target-profile` completes."
- `docs/product/MACHINE_DELIVERY_PLAN.md` specification-sequence table numbers
  the C3 slices (rows 8–11) **after** all M1 host slices (rows 1–6) and the
  iOS/iPadOS adaptation (row 7), and states "Names below are stable planning
  identities, not permission to combine them into one sprint" but also presents
  them as a numbered `Order` column.

**Impact:** It is unclear whether C3 may physically overlap M1 host work
(parallelism clause) or must wait until rows 1–7 are done (ordering clause).
The programme authority says ordering lives only in the delivery plan, so the
CORE_ROADMAP parallelism clause is arguably ultra vires.

**Fix:** Either (a) add an explicit note in the delivery plan that C3 may run
in parallel with M1 host slices 2–7 once `machine-target-profile` completes,
or (b) remove/soften the parallelism clause in CORE_ROADMAP to defer to the
plan.

### M2. M2 gate text omits the iOS/iPadOS dependency its closing slice requires

- Delivery plan M2 gate: "Depends on: M1 and completed C3 snapshot contracts."
- Delivery plan row 12 `machine-session-lifecycle` (the slice that closes M2):
  "Depends on: M1, C3, `machine-ios-ipados-adaptation`."
- `machine-ios-ipados-adaptation` (row 7) itself depends on M1.

**Impact:** The M2 gate definition does not mention iOS/iPadOS, yet the only
slice that can close M2 requires it. A reader checking the M2 gate alone would
miss that iOS/iPadOS adaptation is on the M2 critical path.

**Fix:** Either add `machine-ios-ipados-adaptation` to the M2 "Depends on"
line, or state in the M2 gate text that session lifecycle on iOS/iPadOS is
exercised through row 12's dependency.

### M3. Mermaid dependency graph omits edges the table establishes

- The dependency-view Mermaid in `MACHINE_DELIVERY_PLAN.md` shows:
  - `TP --> C3` ✓ (matches row 8 depends on "target profile")
  - but **no** `C2 --> TP` edge, even though row 1 depends on C2.
  - `C3 --> BP` and `C4 --> BP` ✓, but **no** `TP --> BP` edge, even though
    row 14 depends on "Target profile, C3, relevant C4 foundation".
- The `C2 --> H1` edge collapses target-profile/firmware/runtime/audio/input
  (rows 1–5) into one node, which hides the target-profile fan-out.

**Impact:** The diagram is illustrative but is the only visual dependency view;
missing edges under-represent the criticality of `machine-target-profile`
(everything downstream of C3 and B+ depends on it).

**Fix:** Add `C2 --> TP`, `TP --> BP` (and optionally `TP --> H1`) to the
Mermaid graph, or annotate the diagram that it is simplified and the table is
authoritative for dependency edges.

### M4. B+ 64K profile is on the M3 critical path but its reference set is an open decision

- Delivery plan M3 gate requires Model B+ 64K selection, firmware boot,
  snapshot round-trip, and one timing case from C4.
- `CORE_ROADMAP.md` B+ workstream entry criteria: "Primary references identify
  the selected processor, memory/display paging, firmware and disc-controller
  behavior."
- `VISION.md` §Open product decisions still lists: "Which primary-reference and
  compatibility fixture set will ratify the exact processor and disc-controller
  variants for the Model B+ 64K profile."

**Impact:** M3 cannot start until the B+ reference/fixture set is chosen, yet
that selection is still Open. The schedule risk is not surfaced in the delivery
plan's M3 "Depends on" line.

**Fix:** Add "B+ 64K primary-reference and fixture set ratified" as an explicit
M3 prerequisite in the delivery plan, and track the open decision with an owner
or a named research slice preceding row 14.

### M5. `REFERENCES.md` lacks datasheets for several already-implemented devices

- `docs/REFERENCES.md` lists: MCS6500 programming manual, BBC AUG, Intel 8271,
  Klaus tests, Bruce Clark decimal test, Visual6502 decimal, Harte UEF draft.
- `docs/STATUS.md` hardware-fidelity table lists implemented devices with open
  fidelity gaps for: **6522 VIA** (R6522/SY6522), **6845 CRTC**, **Video ULA**,
  **SN76489**, and cassette (6850 ACIA / Serial ULA).
- None of those device datasheets appear in the references list, despite
  CORE_ROADMAP requiring "a primary reference or documented clean-room
  observation" for each compatibility-led device slice.

**Impact:** The compatibility-led device workstream and the B+ storage profile
both require cited primary references; the current reference list cannot
support those slices without new citations.

**Fix:** Add the WDC/Rockwell 6522, Hitachi HD46505 / Motorola 6845 CRTC, BBC
Video ULA application notes, TI SN76489, and (for later cassette work) the
6850 and Serial ULA references to `REFERENCES.md` as they become needed, or
record an explicit gap.

### M6. Row 15 bundles disk and tape workflows under one slash-separated identity

- Delivery plan row 15: `machine-disk-workflow / machine-tape-file-workflow`,
  "Depends on: Selected C5 slices".
- Constitution principle IX requires every spec to trace to "a named row or
  gate". A slash in a single row is ambiguous: is this one spec or two?
- `CORE_ROADMAP.md` C5 decomposes disk (`writable-disk-export`,
  `dfs-controller-compatibility`) and cassette (`cassette-chipset`,
  `uef-media-primitives`, `wav-edge-decoder`) as **distinct** primitives, and
  states WAV edge decoding is gated on UEF file loading being reliable first.

**Impact:** Traceability from a single spec to this row is unclear; the C5
ordering constraint (UEF before WAV) is not visible in the delivery plan.

**Fix:** Split row 15 into two rows (`machine-disk-workflow` and
`machine-tape-file-workflow`) with distinct dependencies (the latter depending
on the C5 cassette/UEF slices), or state explicitly that the row covers two
sequentially-delivered specs and which C5 primitive ordering applies.

### M7. C4/C5/C6 core sequences are single rows but represent many specs

- Delivery plan rows 13, 14, 16 are labelled "…sequence" and row 18 "…slices".
- The plan states names are "not permission to combine them into one sprint",
- but the constitution's "trace to a named row or gate" rule does not say
  whether multiple specs may trace to one sequence row, nor whether each
  sub-slice needs its own row before implementation.

**Impact:** When `machine-target-profile` (row 1) is selected, the feature
pointer can name it cleanly. When row 13 (C4 sequence) is selected, it is
unclear whether one feature directory traces to the whole sequence or whether
the plan must be amended to add per-slice rows first.

**Fix:** Add a one-line rule to the delivery plan's specification-sequence
section: "A sequence row is a planning container; each implemented spec within
it must be named as a sub-identity (e.g. `c4-bus-trace-contract`) in its
feature directory and may begin only when the sequence's dependencies are
satisfied."

---

## Low severity (hygiene, clarity, minor gaps)

### L1. Apple CI lane runs no C++ functional tests and no sanitizers for the core

- `.github/workflows/ci.yml` `apple-package` job runs only `swift test`,
  `swift build`, `make test-c2-xcode`, and `make docs-check`.
- The `core` (Ubuntu) job runs `make test`, `make sanitize`, `make test-c1`,
  `make test-c2-portable`, `make thread-sanitize`, `make format-check`.
- `Makefile`: on Darwin, `make sanitize` defaults to `undefined` only (ASan is
  intentionally off on Apple).

**Impact:** The C++ functional suite (`Tests/test_main.cpp`) and UBSan run only
on Linux. Given the product is an Apple-native application, the Apple lane has
no core behavioural coverage. This is documented but is a coverage asymmetry.

**Fix:** Consider adding `make test` (and optionally `make sanitize` with
`SANITIZERS=undefined`) to the `apple-package` job, or explicitly record in
`STATUS.md` that Apple core coverage is delegated to the Linux lane.

### L2. `make thread-sanitize` covers only the C1 race script

- `Makefile` target `thread-sanitize` runs
  `C1_ONLY_TSAN=1 Tests/C1/test-runtime-races.sh`.
- CI invokes it with `C1_REQUIRE_TSAN=1`. C2's TSan enforcement comes from
  `C2_REQUIRE_TSAN=1` inside `make test-c2-portable`, not from an explicit
  `thread-sanitize` target.

**Impact:** The target name suggests broad TSan coverage but it is C1-scoped.
A contributor reading `CONTRIBUTING.md` (which lists `make thread-sanitize` as
a check) may believe it covers all races.

**Fix:** Rename to `test-c1-tsan` / add a `test-c2-tsan` target and a combined
`thread-sanitize` that runs both, or document in `CONTRIBUTING.md` that
`make thread-sanitize` is C1-only and C2 TSan is enforced via
`make test-c2-portable`.

### L3. M1 first-user experience tensions with the open clean-room-firmware decision

- M1 gate step 1 requires the user to "import user-owned OS and BASIC ROMs".
- `VISION.md` success outcome: "a new user can import suitable firmware, reach
  the machine and type a simple program **without external instructions**."
- `LEGACY_DECISIONS.md` and `VISION.md` both leave "redistributable clean-room
  firmware" as **Open**.

**Impact:** Without a default firmware path, the "without external
instructions" success outcome requires the user to source Acorn ROMs
independently — a likely friction point at first launch. The tension is
acknowledged but not reflected in the M1 acceptance criteria.

**Fix:** Either add a M1 sub-requirement for an onboarding flow that explains
firmware sourcing clearly, or expedite the clean-room-firmware feasibility
decision before M1 validation (row 6).

### L4. `machine-firmware-onboarding` row 2 dependency wording is loose

- Row 2 depends on "Target profile" — fine — but the independently demonstrable
  outcome says "locally remembers user-owned OS and sideways ROM **access**"
  (likely a typo for "ROMs").

**Fix:** Correct "ROM access" → "ROMs".

### L5. Mermaid graph in `ARCHITECTURE.md` is consistent; the runtime-ownership closure prose duplicates status claims

- `ARCHITECTURE.md` "Runtime ownership closure" describes the capacity-three
  frame FIFO, capacity-4,096 audio ring, and reset epoch boundary in prose.
  These claims duplicate `STATUS.md` §C2 completion evidence and §C1 verified
  outcome.

**Impact:** Minor — two places to keep in sync if capacities or policies
change.

**Fix:** Acceptable as architecture-level intent; consider referencing
`STATUS.md` for exact measured numbers rather than restating them.

### L6. `STATUS.md` evidence counts should be date-stamped per row

- The "Verified" table and the per-phase evidence blocks cite specific
  revisions and dates inside prose, but the top-level table has no
  "last verified" column.

**Impact:** A reader cannot quickly tell how stale each green row is.

**Fix:** Add a "Last verified" column (date or revision) to the top "Verified"
table.

---

## Things that are consistent and correct (no action needed)

- **SHA-256 hashes** in `STATUS.md`, `CORE_BASELINE.md`, and
  `Tests/Fixtures/C0/manifest.txt` are all 64 hex chars and mutually identical
  across files (`f13c6b64…`, `5882cedf…`, `c4c9884a…`).
- **`.specify/feature.json` is `{}`**, matching AGENTS.md's "no feature is
  currently active" and `specs/README.md` lifecycle rules.
- **`origin/develop`** (used by CI's `DOCS_BASE=origin/develop`) exists both
  locally and on the remote.
- **`VERSION` (0.3.0)** matches the `0.3.0` CHANGELOG heading and STATUS.md
  release-state prose.
- **Authority hierarchy** is consistently restated across `docs/README.md`,
  `docs/product/README.md`, `specs/README.md`, `CONTRIBUTING.md`, and the
  constitution (principle IX): MACHINE_DELIVERY_PLAN is sole forward authority;
  STATUS.md owns verified claims; supporting catalogues do not select work.
- **C0/C1/C2 completion** is consistently marked complete across
  `CORE_ROADMAP.md`, `STATUS.md`, `ARCHITECTURE.md`, and the spec directories
  (`specs/001-…`, `002-…`, `003-…` all exist with full artefact sets).
- **C3 snapshot invariant** (quiescent completed-instruction boundary only;
  profile/expansion identity explicit; C4 must preserve the safe point) is
  stated identically in `CORE_ROADMAP.md` C3 and C4 sections.
- **Profile model** (extensible base + versioned expansions; unknown rejects
  safely; no fallback) is consistent across delivery plan, ROADMAP, VISION,
  CORE_ROADMAP C3/B+, and LEGACY_DECISIONS.

---

## Suggested priority of remediation

1. **H1** — flip `CORE_BASELINE.md` status banner (pure doc fix, removes a
   visible contradiction in the evidence authority chain).
2. **H2 / H3** — reconcile the CHANGELOG version state and the missing 0.1.0
   tag before any further release work; this blocks clean execution of
   `RELEASING.md`.
3. **M2 / M4** — surface the iOS/iPadOS and B+-reference prerequisites on the
   M2/M3 gate lines so the critical path is honest.
4. **M1 / M6 / M7** — clarify parallelism and sequence-row traceability so the
   next selected slice has an unambiguous home.
5. **M5** — expand `REFERENCES.md` ahead of any device or B+ work.
6. Low-severity items can be batched into a documentation-tidy pass.

---

*Prepared 2026-07-19 from a read-only review of the tracked planning documents
and repository state. No source code or specifications were modified.*
