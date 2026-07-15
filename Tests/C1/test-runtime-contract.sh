#!/usr/bin/env bash

set -euo pipefail
source "$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/testlib.sh"

c1_build_tests
"${c1_test_binary}" --filter "C1 contract:"
"${c1_test_binary}" --filter "C1 lifecycle:"
"${c1_test_binary}" --filter "C1 transactions:"
"${c1_test_binary}" --filter "C1 input:"
