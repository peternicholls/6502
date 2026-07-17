#!/usr/bin/env bash

set -euo pipefail
source "$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/testlib.sh"

fixture="${c1_build_dir}/aggregate-runner"
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
make -C "${repo_root}" --no-print-directory test-c1 \
    C1_TEST_SCRIPTS="${fixture}/01-first-fails.sh ${fixture}/02-second-passes.sh ${fixture}/03-third-fails.sh ${fixture}/04-fourth-passes.sh" \
    >"${output}" 2>&1
status=$?
set -e

test "${status}" -ne 0
test "$(wc -l <"${log}" | tr -d ' ')" = 4
test "$(sed -n '1p' "${log}")" = first
test "$(sed -n '2p' "${log}")" = second
test "$(sed -n '3p' "${log}")" = third
test "$(sed -n '4p' "${log}")" = fourth
rg -q "C1 group FAIL \(11\).*01-first-fails" "${output}"
rg -q "C1 group PASS: .*02-second-passes" "${output}"
rg -q "C1 group FAIL \(12\).*03-third-fails" "${output}"
rg -q "C1 group PASS: .*04-fourth-passes" "${output}"
