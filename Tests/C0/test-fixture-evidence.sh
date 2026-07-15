#!/usr/bin/env bash
set -euo pipefail

source "$(cd "$(dirname "$0")" && pwd)/testlib.sh"
c0_test_setup
cd "${C0_TEST_ROOT}"

evidence_bin="${BEEB_EVIDENCE_BIN:-${C0_TEST_ROOT}/.build/cpp/beeb-evidence}"
rom_path="${BEEB_EVIDENCE_ROM:-${C0_TEST_ROOT}/.build/cpp/cleanroom-demo.rom}"

if [[ ! -x "${evidence_bin}" ]]; then
    printf 'missing C0 evidence executable: %s\n' "${evidence_bin}" >&2
    exit 1
fi

if [[ ! -f "${rom_path}" ]]; then
    make demo-rom >/dev/null
fi

c0_capture missing-workload "${evidence_bin}" --rom "${rom_path}"
c0_expect_failure missing-workload
c0_assert_contains "${C0_TEST_TMP}/missing-workload.stderr" "workload is required"
c0_pass "missing workload is rejected"

c0_capture invalid-cycles "${evidence_bin}" \
    --rom "${rom_path}" --workload bitmap --cycles nope \
    --output "state:${C0_TEST_TMP}/state.txt"
c0_expect_failure invalid-cycles
c0_assert_contains "${C0_TEST_TMP}/invalid-cycles.stderr" \
    "cycles must be a positive integer"
c0_pass "invalid cycle count is rejected"

mkdir "${C0_TEST_TMP}/output-directory"
c0_capture unwritable-output "${evidence_bin}" \
    --rom "${rom_path}" --workload bitmap --cycles 1 \
    --output "state:${C0_TEST_TMP}/output-directory"
c0_expect_failure unwritable-output
c0_assert_contains "${C0_TEST_TMP}/unwritable-output.stderr" \
    "cannot write state output"
c0_pass "unwritable output is recoverable"

c0_capture unknown-output "${evidence_bin}" \
    --rom "${rom_path}" --workload bitmap --cycles 1 \
    --output "pixels:${C0_TEST_TMP}/pixels.dat"
c0_expect_failure unknown-output
c0_assert_contains "${C0_TEST_TMP}/unknown-output.stderr" \
    "unknown output kind: pixels"
c0_pass "unknown output kind is rejected"

fixture_root="${C0_FIXTURE_ROOT:-${C0_TEST_ROOT}/Tests/Fixtures/C0}"
reference_verifier="${C0_REFERENCE_VERIFIER:-${C0_TEST_ROOT}/scripts/verify-c0-references.sh}"
if [[ ! -d "${fixture_root}" ]]; then
    printf 'missing approved fixture directory: %s\n' "${fixture_root}" >&2
    exit 1
fi
if [[ ! -x "${reference_verifier}" ]]; then
    printf 'missing reference verifier: %s\n' "${reference_verifier}" >&2
    exit 1
fi

c0_snapshot_tree "${fixture_root}" "${C0_TEST_TMP}/fixture-before.sha"
c0_capture ten-runs "${reference_verifier}" --runs 10 \
    --fixture-root "${fixture_root}" --output-dir "${C0_TEST_TMP}/ten-runs"
c0_expect_status 0 ten-runs
c0_assert_contains "${C0_TEST_TMP}/ten-runs.stdout" "mode7-state pass"
c0_assert_contains "${C0_TEST_TMP}/ten-runs.stdout" "bitmap-reference pass"
c0_assert_contains "${C0_TEST_TMP}/ten-runs.stdout" "mode7-reference pass"
c0_assert_tree_unchanged "${fixture_root}" "${C0_TEST_TMP}/fixture-before.sha"
c0_pass "ten runs match approved state and exact PPM references"

incomplete="${C0_TEST_TMP}/incomplete"
cp -R "${fixture_root}" "${incomplete}"
sed '/^coverage=/d' "${incomplete}/manifest.txt" >"${incomplete}/manifest.new"
mv "${incomplete}/manifest.new" "${incomplete}/manifest.txt"
c0_capture incomplete "${reference_verifier}" --runs 1 \
    --fixture-root "${incomplete}" --output-dir "${C0_TEST_TMP}/run-incomplete"
c0_expect_failure incomplete
c0_assert_contains "${C0_TEST_TMP}/incomplete.stderr" "missing manifest field: coverage"
c0_pass "incomplete provenance is rejected"

orphan="${C0_TEST_TMP}/orphan"
cp -R "${fixture_root}" "${orphan}"
printf 'P6\n1 1\n255\n000' >"${orphan}/orphan.ppm"
c0_capture orphan "${reference_verifier}" --runs 1 \
    --fixture-root "${orphan}" --output-dir "${C0_TEST_TMP}/run-orphan"
c0_expect_failure orphan
c0_assert_contains "${C0_TEST_TMP}/orphan.stderr" "orphan approved reference: orphan.ppm"
c0_pass "orphan reference is rejected"

mismatch="${C0_TEST_TMP}/mismatch"
cp -R "${fixture_root}" "${mismatch}"
printf '\000' >>"${mismatch}/bitmap.ppm"
c0_capture mismatch "${reference_verifier}" --runs 1 \
    --fixture-root "${mismatch}" --output-dir "${C0_TEST_TMP}/run-mismatch"
c0_expect_failure mismatch
c0_assert_contains "${C0_TEST_TMP}/mismatch.stderr" "bitmap-reference mismatch expected="
c0_assert_contains "${C0_TEST_TMP}/mismatch.stderr" "observed="
c0_pass "one-byte visual mismatch reports both identities"

wrong_cycles="${C0_TEST_TMP}/wrong-cycles"
cp -R "${fixture_root}" "${wrong_cycles}"
sed '0,/^actual_cycles=/s//actual_cycles=1/' \
    "${wrong_cycles}/manifest.txt" >"${wrong_cycles}/manifest.new"
mv "${wrong_cycles}/manifest.new" "${wrong_cycles}/manifest.txt"
c0_capture wrong-cycles "${reference_verifier}" --runs 1 \
    --fixture-root "${wrong_cycles}" --output-dir "${C0_TEST_TMP}/run-wrong-cycles"
c0_expect_failure wrong-cycles
c0_assert_contains "${C0_TEST_TMP}/wrong-cycles.stderr" "actual cycle mismatch: mode7-state"
c0_pass "requested and actual cycle evidence is validated"
