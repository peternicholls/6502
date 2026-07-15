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
