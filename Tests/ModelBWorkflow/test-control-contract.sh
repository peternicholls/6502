#!/usr/bin/env bash

set -euo pipefail
source "$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/testlib.sh"

source_path="${repo_root}/Sources/BeebDemo/main.swift"

require_source() {
    local description="$1"
    local pattern="$2"
    if rg -q "${pattern}" "${source_path}"; then return 0; fi
    printf 'Missing Model B control contract: %s\n' "${description}" >&2
    return 1
}

require_source "separate run control" 'Button\("Run"'
require_source "separate pause control" 'Button\("Pause"'
require_source "separate reset control" 'Button\("Reset"'
require_source "separate BREAK control" 'Button\("BREAK"'
require_source "core BREAK boundary" 'setBreak\(pressed:'
require_source "host BREAK action" 'breakExecution'

echo 'Model B control contract tests passed'
