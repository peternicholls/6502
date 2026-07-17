#!/usr/bin/env bash

set -euo pipefail
source "$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/testlib.sh"

c2_prepare_paths
project="${repo_root}/Beeb6502.xcodeproj"
pbxproj="${project}/project.pbxproj"
evidence="${c2_evidence_dir}/xcode"
mkdir -p "${evidence}"
derived="$(mktemp -d "${c2_build_dir}/xcode-derived.XXXXXX")"
trap 'rm -rf -- "${derived}"' EXIT

test -f "${pbxproj}"
for scheme in BeebDemo-macOS BeebDemo-iOS Beeb6502-Tests; do
    test -f "${project}/xcshareddata/xcschemes/${scheme}.xcscheme"
    git -C "${repo_root}" ls-files --error-unmatch \
        "Beeb6502.xcodeproj/xcshareddata/xcschemes/${scheme}.xcscheme" >/dev/null
done
git -C "${repo_root}" ls-files --error-unmatch \
    Beeb6502.xcodeproj/project.pbxproj >/dev/null

tracked_xcode_state="$(git -C "${repo_root}" ls-files | \
    rg '(^|/)(xcuserdata|DerivedData|Build)/|\.xcuserstate$|\.xc(result|archive)(/|$)' || true)"
if [[ -n "${tracked_xcode_state}" ]]; then
    printf 'Xcode project tracks user-specific or derived state:\n%s\n' \
        "${tracked_xcode_state}" >&2
    exit 1
fi
git -C "${repo_root}" check-ignore -q --no-index \
    Beeb6502.xcodeproj/xcuserdata/probe.xcuserdatad/UserInterfaceState.xcuserstate
git -C "${repo_root}" check-ignore -q --no-index DerivedData/probe
git -C "${repo_root}" check-ignore -q --no-index Build/probe
if git -C "${repo_root}" check-ignore -q --no-index \
    Beeb6502.xcodeproj/xcshareddata/xcschemes/BeebDemo-macOS.xcscheme; then
    printf 'maintained shared schemes are ignored\n' >&2
    exit 1
fi

# Xcode creates local xcuserdata during ordinary use. It is safe only while the
# repository ignore contract contains every such path; tracked state was
# rejected above and unignored state would escape into a contributor's diff.
while IFS= read -r user_data; do
    relative="${user_data#"${repo_root}/"}"
    if ! git -C "${repo_root}" check-ignore -q -- "${relative}/"; then
        printf 'Xcode project contains unignored user state: %s\n' "${relative}" >&2
        exit 1
    fi
done < <(find "${project}" -type d -name xcuserdata -print)
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
    SYMROOT="${derived}/macos/Build" OBJROOT="${derived}/macos/Intermediates" \
    CODE_SIGNING_ALLOWED=NO build
run_logged build-ios-simulator xcodebuild -project "${project}" -scheme BeebDemo-iOS \
    -destination 'generic/platform=iOS Simulator' -derivedDataPath "${derived}/ios" \
    SYMROOT="${derived}/ios/Build" OBJROOT="${derived}/ios/Intermediates" \
    CODE_SIGNING_ALLOWED=NO build
run_logged test-macos xcodebuild -project "${project}" -scheme Beeb6502-Tests \
    -destination 'platform=macOS' -derivedDataPath "${derived}/tests" \
    SYMROOT="${derived}/tests/Build" OBJROOT="${derived}/tests/Intermediates" \
    CODE_SIGNING_ALLOWED=NO test

run_logged swift-build swift build --package-path "${repo_root}"
run_logged swift-test swift test --package-path "${repo_root}"
run_logged make-test make -C "${repo_root}" test

# Maintained project metadata must already use the canonical format accepted by
# the supported Xcode toolchain. A validation run must not rewrite tracked input.
if ! git -C "${repo_root}" diff --quiet -- Beeb6502.xcodeproj; then
    printf 'Xcode validation rewrote maintained project metadata\n' >&2
    git -C "${repo_root}" diff -- Beeb6502.xcodeproj >&2
    exit 1
fi
