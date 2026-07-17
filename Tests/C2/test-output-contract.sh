#!/usr/bin/env bash

set -euo pipefail
source "$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/testlib.sh"

c2_prepare_paths
source_path="${c2_fixture_dir}/output-contract.c"
object_path="${c2_build_dir}/output-contract.o"
binary_path="${c2_build_dir}/output-contract"
cat >"${source_path}" <<'C'
#include "beeb_c.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static int expect(beeb_status status, beeb_status_code code) {
    return status.code == code && status.message[BEEB_STATUS_MESSAGE_CAPACITY - 1] == '\0';
}

static int produce_frame(beeb_machine* machine) {
    int completed = 0;
    return expect(beeb_run_until_frame(machine, 200000, &completed), BEEB_STATUS_OK) && completed;
}

int main(void) {
    beeb_frame untouched = {0};
    untouched.width = 77;
    if (!expect(beeb_dequeue_frame(NULL, &untouched), BEEB_STATUS_INVALID_ARGUMENT)) return 1;
    if (untouched.width != 77) return 2;

    beeb_machine* machine = NULL;
    if (!expect(beeb_create(&machine), BEEB_STATUS_OK) || machine == NULL) return 3;
    if (!expect(beeb_dequeue_frame(machine, NULL), BEEB_STATUS_INVALID_ARGUMENT)) return 4;

    untouched.width = 77;
    if (!expect(beeb_dequeue_frame(machine, &untouched), BEEB_STATUS_EMPTY)) return 5;
    if (untouched.width != 77 || untouched.rgba != NULL) return 6;

    uint8_t rom[0x4000];
    memset(rom, 0xEA, sizeof(rom));
    rom[0] = 0x4C;
    rom[1] = 0x00;
    rom[2] = 0xC0;
    rom[0x3FFC] = 0x00;
    rom[0x3FFD] = 0xC0;
    if (!expect(beeb_load_os_rom(machine, rom, sizeof(rom)), BEEB_STATUS_OK)) return 7;
    if (!produce_frame(machine)) return 8;

    beeb_frame first = {0};
    if (!expect(beeb_dequeue_frame(machine, &first), BEEB_STATUS_OK)) return 9;
    if (!first.available || first.rgba == NULL || first.rgba_size == 0) return 10;
    uint8_t* retained = malloc(first.rgba_size);
    if (retained == NULL) return 11;
    memcpy(retained, first.rgba, first.rgba_size);

    if (!produce_frame(machine)) return 12;
    beeb_frame second = {0};
    if (!expect(beeb_dequeue_frame(machine, &second), BEEB_STATUS_OK)) return 13;
    if (!second.available || second.rgba == NULL || second.rgba == first.rgba) return 14;
    if (memcmp(first.rgba, retained, first.rgba_size) != 0) return 15;
    if (!expect(beeb_dequeue_frame(machine, &first), BEEB_STATUS_INVALID_ARGUMENT)) return 16;

    free(retained);
    if (!expect(beeb_frame_release(&first), BEEB_STATUS_OK)) return 17;
    if (first.available || first.rgba != NULL || first.rgba_size != 0) return 18;
    if (!expect(beeb_frame_release(&second), BEEB_STATUS_OK)) return 19;
    return expect(beeb_destroy(machine), BEEB_STATUS_OK) ? 0 : 20;
}
C

"${CC:-cc}" -std=c11 -Wall -Wextra -Wpedantic -Werror \
    "-I${repo_root}/Sources/BeebCore/include" -c "${source_path}" -o "${object_path}"
"${c2_cxx}" "${c2_common_flags[@]}" \
    "${c2_core_sources[@]}" "${object_path}" -o "${binary_path}"
"${binary_path}"
