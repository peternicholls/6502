#!/usr/bin/env bash
set -euo pipefail

source "$(cd "$(dirname "$0")" && pwd)/testlib.sh"
c0_test_setup
cd "${C0_TEST_ROOT}"

updater="${C0_REFERENCE_UPDATER:-${C0_TEST_ROOT}/scripts/update-c0-reference.sh}"
fixture_root="${C0_FIXTURE_ROOT:-${C0_TEST_ROOT}/Tests/Fixtures/C0}"

if [[ ! -x "${updater}" ]]; then
    printf 'missing explicit reference updater: %s\n' "${updater}" >&2
    exit 1
fi
if [[ ! -d "${fixture_root}" ]]; then
    printf 'missing approved fixture directory: %s\n' "${fixture_root}" >&2
    exit 1
fi

c0_capture missing-reason "${updater}" --reference bitmap-reference \
    --fixture-root "${fixture_root}"
c0_expect_failure missing-reason
c0_assert_contains "${C0_TEST_TMP}/missing-reason.stderr" "reason is required"
c0_pass "blank review reason is rejected"

c0_capture unknown "${updater}" --reference unknown --reason test \
    --fixture-root "${fixture_root}"
c0_expect_failure unknown
c0_assert_contains "${C0_TEST_TMP}/unknown.stderr" "unknown reference: unknown"
c0_pass "unknown reference is rejected"

c0_capture ci env CI=true "${updater}" --reference bitmap-reference --reason test \
    --fixture-root "${fixture_root}"
c0_expect_failure ci
c0_assert_contains "${C0_TEST_TMP}/ci.stderr" "refusing reference update in CI"
c0_pass "CI cannot update approved references"

make .build/cpp/beeb-evidence .build/cpp/make-demo-rom >/dev/null
nondeterministic="${C0_TEST_TMP}/nondeterministic-evidence"
printf '%s\n' '#!/usr/bin/env bash' \
    'set -euo pipefail' \
    '"${REAL_EVIDENCE}" "$@"' \
    'count=0' \
    '[[ -f "${COUNTER}" ]] && count="$(cat "${COUNTER}")"' \
    'count=$((count + 1))' \
    'printf "%s\n" "${count}" >"${COUNTER}"' \
    'if [[ ${count} -eq 2 ]]; then' \
    '  for argument in "$@"; do' \
    '    case "${argument}" in frame:*) printf "\\000" >>"${argument#frame:}" ;; esac' \
    '  done' \
    'fi' >"${nondeterministic}"
chmod +x "${nondeterministic}"

nonidentical_root="${C0_TEST_TMP}/nonidentical-fixtures"
cp -R "${fixture_root}" "${nonidentical_root}"
c0_capture nonidentical env \
    REAL_EVIDENCE="${C0_TEST_ROOT}/.build/cpp/beeb-evidence" \
    COUNTER="${C0_TEST_TMP}/candidate-counter" \
    BEEB_EVIDENCE_BIN="${nondeterministic}" \
    "${updater}" --reference bitmap-reference --reason test \
    --fixture-root "${nonidentical_root}"
c0_expect_failure nonidentical
c0_assert_contains "${C0_TEST_TMP}/nonidentical.stderr" \
    "candidate runs are not identical: bitmap-reference"
c0_pass "non-identical candidate runs are rejected"

update_root="${C0_TEST_TMP}/update-fixtures"
cp -R "${fixture_root}" "${update_root}"
state_before="$(shasum -a 256 "${update_root}/approved-state.txt" | awk '{print $1}')"
mode7_before="$(shasum -a 256 "${update_root}/mode7.ppm" | awk '{print $1}')"
printf '\000' >>"${update_root}/bitmap.ppm"
c0_capture update "${updater}" --reference bitmap-reference \
    --reason "intentional test refresh" --fixture-root "${update_root}"
c0_expect_status 0 update
test "$(shasum -a 256 "${update_root}/approved-state.txt" | awk '{print $1}')" = "${state_before}"
test "$(shasum -a 256 "${update_root}/mode7.ppm" | awk '{print $1}')" = "${mode7_before}"
bitmap_hash="$(shasum -a 256 "${update_root}/bitmap.ppm" | awk '{print $1}')"
grep -A20 '^id=bitmap-reference$' "${update_root}/manifest.txt" | \
    grep -Fq "sha256=${bitmap_hash}"
grep -A20 '^id=bitmap-reference$' "${update_root}/manifest.txt" | \
    grep -Fq "review_note=intentional test refresh"
c0_assert_contains "${C0_TEST_TMP}/update.stdout" "Review with: git diff -- ${update_root}"
c0_pass "one reference and its manifest identity refresh in reviewed scope"
