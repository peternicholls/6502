#!/usr/bin/env bash
set -euo pipefail

source "$(cd "$(dirname "$0")" && pwd)/testlib.sh"
c0_test_setup
cd "${C0_TEST_ROOT}"

verifier="${C0_VERIFY_SCRIPT:-${C0_TEST_ROOT}/scripts/verify-c0.sh}"
if [[ ! -x "${verifier}" ]]; then
    printf 'missing C0 baseline verifier: %s\n' "${verifier}" >&2
    exit 1
fi

commands="${C0_TEST_TMP}/commands"
mkdir "${commands}"

make_command() {
    local name="$1"
    local status="$2"
    local message="$3"
    local path="${commands}/${name}"
    printf '#!/usr/bin/env bash\nprintf "%%s\\n" %q\nexit %s\n' \
        "${message}" "${status}" >"${path}"
    chmod +x "${path}"
}

make_command pass-a 0 "alpha detail"
make_command pass-c 0 "charlie detail"
make_command pass-swift 0 "swift detail"
make_command fail-b 7 "bravo failed"
make_command fail-d 9 "delta failed"
make_command interrupted 130 "signal received"

all_pass="${C0_TEST_TMP}/all-pass.groups"
printf 'alpha\tall\t%s\ncharlie\tall\t%s\nswift-only\tmacos\t%s\n' \
    "${commands}/pass-a" "${commands}/pass-c" "${commands}/pass-swift" >"${all_pass}"

isolated_root="${C0_TEST_TMP}/fresh-checkout"
mkdir -p "${isolated_root}/scripts"
cp "${verifier}" "${isolated_root}/scripts/verify-c0.sh"
c0_capture fresh-default "${isolated_root}/scripts/verify-c0.sh" --profile portable \
    --groups "${all_pass}"
c0_expect_status 0 fresh-default
test -f "${isolated_root}/.build/c0/run/run.txt"
c0_pass "default evidence directory is created in a fresh checkout"

assert_verify_output_rejected() {
    local name="$1"
    local output="$2"
    local fake_home="${C0_TEST_TMP}/home"
    local sentinel="${fake_home}/sentinel.txt"
    mkdir -p "${fake_home}"
    printf 'keep\n' >"${sentinel}"
    c0_capture "${name}" env HOME="${fake_home}" "${verifier}" --profile portable \
        --groups "${all_pass}" --output-dir "${output}"
    c0_expect_failure "${name}"
    test "$(cat "${sentinel}")" = "keep"
}

assert_verify_output_rejected verify-output-repository "${C0_TEST_ROOT}"
assert_verify_output_rejected verify-output-home "${C0_TEST_TMP}/home"
assert_verify_output_rejected verify-output-root /
c0_capture verify-output-empty "${verifier}" --profile portable \
    --groups "${all_pass}" --output-dir ''
c0_expect_failure verify-output-empty

unowned_run="${C0_TEST_TMP}/unowned-run"
mkdir -p "${unowned_run}"
printf 'keep\n' >"${unowned_run}/sentinel.txt"
assert_verify_output_rejected verify-output-unowned "${unowned_run}"
test "$(cat "${unowned_run}/sentinel.txt")" = "keep"

escaped_parent="${C0_TEST_TMP}/escaped-parent"
escaped_target="${C0_TEST_TMP}/escaped-target"
mkdir -p "${escaped_target}/run"
printf 'keep\n' >"${escaped_target}/run/sentinel.txt"
ln -s "${escaped_target}" "${escaped_parent}"
assert_verify_output_rejected verify-output-symlink "${escaped_parent}/run"
test "$(cat "${escaped_target}/run/sentinel.txt")" = "keep"

unowned_build_run="${C0_TEST_ROOT}/.build/c0-test-unowned-run"
mkdir -p "${unowned_build_run}"
printf 'keep\n' >"${unowned_build_run}/sentinel.txt"
assert_verify_output_rejected verify-output-unowned-build "${unowned_build_run}"
test "$(cat "${unowned_build_run}/sentinel.txt")" = "keep"
rm -rf -- "${unowned_build_run}"
c0_pass "dangerous baseline output paths fail without deleting sentinels"

c0_snapshot_tree Tests "${C0_TEST_TMP}/tests-before.sha"
c0_capture all-pass "${verifier}" --profile portable --groups "${all_pass}" \
    --output-dir "${C0_TEST_TMP}/run-all-pass"
