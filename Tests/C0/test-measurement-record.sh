#!/usr/bin/env bash
set -euo pipefail

source "$(cd "$(dirname "$0")" && pwd)/testlib.sh"
c0_test_setup
cd "${C0_TEST_ROOT}"

measure="${C0_MEASURE_SCRIPT:-${C0_TEST_ROOT}/scripts/measure-c0.sh}"
if [[ ! -x "${measure}" ]]; then
    printf 'missing C0 measurement script: %s\n' "${measure}" >&2
    exit 1
fi

environment_file="${C0_TEST_TMP}/environment.txt"
printf 'os=TestOS 1\narchitecture=test64\ncpu=Test CPU\ncompiler=Test C++ 1\n' >"${environment_file}"

sample_command="${C0_TEST_TMP}/sample-command"
printf '%s\n' '#!/usr/bin/env bash' \
    'set -euo pipefail' \
    'sample="$1"' \
    'case "${C0_SAMPLE_MODE:-valid}" in' \
    '  valid)' \
    '    case "${sample}" in 1) duration=333333333;; 2) duration=1000000000;; 3) duration=200000000;; 4) duration=500000000;; *) duration=250000000;; esac' \
    '    ;;' \
    '  zero) duration=0 ;;' \
    '  negative) duration=-1 ;;' \
    '  incomplete) [[ "${sample}" -lt 3 ]] || exit 7; duration=500000000 ;;' \
    '  interrupted) [[ "${sample}" -lt 2 ]] || exit 130; duration=500000000 ;;' \
    '  *) exit 9 ;;' \
    'esac' \
    'printf "actual_cycles=100000\\nelapsed_ns=%s\\n" "${duration}"' >"${sample_command}"
chmod +x "${sample_command}"

valid_record="${C0_TEST_TMP}/valid.txt"
c0_capture valid env C0_SAMPLE_MODE=valid "${measure}" --samples 5 \
    --output "${valid_record}" --sample-command "${sample_command}" \
    --environment-file "${environment_file}"
c0_expect_status 0 valid
c0_assert_contains "${valid_record}" "schema=beeb-c0-measurement-v1"
c0_assert_contains "${valid_record}" \
    "label=descriptive comparison baseline; not a product guarantee"
c0_assert_contains "${valid_record}" "sample_count=5"
c0_assert_contains "${valid_record}" "median_cycles_per_second=300000"
c0_assert_contains "${valid_record}" "min_cycles_per_second=100000"
c0_assert_contains "${valid_record}" "max_cycles_per_second=500000"
c0_assert_contains "${valid_record}" "valid=true"
c0_assert_contains "${valid_record}" "source_revision="
c0_assert_contains "${valid_record}" "-dirty"
computed="$(sed -n 's/^sample\.[0-9][0-9]*\.cycles_per_second=//p' "${valid_record}" | sort -n)"
test "$(printf '%s\n' "${computed}" | sed -n '3p')" = "300000"
test "$(printf '%s\n' "${computed}" | sed -n '1p')" = "100000"
test "$(printf '%s\n' "${computed}" | sed -n '5p')" = "500000"
c0_pass "five samples produce independently reproducible summary statistics"

assert_invalid() {
    local name="$1"
    local reason="$2"
    shift 2
    local record="${C0_TEST_TMP}/${name}.txt"
    c0_capture "${name}" "$@" --output "${record}" --sample-command "${sample_command}" \
        --environment-file "${environment_file}"
    c0_expect_failure "${name}"
    c0_assert_contains "${record}" "valid=false"
    c0_assert_contains "${record}" "invalid_reason=${reason}"
}

assert_invalid under-sampled "fewer than five valid samples" \
    env C0_SAMPLE_MODE=valid "${measure}" --samples 4
c0_pass "four samples cannot become the baseline"

assert_invalid zero-duration "sample 1 has non-positive duration" \
    env C0_SAMPLE_MODE=zero "${measure}" --samples 5
c0_pass "zero duration is invalid"

assert_invalid negative-duration "sample 1 has non-positive duration" \
    env C0_SAMPLE_MODE=negative "${measure}" --samples 5
c0_pass "negative duration is invalid"

assert_invalid incomplete "workload incomplete at sample 3" \
    env C0_SAMPLE_MODE=incomplete "${measure}" --samples 5
c0_pass "incomplete workload is retained as invalid"

assert_invalid interrupted "measurement interrupted at sample 2" \
    env C0_SAMPLE_MODE=interrupted "${measure}" --samples 5
c0_pass "interruption is retained as invalid"

missing_environment="${C0_TEST_TMP}/missing-environment.txt"
printf 'os=TestOS 1\narchitecture=test64\ncpu=Test CPU\n' >"${missing_environment}"
missing_record="${C0_TEST_TMP}/missing-environment-record.txt"
c0_capture missing-environment env C0_SAMPLE_MODE=valid "${measure}" --samples 5 \
    --output "${missing_record}" --sample-command "${sample_command}" \
    --environment-file "${missing_environment}"
c0_expect_failure missing-environment
c0_assert_contains "${missing_record}" "valid=false"
c0_assert_contains "${missing_record}" "invalid_reason=missing environment field: compiler"
c0_pass "missing environment metadata is invalid"
