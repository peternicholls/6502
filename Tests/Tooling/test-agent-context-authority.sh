#!/usr/bin/env bash

set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
script_source="$repo_root/.specify/extensions/agent-context/scripts/bash/update-agent-context.sh"
config_source="$repo_root/.specify/extensions/agent-context/agent-context-config.yml"
common_source="$repo_root/.specify/scripts/bash/common.sh"
test_root="$(mktemp -d)"
trap 'rm -rf "$test_root"' EXIT

mkdir -p "$test_root/.specify/extensions/agent-context/scripts/bash"
mkdir -p "$test_root/.specify/scripts/bash"
mkdir -p "$test_root/specs/003-completed" "$test_root/specs/004-machine-target-profile"
cp "$script_source" \
  "$test_root/.specify/extensions/agent-context/scripts/bash/update-agent-context.sh"
cp "$config_source" "$test_root/.specify/extensions/agent-context/agent-context-config.yml"
cp "$common_source" "$test_root/.specify/scripts/bash/common.sh"

printf '%s\n' \
  '<!-- SPECKIT START -->' \
  'stale context' \
  '<!-- SPECKIT END -->' > "$test_root/AGENTS.md"

printf '# Historical plan\n' > "$test_root/specs/003-completed/plan.md"
printf '# Active plan\n' > "$test_root/specs/004-machine-target-profile/plan.md"

printf '{}\n' > "$test_root/.specify/feature.json"
(
  cd "$test_root"
  .specify/extensions/agent-context/scripts/bash/update-agent-context.sh
)

grep -Fq 'MACHINE_DELIVERY_PLAN.md is the sole forward programme authority' \
  "$test_root/AGENTS.md"
if grep -Fq 'specs/003-completed/plan.md' "$test_root/AGENTS.md"; then
  echo 'agent-context selected a completed plan with no active feature' >&2
  exit 1
fi
if (
  cd "$test_root"
  source .specify/scripts/bash/common.sh
  get_feature_paths >/dev/null
); then
  echo 'Spec Kit resolved feature paths with an empty active-feature pointer' >&2
  exit 1
fi

printf '%s\n' \
  '{' \
  '  "feature_directory": "specs/004-machine-target-profile"' \
  '}' > "$test_root/.specify/feature.json"
(
  cd "$test_root"
  .specify/extensions/agent-context/scripts/bash/update-agent-context.sh
)

grep -Fq 'specs/004-machine-target-profile/plan.md' "$test_root/AGENTS.md"
if grep -Fq 'specs/003-completed/plan.md' "$test_root/AGENTS.md"; then
  echo 'agent-context ignored the explicit active feature pointer' >&2
  exit 1
fi

resolved_paths="$(
  cd "$test_root"
  source .specify/scripts/bash/common.sh
  get_feature_paths
)"
grep -Fq 'specs/004-machine-target-profile' <<< "$resolved_paths"

if (
  cd "$test_root"
  source .specify/scripts/bash/common.sh
  SPECIFY_FEATURE_DIRECTORY=specs/003-completed get_feature_paths >/dev/null
); then
  echo 'Spec Kit accepted a feature-directory override outside the active pointer' >&2
  exit 1
fi

if (
  cd "$test_root"
  .specify/extensions/agent-context/scripts/bash/update-agent-context.sh \
    specs/003-completed/plan.md
); then
  echo 'agent-context accepted a plan outside the active feature' >&2
  exit 1
fi

echo 'agent-context authority tests passed'
