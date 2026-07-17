#!/usr/bin/env bash

set -euo pipefail
source "$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/testlib.sh"

fixture="${c2_build_dir}/aggregate-runner"
rm -rf "${fixture}"
mkdir -p "${fixture}"
log="${fixture}/groups.log"
output="${fixture}/make.out"

make_group() {
    local path="$1"
    local marker="$2"
    local status="$3"
    printf '%s\n' \
        '#!/usr/bin/env bash' \
        "printf '%s\\n' '${marker}' >>'${log}'" \
        "exit ${status}" >"${path}"
    chmod +x "${path}"
}

make_group "${fixture}/01-first-fails.sh" first 11
make_group "${fixture}/02-second-passes.sh" second 0
make_group "${fixture}/03-third-fails.sh" third 12
make_group "${fixture}/04-fourth-passes.sh" fourth 0

set +e
make -C "${repo_root}" --no-print-directory test-c2 \
    C2_TEST_SCRIPTS="${fixture}/01-first-fails.sh ${fixture}/02-second-passes.sh ${fixture}/03-third-fails.sh ${fixture}/04-fourth-passes.sh" \
    >"${output}" 2>&1
status=$?
set -e

test "${status}" -ne 0
test "$(wc -l <"${log}" | tr -d ' ')" = 4
test "$(sed -n '1p' "${log}")" = first
test "$(sed -n '2p' "${log}")" = second
test "$(sed -n '3p' "${log}")" = third
test "$(sed -n '4p' "${log}")" = fourth
rg -q "C2 group FAIL \(11\).*01-first-fails" "${output}"
rg -q "C2 group PASS: .*02-second-passes" "${output}"
rg -q "C2 group FAIL \(12\).*03-third-fails" "${output}"
rg -q "C2 group PASS: .*04-fourth-passes" "${output}"

# A clean aggregate must also propagate success instead of retaining failure
# state from an earlier invocation.
: >"${log}"
make -C "${repo_root}" --no-print-directory test-c2 \
    C2_TEST_SCRIPTS="${fixture}/02-second-passes.sh ${fixture}/04-fourth-passes.sh" \
    >"${output}" 2>&1
test "$(wc -l <"${log}" | tr -d ' ')" = 2
test "$(sed -n '1p' "${log}")" = second
test "$(sed -n '2p' "${log}")" = fourth
test "$(rg -c 'C2 group PASS:' "${output}")" = 2
if rg -q 'C2 group FAIL' "${output}"; then
    printf 'successful C2 aggregate retained a failure\n' >&2
    exit 1
fi

# The required portable CI surface must retain every C2 group except the
# Xcode-only contract, and its concurrent boundary evidence must require TSan.
portable_plan="${fixture}/portable-plan.txt"
make -C "${repo_root}" --no-print-directory -n test-c2-portable >"${portable_plan}"
rg -q 'test-output-races\.sh' "${portable_plan}"
if rg -q 'test-xcode-project\.sh' "${portable_plan}"; then
    printf 'portable C2 aggregate included the Xcode-only contract\n' >&2
    exit 1
fi
rg -q 'C2_REQUIRE_TSAN:[[:space:]]*"1"' "${repo_root}/.github/workflows/ci.yml"
rg -q 'make test-c2-portable' "${repo_root}/.github/workflows/ci.yml"
rg -q 'make test-c2-xcode' "${repo_root}/.github/workflows/ci.yml"