c0_expect_status 0 all-pass
c0_assert_contains "${C0_TEST_TMP}/all-pass.stdout" "alpha pass"
c0_assert_contains "${C0_TEST_TMP}/all-pass.stdout" "charlie pass"
c0_assert_contains "${C0_TEST_TMP}/all-pass.stdout" "swift-only not-applicable"
c0_assert_contains "${C0_TEST_TMP}/all-pass.stdout" "overall pass"
c0_assert_tree_unchanged Tests "${C0_TEST_TMP}/tests-before.sha"
c0_pass "all applicable groups pass without mutating approved paths"

one_failure="${C0_TEST_TMP}/one-failure.groups"
printf 'alpha\tall\t%s\nbravo\tall\t%s\ncharlie\tall\t%s\n' \
    "${commands}/pass-a" "${commands}/fail-b" "${commands}/pass-c" >"${one_failure}"
c0_capture one-failure "${verifier}" --profile portable --groups "${one_failure}" \
    --output-dir "${C0_TEST_TMP}/run-one-failure"
c0_expect_failure one-failure
c0_assert_contains "${C0_TEST_TMP}/one-failure.stdout" "bravo fail"
c0_assert_contains "${C0_TEST_TMP}/one-failure.stdout" "diagnosis=bravo failed"
c0_assert_contains "${C0_TEST_TMP}/one-failure.stdout" "charlie pass"
test -f "${C0_TEST_TMP}/run-one-failure/charlie.log"
summary_order="$(grep -E '^(alpha|bravo|charlie) ' "${C0_TEST_TMP}/one-failure.stdout" | cut -d' ' -f1 | paste -sd, -)"
test "${summary_order}" = "alpha,bravo,charlie"
c0_pass "one failure does not mask later groups or reorder the summary"

multiple="${C0_TEST_TMP}/multiple.groups"
printf 'bravo\tall\t%s\ndelta\tall\t%s\n' \
    "${commands}/fail-b" "${commands}/fail-d" >"${multiple}"
c0_capture multiple "${verifier}" --profile portable --groups "${multiple}" \
    --output-dir "${C0_TEST_TMP}/run-multiple"
c0_expect_failure multiple
c0_assert_contains "${C0_TEST_TMP}/multiple.stdout" "bravo fail"
c0_assert_contains "${C0_TEST_TMP}/multiple.stdout" "delta fail"
c0_pass "multiple failures remain visible"

unexpected_skip="${C0_TEST_TMP}/unexpected-skip.groups"
printf 'missing-tool\tall\t%s\ncharlie\tall\t%s\n' \
    "${commands}/does-not-exist" "${commands}/pass-c" >"${unexpected_skip}"
c0_capture unexpected-skip "${verifier}" --profile portable --groups "${unexpected_skip}" \
    --output-dir "${C0_TEST_TMP}/run-unexpected-skip"
c0_expect_failure unexpected-skip
c0_assert_contains "${C0_TEST_TMP}/unexpected-skip.stdout" "missing-tool unexpected-skip"
c0_assert_contains "${C0_TEST_TMP}/unexpected-skip.stdout" "charlie pass"
c0_pass "missing required command is an unexpected skip"

interrupted="${C0_TEST_TMP}/interrupted.groups"
printf 'interrupted\tall\t%s\ncharlie\tall\t%s\n' \
    "${commands}/interrupted" "${commands}/pass-c" >"${interrupted}"
c0_capture interrupted "${verifier}" --profile portable --groups "${interrupted}" \
    --output-dir "${C0_TEST_TMP}/run-interrupted"
c0_expect_failure interrupted
c0_assert_contains "${C0_TEST_TMP}/interrupted.stdout" "interrupted fail (interrupted)"
c0_assert_contains "${C0_TEST_TMP}/interrupted.stdout" "charlie pass"
c0_pass "interrupted group is retained and later groups run"

c0_capture invalid-profile "${verifier}" --profile other --groups "${all_pass}" \
    --output-dir "${C0_TEST_TMP}/run-invalid"
c0_expect_failure invalid-profile
c0_assert_contains "${C0_TEST_TMP}/invalid-profile.stderr" "unsupported profile: other"
c0_pass "unsupported profile is rejected before group execution"

c0_capture focused-suite make --no-print-directory test-c0 \
    C0_TEST_SCRIPTS="${commands}/fail-b ${commands}/pass-c"
c0_expect_failure focused-suite
c0_assert_contains "${C0_TEST_TMP}/focused-suite.stdout" "bravo failed"
c0_assert_contains "${C0_TEST_TMP}/focused-suite.stdout" "charlie detail"
c0_pass "focused C0 suite retains failure after running later scripts"
