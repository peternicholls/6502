#!/usr/bin/env bash

set -euo pipefail
source "$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/testlib.sh"

workflow="${repo_root}/.github/workflows/ci.yml"
apple_job="$({
    awk '
        /^  apple-package:/ { in_job = 1 }
        in_job && /^  [a-zA-Z0-9_-]+:/ && $1 != "apple-package:" { exit }
        in_job { print }
    ' "${workflow}"
})"

if ! rg -Fq -- 'run: make test-machine-target-profile' <<<"${apple_job}"; then
    printf '%s: apple-package job does not enforce the target-profile aggregate\n' \
        "${workflow}" >&2
    exit 1
fi

echo 'target-profile CI contract tests passed'
