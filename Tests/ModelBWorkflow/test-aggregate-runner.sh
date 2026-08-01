#!/usr/bin/env bash

set -euo pipefail
source "$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/testlib.sh"

model_b_workflow_prepare_build_dir
fixture="${model_b_workflow_build_dir}/aggregate-runner"
rm -rf -- "${fixture}"
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

set +e
make -C "${repo_root}" --no-print-directory test-model-b-workflow \
    MODEL_B_WORKFLOW_TEST_SCRIPTS="${fixture}/01-first-fails.sh ${fixture}/02-second-passes.sh" \
    >"${output}" 2>&1
status=$?
set -e

test "${status}" -ne 0
test "$(wc -l <"${log}" | tr -d ' ')" = 2
test "$(sed -n '1p' "${log}")" = first
test "$(sed -n '2p' "${log}")" = second
rg -q "Model B workflow group FAIL \\(11\\).*01-first-fails" "${output}"
rg -q "Model B workflow group PASS: .*02-second-passes" "${output}"

: >"${log}"
make -C "${repo_root}" --no-print-directory test-model-b-workflow \
    MODEL_B_WORKFLOW_TEST_SCRIPTS="${fixture}/02-second-passes.sh" \
    >"${output}" 2>&1
test "$(wc -l <"${log}" | tr -d ' ')" = 1
test "$(rg -c 'Model B workflow group PASS:' "${output}")" = 1
