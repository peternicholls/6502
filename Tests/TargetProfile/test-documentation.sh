#!/usr/bin/env bash

set -euo pipefail
source "$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/testlib.sh"

require_text() {
    local path="$1"
    local text="$2"
    if ! rg -Fq -- "${text}" "${repo_root}/${path}"; then
        printf '%s: missing target-profile documentation contract: %s\n' \
            "${path}" "${text}" >&2
        return 1
    fi
}

require_text docs/code/target-profile.md \
    'identity separate from support prevents an unavailable or invalid request'
require_text docs/code/target-profile.md \
    'BBC Model B+ 64K | `0x00000002` | 1 | recognised but unavailable'
require_text docs/code/target-profile.md \
    'A count above 16 is `malformed` without inspecting any expansion slot.'
require_text docs/code/target-profile.md \
    '1. `malformed`:'
require_text docs/code/target-profile.md \
    '2. `unknown`:'
require_text docs/code/target-profile.md \
    '3. `incompatible`:'
require_text docs/code/target-profile.md \
    '4. `recognisedUnavailable`:'
require_text docs/code/target-profile.md \
    'Model B+ 64K produces an explicit'
require_text docs/code/target-profile.md \
    'never falls back to Model B.'
require_text docs/code/target-profile.md \
    'transporting its raw value neither reserves that code'
require_text docs/code/target-profile.md \
    'not a serialized or persisted byte format'
require_text docs/code/target-profile.md \
    'must not persist raw object bytes or the release-dependent support result'

require_text docs/code/host-boundary.md \
    "the caller's handle output byte for byte"
require_text docs/code/host-boundary.md \
    'Each profile error owns the original raw candidate'
require_text docs/code/host-boundary.md \
    '[Machine Target Profiles](target-profile.md)'

make -C "${repo_root}" --no-print-directory docs-check
test -f "${repo_root}/.build/docs/cpp/md_docs_2code_2target-profile.html"
rg -q 'recognised but unavailable' \
    "${repo_root}/.build/docs/cpp/md_docs_2code_2target-profile.html"
rg -q 'not a serialized or persisted byte format' \
    "${repo_root}/.build/docs/cpp/md_docs_2code_2target-profile.html"
