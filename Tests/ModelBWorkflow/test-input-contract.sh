#!/usr/bin/env bash

set -euo pipefail
source "$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/testlib.sh"

source_path="${repo_root}/Sources/BeebDemo/main.swift"

require_source() {
    local description="$1"
    local pattern="$2"
    if rg -q "${pattern}" "${source_path}"; then return 0; fi
    printf 'Missing Model B input contract: %s\n' "${description}" >&2
    return 1
}

require_source "physical keyboard capture" 'MachineKeyboardCapture'
require_source "machine input focus state" 'inputFocus'
require_source "owner-serialized key submission" 'setKey\(column:'
require_source "documented program marker" '10 PRINT'
require_source "keyboard-operable focus" 'becomeFirstResponder'

echo 'Model B input contract tests passed'
