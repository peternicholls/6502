#!/usr/bin/env bash

set -euo pipefail
source "$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/testlib.sh"

c2_prepare_paths
project="${repo_root}/Beeb6502.xcodeproj"
pbxproj="${project}/project.pbxproj"
derived="${c2_build_dir}/xcode-derived"
evidence="${c2_evidence_dir}/xcode"
mkdir -p "${evidence}"
rm -rf "${derived}"

test -f "${pbxproj}"
for scheme in BeebDemo-macOS BeebDemo-iOS Beeb6502-Tests; do
    test -f "${project}/xcshareddata/xcschemes/${scheme}.xcscheme"
done

if find "${project}" -type d -name xcuserdata -print -quit | rg -q .; then
    printf 'Xcode project contains user-specific xcuserdata\n' >&2
    exit 1
fi
if rg -n '/Users/|/home/|/tmp/|DEVELOPMENT_TEAM|PROVISIONING_PROFILE|CODE_SIGN_IDENTITY' \
    "${project}" >"${evidence}/forbidden-metadata.txt"; then
    printf 'Xcode project contains checkout-, user-, or signing-specific metadata\n' >&2
    cat "${evidence}/forbidden-metadata.txt" >&2
    exit 1
fi

# BeebCore and BeebKit remain package-owned. The project may compile the one
# existing demo entry point, but must not enumerate or copy package sources.
rg -q 'XCLocalSwiftPackageReference' "${pbxproj}"
rg -q 'relativePath = \.;' "${pbxproj}"
rg -q 'productName = BeebKit;' "${pbxproj}"
if rg -n 'Sources/BeebCore|Sources/BeebKit|BeebMachine\.swift|BeebVersion\.swift|src/.*\.cpp' \
    "${pbxproj}" >"${evidence}/duplicated-sources.txt"; then
    printf 'Xcode project duplicates package-owned source membership\n' >&2
    cat "${evidence}/duplicated-sources.txt" >&2
    exit 1
fi
test "$(rg -o 'path = main\.swift;' "${pbxproj}" | wc -l | tr -d ' ')" = 1

run_logged() {
    local label="$1"
    shift
    if "$@" >"${evidence}/${label}.log" 2>&1; then
        return 0
    fi
    printf 'Xcode contract command failed: %s\n' "${label}" >&2
    tail -80 "${evidence}/${label}.log" >&2
    return 1
}

run_logged list xcodebuild -project "${project}" -list -json
for scheme in BeebDemo-macOS BeebDemo-iOS Beeb6502-Tests; do
    rg -q "\"${scheme}\"" "${evidence}/list.log"
done

run_logged build-macos xcodebuild -project "${project}" -scheme BeebDemo-macOS \
    -destination 'platform=macOS' -derivedDataPath "${derived}/macos" \
    CODE_SIGNING_ALLOWED=NO build
run_logged build-ios-simulator xcodebuild -project "${project}" -scheme BeebDemo-iOS \
    -destination 'generic/platform=iOS Simulator' -derivedDataPath "${derived}/ios" \
    CODE_SIGNING_ALLOWED=NO build
run_logged test-macos xcodebuild -project "${project}" -scheme Beeb6502-Tests \
    -destination 'platform=macOS' -derivedDataPath "${derived}/tests" \
    CODE_SIGNING_ALLOWED=NO test

run_logged swift-build swift build --package-path "${repo_root}"
run_logged swift-test swift test --package-path "${repo_root}"
run_logged make-test make -C "${repo_root}" test
