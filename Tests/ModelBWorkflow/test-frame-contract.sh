#!/usr/bin/env bash

set -euo pipefail
source "$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/testlib.sh"

source_path="${repo_root}/Sources/BeebDemo/main.swift"

require_source() {
    local description="$1"
    local pattern="$2"
    if rg -q "${pattern}" "${source_path}"; then return 0; fi
    printf 'Missing Model B frame contract: %s\n' "${description}" >&2
    return 1
}

require_source "presentation epoch" 'presentationEpoch'
require_source "owned completed-frame dequeue" 'dequeueVideoFrame\(\)'
require_source "monotonic presented-frame guard" 'lastPresentedFrame'
require_source "epoch invalidation on reset" 'presentationEpoch &\+= 1'

echo 'Model B frame contract tests passed'
