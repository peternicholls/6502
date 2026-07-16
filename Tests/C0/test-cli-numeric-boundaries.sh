#!/usr/bin/env bash
set -euo pipefail

source "$(cd "$(dirname "$0")" && pwd)/testlib.sh"
c0_test_setup
cd "${C0_TEST_ROOT}"

make .build/cpp/beeb-headless >/dev/null
headless="${C0_TEST_ROOT}/.build/cpp/beeb-headless"
functional_image="${C0_TEST_TMP}/functional.bin"
os_rom="${C0_TEST_TMP}/os.rom"
sideways_rom="${C0_TEST_TMP}/sideways.rom"

dd if=/dev/zero of="${functional_image}" bs=65536 count=1 2>/dev/null
dd if=/dev/zero of="${os_rom}" bs=16384 count=1 2>/dev/null
dd if=/dev/zero of="${sideways_rom}" bs=16384 count=1 2>/dev/null

c0_capture functional-max "${headless}" --functional "${functional_image}" \
    --pc 65535 --success 65535 --max-instructions 0
c0_expect_status 3 functional-max
c0_assert_contains "${C0_TEST_TMP}/functional-max.stderr" \
    "Functional test instruction limit reached"
c0_pass "16-bit functional addresses accept their exact maximum"

for option in pc success; do
    c0_capture "${option}-one-past" "${headless}" --functional "${functional_image}" \
        "--${option}" 65536 --max-instructions 0
    c0_expect_status 1 "${option}-one-past"
    c0_assert_contains "${C0_TEST_TMP}/${option}-one-past.stderr" \
        "--${option} must be in 0...65535"

    c0_capture "${option}-wrap" "${headless}" --functional "${functional_image}" \
        "--${option}" 18446744073709551616 --max-instructions 0
    c0_expect_status 1 "${option}-wrap"
    c0_assert_contains "${C0_TEST_TMP}/${option}-wrap.stderr" "invalid number"
done
c0_pass "16-bit functional addresses reject one-past and wide wrapping inputs"

c0_capture bank-max "${headless}" --os "${os_rom}" --rom 15 "${sideways_rom}" --cycles 0
c0_expect_status 0 bank-max
c0_pass "sideways ROM banks accept their exact maximum"

for value in 16 4294967296; do
    name="bank-${value}"
    c0_capture "${name}" "${headless}" --os "${os_rom}" --rom "${value}" \
        "${sideways_rom}" --cycles 0
    c0_expect_status 1 "${name}"
    c0_assert_contains "${C0_TEST_TMP}/${name}.stderr" "--rom BANK must be in 0...15"
done
c0_pass "sideways ROM banks reject one-past and unsigned-width wrapping inputs"
