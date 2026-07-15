#!/usr/bin/env bash

set -euo pipefail
source "$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/testlib.sh"

make -C "${repo_root}" --no-print-directory docs-check
test -f "${repo_root}/.build/docs/cpp/beeb__c_8h.html"
test -f "${repo_root}/.build/docs/cpp/classbeeb_1_1_machine_runtime.html"
test -f "${repo_root}/.build/docs/swift/documentation/beebkit/beebstatuscategory/index.html"
rg -q "MachineRuntime" "${repo_root}/.build/docs/cpp/runtime_ownership.html"
rg -q "operation-owned" "${repo_root}/.build/docs/cpp/md_docs_2code_2host-boundary.html"

# Reuse the maintained negative fixtures for missing rationale, broken links,
# invalid markup, and undocumented public declarations.
"${repo_root}/Tests/C0/test-documentation.sh"
