#!/usr/bin/env bash

set -euo pipefail

model_b_workflow_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd "${model_b_workflow_dir}/../.." && pwd)"
model_b_workflow_build_dir="${repo_root}/.build/model-b-workflow"

model_b_workflow_prepare_build_dir() {
    mkdir -p "${model_b_workflow_build_dir}"
}

model_b_workflow_assert_untracked_build_dir() {
    if git -C "${repo_root}" check-ignore -q "${model_b_workflow_build_dir}/probe"; then
        return 0
    fi
    printf 'Model B workflow build directory is not ignored: %s\n' \
        "${model_b_workflow_build_dir}" >&2
    return 1
}

if [[ "${BASH_SOURCE[0]}" == "$0" ]]; then
    model_b_workflow_assert_untracked_build_dir
fi
