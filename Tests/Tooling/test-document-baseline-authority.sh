#!/usr/bin/env bash

set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$repo_root"

require_text() {
  local file="$1"
  local text="$2"
  if ! grep -Fq "$text" "$file"; then
    echo "$file is missing required baseline text: $text" >&2
    exit 1
  fi
}

require_text docs/product/MACHINE_DELIVERY_PLAN.md \
  'Sole canonical forward programme authority'
require_text docs/STATUS.md \
  'Sole authority for verified implementation claims'
require_text docs/IMPLEMENTATION_CONSTRAINTS.md \
  'It is not a roadmap,'
require_text .specify/memory/constitution.md \
  '**Version**: 1.5.0'
require_text .specify/memory/constitution.md \
  'unit tests alone are insufficient'
require_text .specify/templates/plan-template.md \
  'User-facing work builds and launches the maintained application'
require_text .specify/templates/tasks-template.md \
  'unit tests alone do not close product acceptance'
require_text .specify/templates/checklist-template.md \
  'Unit and contract tests are not used as a substitute'

live_files=(
  AGENTS.md
  CONTRIBUTING.md
  README.md
  .specify/memory/constitution.md
  .specify/templates/checklist-template.md
  .specify/templates/plan-template.md
  .specify/templates/spec-template.md
  .specify/templates/tasks-template.md
  docs/ARCHITECTURE.md
  docs/IMPLEMENTATION_CONSTRAINTS.md
  docs/README.md
  docs/STATUS.md
  docs/product/MACHINE_DELIVERY_PLAN.md
  docs/product/VISION.md
  specs/README.md
)

if rg -n \
  'docs/CORE_ROADMAP\.md|docs/product/ROADMAP\.md|docs/product/LEGACY_DECISIONS\.md' \
  "${live_files[@]}"; then
  echo 'live baseline still references a superseded roadmap or register' >&2
  exit 1
fi

test -f docs/completed/CORE_BASELINE.md
test -f docs/completed/STATUS-through-C2-2026-07-19.md
test -d specs/completed/001-core-baseline-evidence
test -d specs/completed/002-runtime-ownership
test -d specs/completed/003-bounded-output-contracts

echo 'document baseline authority tests passed'
