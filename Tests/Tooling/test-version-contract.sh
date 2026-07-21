#!/usr/bin/env bash

set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "${repo_root}"

version="$(sed -n '1p' VERSION)"
IFS=. read -r major minor patch <<<"${version}"

require_literal() {
    local path="$1"
    local literal="$2"
    if ! rg -Fq -- "${literal}" "${path}"; then
        printf '%s: version contract is missing %s\n' "${path}" "${literal}" >&2
        return 1
    fi
}

require_literal Sources/BeebCore/include/beeb/version.h \
    "#define BEEB_VERSION_MAJOR ${major}"
require_literal Sources/BeebCore/include/beeb/version.h \
    "#define BEEB_VERSION_MINOR ${minor}"
require_literal Sources/BeebCore/include/beeb/version.h \
    "#define BEEB_VERSION_PATCH ${patch}"
require_literal Sources/BeebCore/include/beeb/version.h \
    "#define BEEB_VERSION_STRING \"${version}\""
require_literal Tests/test_main.cpp \
    "beeb_version_string()) == \"${version}\""
require_literal Tests/BeebKitTests/BeebMachineTests.swift \
    "XCTAssertEqual(BeebVersion.current, \"${version}\")"
require_literal CHANGELOG.md "## [${version}] -"

version_header=Sources/BeebCore/include/beeb/version.h
for target in \
    .build/cpp/beeb-headless \
    .build/cpp/beeb-tests \
    .build/cpp/beeb-evidence; do
    dry_run="$(make -n -W "${version_header}" "${target}")"
    if ! rg -Fq -- "-o ${target}" <<<"${dry_run}"; then
        printf '%s: changing %s does not rebuild the target\n' \
            "${target}" "${version_header}" >&2
        exit 1
    fi
done

echo 'version contract tests passed'
