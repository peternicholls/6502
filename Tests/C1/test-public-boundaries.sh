#!/usr/bin/env bash

set -euo pipefail
source "$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/testlib.sh"

c1_build_tests
"${c1_test_binary}" --filter "C 0.2:"
swift test --package-path "${repo_root}" --filter BeebMachineTests
