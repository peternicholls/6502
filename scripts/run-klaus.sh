#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "$0")/.." && pwd)"
image="${TMPDIR:-/tmp}/6502_functional_test.bin"
url="https://raw.githubusercontent.com/Klaus2m5/6502_65C02_functional_tests/master/bin_files/6502_functional_test.bin"

curl -fsSL "$url" -o "$image"
make -C "$root" all
"$root/.build/beeb-headless" --functional "$image" --success 0x3469 --max-instructions 100000000
