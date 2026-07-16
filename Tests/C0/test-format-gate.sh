#!/usr/bin/env bash
set -euo pipefail

source "$(cd "$(dirname "$0")" && pwd)/testlib.sh"
c0_test_setup
cd "${C0_TEST_ROOT}"

make format-check
c0_pass "the maintained tree passes the formatting gate"

fixture_root="${C0_TEST_TMP}/format-fixture"
mkdir -p "${fixture_root}/Sources" "${fixture_root}/Tests" "${fixture_root}/Tools"
cp Makefile .clang-format "${fixture_root}/"
printf '%s\n' 'int main(){return 0;}' >"${fixture_root}/Tools/unformatted.cpp"

c0_capture unformatted make -C "${fixture_root}" --no-print-directory format-check
c0_expect_failure unformatted
c0_assert_contains "${C0_TEST_TMP}/unformatted.stderr" "code should be clang-formatted"
c0_pass "an injected C++ formatting violation fails the maintained make target"

rg -Uq -- '- name: Check C\+\+ formatting\n[[:space:]]+run: make format-check' \
    .github/workflows/ci.yml
c0_pass "CI has a dedicated mandatory format-check step"
