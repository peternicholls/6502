#!/usr/bin/env bash

set -euo pipefail
source "$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/testlib.sh"

evidence_dir="${c1_build_dir}/replay"
mkdir -p "${evidence_dir}"
rm -f "${evidence_dir}/accepted-ledger.txt"

c1_build_tests
BEEB_C1_EVIDENCE_DIR="${evidence_dir}" \
    "${c1_test_binary}" --filter "C1 replay:"
test -s "${evidence_dir}/accepted-ledger.txt"
