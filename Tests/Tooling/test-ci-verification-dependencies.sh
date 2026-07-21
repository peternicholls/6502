#!/usr/bin/env bash

set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
workflow="${repo_root}/.github/workflows/ci.yml"

job_body() {
    local job="$1"
    awk -v job="${job}" '
        $0 == "  " job ":" { in_job = 1 }
        in_job && /^  [a-zA-Z0-9_-]+:/ && $1 != job ":" { exit }
        in_job { print }
    ' "${workflow}"
}

require_job_text() {
    local job="$1"
    local text="$2"
    if ! job_body "${job}" | rg -Fq -- "${text}"; then
        printf '%s: %s job does not declare %s\n' "${workflow}" "${job}" "${text}" >&2
        return 1
    fi
}

require_job_text core 'python3-yaml'
require_job_text apple-package 'python3 -m venv .build/ci-python'
require_job_text apple-package '.build/ci-python/bin/python -m pip install pyyaml'
require_job_text apple-package \
    'echo "${PWD}/.build/ci-python/bin" >> "${GITHUB_PATH}"'

echo 'CI verification dependency tests passed'
