#!/usr/bin/env bash

set -euo pipefail
source "$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/testlib.sh"

source_path="${repo_root}/Sources/BeebDemo/main.swift"

require_source() {
    local description="$1"
    local pattern="$2"
    if rg -q "${pattern}" "${source_path}"; then return 0; fi
    printf 'Missing Model B firmware contract: %s\n' "${description}" >&2
    return 1
}

require_source "separate language import state" '@Published var isImportingLanguage'
require_source "typed firmware loader" 'loadFirmware\([^)]*role:'
require_source "remembered assignment storage" 'UserDefaults\.standard'
require_source "security-scoped bookmark creation" 'bookmarkData\('
require_source "security-scoped bookmark resolution" 'resolvingBookmarkData'
require_source "read-only access lifetime" 'startAccessingSecurityScopedResource'
require_source "language assignment presentation" 'Open Language ROM'
require_source "BASIC-ready firmware status" 'BASIC-ready'

echo 'Model B firmware contract tests passed'
