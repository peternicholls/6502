#!/usr/bin/env bash
set -euo pipefail

source "$(cd "$(dirname "$0")" && pwd)/testlib.sh"
c0_test_setup
cd "${C0_TEST_ROOT}"

make .build/cpp/make-demo-rom >/dev/null
generator="${C0_TEST_ROOT}/.build/cpp/make-demo-rom"

default_rom="${C0_TEST_TMP}/default.rom"
mode7_rom="${C0_TEST_TMP}/mode7.rom"
bitmap_a="${C0_TEST_TMP}/bitmap-a.rom"
bitmap_b="${C0_TEST_TMP}/bitmap-b.rom"

c0_capture default "${generator}" "${default_rom}"
c0_expect_status 0 default
test "$(wc -c <"${default_rom}" | tr -d ' ')" = "16384"
c0_pass "legacy default workload remains available"

c0_capture mode7 "${generator}" --workload mode7 "${mode7_rom}"
c0_expect_status 0 mode7
cmp -s "${default_rom}" "${mode7_rom}"
c0_pass "named Mode 7 workload matches the legacy default"

c0_capture bitmap-a "${generator}" --workload bitmap "${bitmap_a}"
c0_expect_status 0 bitmap-a
c0_capture bitmap-b "${generator}" --workload bitmap "${bitmap_b}"
c0_expect_status 0 bitmap-b
test "$(wc -c <"${bitmap_a}" | tr -d ' ')" = "16384"
cmp -s "${bitmap_a}" "${bitmap_b}"
if cmp -s "${bitmap_a}" "${mode7_rom}"; then
    printf 'bitmap and Mode 7 workloads unexpectedly produced identical ROMs\n' >&2
    exit 1
fi
c0_pass "named bitmap workload is deterministic and distinct"

c0_capture unknown "${generator}" --workload unknown "${C0_TEST_TMP}/unknown.rom"
c0_expect_failure unknown
c0_assert_contains "${C0_TEST_TMP}/unknown.stderr" "unknown workload: unknown"
c0_pass "unknown workload is rejected"
