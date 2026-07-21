#!/usr/bin/env bash

set -euo pipefail
source "$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/testlib.sh"

assert_source_contains() {
    local path="$1"
    local text="$2"
    rg -Fq -- "${text}" "${repo_root}/${path}" || {
        printf '%s: missing required contract text: %s\n' "${path}" "${text}" >&2
        return 1
    }
}

assert_source_absent() {
    local path="$1"
    local text="$2"
    if rg -Fq -- "${text}" "${repo_root}/${path}"; then
        printf '%s: stale contract text remains: %s\n' "${path}" "${text}" >&2
        return 1
    fi
}

assert_source_contains README.md "BBC-mode"
assert_source_contains README.md "instruction-level diagnostic"
assert_source_absent README.md "--cycles 5000000 --trace"
assert_source_contains docs/code/runtime-ownership.md "completed, versioned C API 0.2"
assert_source_absent docs/code/runtime-ownership.md "will use this runtime"
assert_source_contains docs/code/architecture.md "caller-owned allocation"
assert_source_absent docs/code/architecture.md "C pointers are borrowed"
assert_source_contains CONTRIBUTING.md "make verify-c0"
assert_source_contains CONTRIBUTING.md "make format-check"
assert_source_contains CONTRIBUTING.md "Hosts may call that"
assert_source_absent CONTRIBUTING.md "Hosts must serialize"
assert_source_contains docs/code/host-boundary.md "may enter C 0.2 concurrently"
assert_source_contains CHANGELOG.md "instruction-level tracing remains"

make -C "${repo_root}" --no-print-directory docs-check
test -f "${repo_root}/.build/docs/cpp/beeb__c_8h.html"
test -f "${repo_root}/.build/docs/cpp/classbeeb_1_1_machine_runtime.html"

docs_profile="${DOCS_PROFILE:-auto}"
if [[ "${docs_profile}" == auto ]]; then
    if [[ "$(uname -s)" == Darwin ]]; then
        docs_profile=macos
    else
        docs_profile=portable
    fi
fi
if [[ "${docs_profile}" == macos ]]; then
    test -f "${repo_root}/.build/docs/swift/documentation/beebkit/beebstatuscategory/index.html"
fi
rg -q "MachineRuntime" "${repo_root}/.build/docs/cpp/runtime_ownership.html"
rg -q "operation-owned" "${repo_root}/.build/docs/cpp/md_docs_2code_2host-boundary.html"

# Reuse the maintained negative fixtures for missing rationale, broken links,
# invalid markup, and undocumented public declarations.
"${repo_root}/Tests/C0/test-documentation.sh"
