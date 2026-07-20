#!/usr/bin/env bash

set -euo pipefail
source "$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/testlib.sh"

target_profile_prepare_build_dir
source_path="${repo_root}/Sources/BeebDemo/main.swift"
project="${repo_root}/Beeb6502.xcodeproj"
derived="$(mktemp -d "${target_profile_build_dir}/application-derived.XXXXXX")"
trap 'rm -rf -- "${derived}"' EXIT

require_source() {
    local description="$1"
    local pattern="$2"
    if rg -q "${pattern}" "${source_path}"; then return 0; fi
    printf 'Missing target-profile host contract: %s\n' "${description}" >&2
    return 1
}

require_source "native labelled machine-profile picker" \
    'Picker\("Machine profile", selection: \$model\.requestedProfile\)'
require_source "separate requested profile state" '@Published .*requestedProfile'
require_source "separate active profile state" '@Published .*activeProfile'
require_source "profile-aware candidate construction" 'BeebMachine\(profile:'
require_source "Model B+ 64K picker choice" 'case modelBPlus64K'
require_source "Model B+ 64K profile mapping" 'case \.modelBPlus64K: return \.modelBPlus64K'
require_source "recognised-but-unavailable recovery copy" \
    'is recognised, but machine support is '
require_source "unchanged active-profile rejection presentation" \
    'let activeName = activeProfile\?\.displayName \?\? "None"'
require_source "visible retained active-profile copy" \
    'not yet available\. Active profile remains: \\\(activeName\)'
require_source "stable picker accessibility identifier" \
    'accessibilityIdentifier\("machine-profile-picker"\)'
require_source "stable requested-profile accessibility identifier" \
    'accessibilityIdentifier\("requested-machine-profile"\)'
require_source "stable active-profile accessibility identifier" \
    'accessibilityIdentifier\("active-machine-profile"\)'
require_source "stable profile-status accessibility identifier" \
    'accessibilityIdentifier\("machine-profile-status"\)'

run_logged() {
    local label="$1"
    shift
    if "$@" >"${target_profile_build_dir}/${label}.log" 2>&1; then return 0; fi
    printf 'Target-profile application build failed: %s\n' "${label}" >&2
    tail -80 "${target_profile_build_dir}/${label}.log" >&2
    return 1
}

run_logged build-macos xcodebuild -project "${project}" -scheme BeebDemo-macOS \
    -destination 'platform=macOS' -derivedDataPath "${derived}/macos" \
    SYMROOT="${derived}/macos/Build" OBJROOT="${derived}/macos/Intermediates" \
    CODE_SIGNING_ALLOWED=NO build
run_logged build-ios-simulator xcodebuild -project "${project}" -scheme BeebDemo-iOS \
    -destination 'generic/platform=iOS Simulator' -derivedDataPath "${derived}/ios" \
    SYMROOT="${derived}/ios/Build" OBJROOT="${derived}/ios/Intermediates" \
    CODE_SIGNING_ALLOWED=NO build
