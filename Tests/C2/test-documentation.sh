#!/usr/bin/env bash

set -euo pipefail
source "$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/testlib.sh"

c2_prepare_paths

require_contract() {
    local path="$1"
    local text="$2"
    if ! rg -Fq -- "${text}" "${repo_root}/${path}"; then
        printf '%s: missing C2 documentation contract: %s\n' "${path}" "${text}" >&2
        return 1
    fi
}

require_contract Sources/BeebCore/include/beeb/output.hpp \
    '/// Complete immutable-in-transit RGBA observation owned by its result value.'
require_contract Sources/BeebCore/include/beeb/output.hpp \
    '/// Consistent owned observation of emulated progress and output pressure.'
require_contract Sources/BeebCore/include/beeb_c.h \
    '/// Transfers the oldest retained completed frame into caller-owned storage.'
require_contract Sources/BeebKit/BeebMachine.swift \
    '/// Independently owned result of one continuous 48 kHz mono audio drain.'
require_contract docs/code/bounded-output.md \
    'frames produced = frames consumed + frames dropped + retained frame depth'

# Prove that the documentation gate rejects a new undocumented public C2 API.
fixture="${c2_build_dir}/documentation-negative"
rm -rf "${fixture}"
mkdir -p "${fixture}/include" "${fixture}/src" "${fixture}/docs" "${fixture}/.build"
printf '%s\n' '#pragma once' 'int undocumented_c2_output(void);' \
    >"${fixture}/include/c2-output.h"
printf '%s\n' '# C2 fixture' '' 'Documentation-negative fixture.' \
    >"${fixture}/docs/guide.md"
printf 'public_baseline_count=0\ninternal_baseline_count=0\n' \
    >"${fixture}/documentation-debt.txt"
printf 'include/c2-output.h\n' >"${fixture}/changed-files.txt"

set +e
"${repo_root}/scripts/build-docs.sh" --profile portable --check \
    --source-root "${fixture}" --output-dir "${fixture}/.build/docs" \
    --debt-baseline "${fixture}/documentation-debt.txt" \
    --changed-files "${fixture}/changed-files.txt" \
    >"${fixture}/stdout.log" 2>"${fixture}/stderr.log"
status=$?
set -e
test "${status}" -ne 0
rg -q 'missing public documentation: include/c2-output.h' "${fixture}/stderr.log"

make -C "${repo_root}" --no-print-directory docs-check
