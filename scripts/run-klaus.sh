#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "$0")/.." && pwd)"
work_dir="$(mktemp -d "${TMPDIR:-/tmp}/beeb-klaus.XXXXXX")"
trap 'rm -rf -- "${work_dir}"' EXIT
image="${work_dir}/6502_functional_test.bin"
revision="7954e2dbb49c469ea286070bf46cdd71aeb29e4b"
expected_sha256="fa12bfc761e6f9057e4cc01a665a7b800ff01ae91f598af1e39a1201d01953fd"
url="https://raw.githubusercontent.com/Klaus2m5/6502_65C02_functional_tests/${revision}/bin_files/6502_functional_test.bin"

curl -fsSL "$url" -o "$image"
actual_sha256="$(shasum -a 256 "$image" | awk '{print $1}')"
[[ "${actual_sha256}" == "${expected_sha256}" ]] || {
    printf 'Klaus functional image checksum mismatch\n' >&2
    exit 1
}
make -C "$root" all
"$root/.build/cpp/beeb-headless" --functional "$image" --success 0x3469 --max-instructions 100000000
