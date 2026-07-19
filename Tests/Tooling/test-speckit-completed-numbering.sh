#!/usr/bin/env bash

set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
test_root="$(mktemp -d)"
trap 'rm -rf "$test_root"' EXIT

mkdir -p "$test_root/.specify/scripts/bash"
mkdir -p "$test_root/.specify/extensions/git/scripts/bash"
mkdir -p "$test_root/specs/completed/003-bounded-output-contracts"

cp "$repo_root/.specify/scripts/bash/common.sh" \
  "$test_root/.specify/scripts/bash/common.sh"
cp "$repo_root/.specify/scripts/bash/create-new-feature.sh" \
  "$test_root/.specify/scripts/bash/create-new-feature.sh"
cp "$repo_root/.specify/extensions/git/scripts/bash/create-new-feature.sh" \
  "$test_root/.specify/extensions/git/scripts/bash/create-new-feature.sh"

assert_next_number() {
  local script="$1"
  local output
  output="$(
    cd "$test_root"
    "$script" --dry-run --short-name machine-target-profile \
      'Define machine target profiles'
  )"
  grep -Fq 'FEATURE_NUM: 004' <<<"$output"
}

assert_next_number .specify/scripts/bash/create-new-feature.sh
assert_next_number .specify/extensions/git/scripts/bash/create-new-feature.sh

echo 'completed-feature numbering tests passed'
