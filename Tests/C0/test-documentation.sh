#!/usr/bin/env bash
set -euo pipefail

source "$(cd "$(dirname "$0")" && pwd)/testlib.sh"
c0_test_setup
cd "${C0_TEST_ROOT}"

builder="${C0_DOCS_SCRIPT:-${C0_TEST_ROOT}/scripts/build-docs.sh}"
if [[ ! -x "${builder}" ]]; then
    printf 'missing code documentation builder: %s\n' "${builder}" >&2
    exit 1
fi

fixture="${C0_TEST_TMP}/fixture"
mkdir -p "${fixture}/include" "${fixture}/src" "${fixture}/docs"
printf '%s\n' \
    '#pragma once' \
    '/// Adds two values.' \
    '/// @param lhs Left value.' \
    '/// @param rhs Right value.' \
    '/// @return The sum.' \
    'int add(int lhs, int rhs);' >"${fixture}/include/example.hpp"
printf '%s\n' '# Guide' '' 'A small conceptual guide.' >"${fixture}/docs/guide.md"
printf 'baseline_count=0\n' >"${fixture}/documentation-debt.txt"
: >"${fixture}/changed-files.txt"

common_args=(--check --source-root "${fixture}" \
    --debt-baseline "${fixture}/documentation-debt.txt" \
    --changed-files "${fixture}/changed-files.txt")

c0_capture portable "${builder}" --profile portable \
    --output-dir "${C0_TEST_TMP}/docs-portable" "${common_args[@]}"
c0_expect_status 0 portable
test -f "${C0_TEST_TMP}/docs-portable/index.html"
test -f "${C0_TEST_TMP}/docs-portable/cpp/index.html"
c0_assert_contains "${C0_TEST_TMP}/docs-portable/index.html" "cpp/index.html"
c0_assert_contains "${C0_TEST_TMP}/docs-portable/index.html" "Swift documentation requires the macOS profile"
c0_pass "portable profile generates strict C/C++ docs and one landing page"

fake_docc="${C0_TEST_TMP}/fake-docc"
printf '%s\n' '#!/usr/bin/env bash' 'set -euo pipefail' \
    'mkdir -p "$1"' \
    'printf "<html><body>Swift fixture</body></html>\\n" >"$1/index.html"' >"${fake_docc}"
chmod +x "${fake_docc}"
c0_capture macos env C0_DOCC_COMMAND="${fake_docc}" "${builder}" --profile macos \
    --output-dir "${C0_TEST_TMP}/docs-macos" "${common_args[@]}"
c0_expect_status 0 macos
test -f "${C0_TEST_TMP}/docs-macos/swift/index.html"
c0_assert_contains "${C0_TEST_TMP}/docs-macos/index.html" "swift/index.html"
c0_pass "macOS profile includes the DocC site"

undocumented="${C0_TEST_TMP}/undocumented"
cp -R "${fixture}" "${undocumented}"
printf '%s\n' '#pragma once' 'int undocumented();' >"${undocumented}/include/example.hpp"
c0_capture undocumented "${builder}" --profile portable --check \
    --source-root "${undocumented}" --output-dir "${C0_TEST_TMP}/docs-undocumented" \
    --debt-baseline "${undocumented}/documentation-debt.txt" \
    --changed-files "${undocumented}/changed-files.txt"
c0_expect_failure undocumented
c0_assert_contains "${C0_TEST_TMP}/undocumented.stderr" \
    "missing public documentation: include/example.hpp:2"
c0_pass "undocumented public declaration is rejected"

invalid_markup="${C0_TEST_TMP}/invalid-markup"
cp -R "${fixture}" "${invalid_markup}"
printf '%s\n' \
    '#pragma once' \
    '/// Broken parameter contract.' \
    '/// @param missing This parameter does not exist.' \
    'int value();' >"${invalid_markup}/include/example.hpp"
c0_capture invalid-markup "${builder}" --profile portable --check \
    --source-root "${invalid_markup}" --output-dir "${C0_TEST_TMP}/docs-invalid" \
    --debt-baseline "${invalid_markup}/documentation-debt.txt" \
    --changed-files "${invalid_markup}/changed-files.txt"
c0_expect_failure invalid-markup
c0_assert_contains "${C0_TEST_TMP}/invalid-markup.stderr" "argument 'missing'"
c0_pass "invalid documentation markup is diagnosed"

broken_link="${C0_TEST_TMP}/broken-link"
cp -R "${fixture}" "${broken_link}"
printf '%s\n' \
    '#pragma once' \
    '/// Refers to @ref MissingSymbol.' \
    'int linked();' >"${broken_link}/include/example.hpp"
c0_capture broken-link "${builder}" --profile portable --check \
    --source-root "${broken_link}" --output-dir "${C0_TEST_TMP}/docs-broken-link" \
    --debt-baseline "${broken_link}/documentation-debt.txt" \
    --changed-files "${broken_link}/changed-files.txt"
c0_expect_failure broken-link
c0_assert_contains "${C0_TEST_TMP}/broken-link.stderr" "unable to resolve reference"
c0_pass "unresolved symbol link is diagnosed"

complex="${C0_TEST_TMP}/complex"
cp -R "${fixture}" "${complex}"
printf '%s\n' 'int timing_step(int cycles) { return cycles + 1; }' >"${complex}/src/complex.cpp"
printf 'src/complex.cpp\n' >"${complex}/changed-files.txt"
c0_capture complex "${builder}" --profile portable --check \
    --source-root "${complex}" --output-dir "${C0_TEST_TMP}/docs-complex" \
    --debt-baseline "${complex}/documentation-debt.txt" \
    --changed-files "${complex}/changed-files.txt"
c0_expect_failure complex
c0_assert_contains "${C0_TEST_TMP}/complex.stderr" \
    "complex documentation missing: src/complex.cpp"
c0_pass "changed complex code requires rationale or a reviewed N/A"

debt="${C0_TEST_TMP}/debt"
cp -R "${fixture}" "${debt}"
printf '%s\n' 'DEBT-001|src/legacy.cpp|low|Document when changed|C1' \
    >>"${debt}/documentation-debt.txt"
c0_capture debt "${builder}" --profile portable --check \
    --source-root "${debt}" --output-dir "${C0_TEST_TMP}/docs-debt" \
    --debt-baseline "${debt}/documentation-debt.txt" \
    --changed-files "${debt}/changed-files.txt"
c0_expect_failure debt
c0_assert_contains "${C0_TEST_TMP}/debt.stderr" \
    "documentation debt exceeds baseline: 1 > 0"
c0_pass "documentation debt cannot grow"

test "$(git check-ignore -v .build/docs/index.html | wc -l | tr -d ' ')" = "1"
if git ls-files --error-unmatch .build/docs/index.html >/dev/null 2>&1; then
    printf 'generated documentation is tracked unexpectedly\n' >&2
    exit 1
fi
c0_pass "generated documentation remains ignored"

rg -q '\*\*Code Documentation\*\*' .specify/templates/spec-template.md
rg -q '\*\*Code Documentation\*\*' .specify/templates/plan-template.md
rg -q 'Generate browsable code documentation' .specify/templates/tasks-template.md
rg -q 'Code Documentation' .specify/templates/checklist-template.md
c0_pass "Spec Kit templates enforce documentation impact and validation"
