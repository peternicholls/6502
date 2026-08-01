#!/usr/bin/env bash

set -euo pipefail
source "$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/testlib.sh"

model_b_workflow_prepare_build_dir
scratch_path="${model_b_workflow_build_dir}/swift-application"

swift build --package-path "${repo_root}" --scratch-path "${scratch_path}" \
    --product BeebDemo

binary_path="$(swift build --package-path "${repo_root}" --scratch-path "${scratch_path}" \
    --show-bin-path)/BeebDemo"
test -x "${binary_path}"

echo 'Model B application build tests passed'
