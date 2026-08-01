#!/usr/bin/env bash

set -euo pipefail

model_b_workflow_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd "${model_b_workflow_dir}/../.." && pwd)"
source "${model_b_workflow_dir}/testlib.sh"
model_b_workflow_prepare_build_dir
model_b_workflow_assert_untracked_build_dir

fixture_dir="${repo_root}/.build/model-b-workflow/runtime-acceptance"
rm -rf "${fixture_dir}"
mkdir -p "${fixture_dir}"

swift test --filter BeebMachineTests/testProductionInputSequenceProducesDeterministicOwnedOutput
make --no-print-directory demo-rom .build/cpp/beeb-headless

for run in one two; do
    .build/cpp/beeb-headless \
        --os .build/cpp/cleanroom-demo.rom \
        --cycles 100000 \
        --frame "${fixture_dir}/frame-${run}.ppm" \
        >"${fixture_dir}/stdout-${run}.txt"
    test -s "${fixture_dir}/stdout-${run}.txt"
    test -s "${fixture_dir}/frame-${run}.ppm"
    grep -q '^P6$' "${fixture_dir}/frame-${run}.ppm"
done

grep -v '^wrote ' "${fixture_dir}/stdout-one.txt" >"${fixture_dir}/state-one.txt"
grep -v '^wrote ' "${fixture_dir}/stdout-two.txt" >"${fixture_dir}/state-two.txt"
cmp "${fixture_dir}/state-one.txt" "${fixture_dir}/state-two.txt"
cmp "${fixture_dir}/frame-one.ppm" "${fixture_dir}/frame-two.ppm"
echo "Model B runtime acceptance: production Swift input/output and portable headless evidence passed"
