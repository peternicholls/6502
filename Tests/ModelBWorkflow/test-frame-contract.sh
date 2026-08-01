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
require_source "firmware reset invalidation" 'invalidatePresentation\(\)'
require_source "presentation-only polling" 'private func pollFrame\(\)'
require_source "portable cycle-count status format" 'PC %04X   %llu cycles'

if rg -q '%,llu cycles' "${source_path}"; then
    fail "cycle-count status uses an unsupported grouping flag"
fi

poll_block="$(sed -n '/private func pollFrame()/,/private func platformImage/p' "${source_path}")"
if rg -q 'runToNextFrame|run\(cycles:' <<<"${poll_block}"; then
    printf 'Presentation polling advances emulated execution\n' >&2
    exit 1
fi

echo 'Model B frame contract tests passed'
