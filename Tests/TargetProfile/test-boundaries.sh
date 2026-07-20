#!/usr/bin/env bash

set -euo pipefail
source "$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/testlib.sh"

target_profile_prepare_build_dir
source_path="${target_profile_build_dir}/profile-boundaries.c"
object_path="${target_profile_build_dir}/profile-boundaries.o"
binary_path="${target_profile_build_dir}/profile-boundaries"

cat >"${source_path}" <<'C'
#include "beeb_c.h"

#include <stdint.h>
#include <string.h>

_Static_assert(BEEB_MACHINE_PROFILE_SCHEMA_VERSION == 1, "schema version");
_Static_assert(BEEB_MODEL_B_BASE_IDENTIFIER == UINT32_C(0x00000001), "Model B code");
_Static_assert(BEEB_MODEL_B_PLUS_64K_BASE_IDENTIFIER == UINT32_C(0x00000002),
               "Model B+ 64K code");
_Static_assert(BEEB_MACHINE_PROFILE_COMPONENT_VERSION == 1, "component version");
_Static_assert(BEEB_MACHINE_PROFILE_EXPANSION_CAPACITY == 16, "fixed capacity");
_Static_assert(BEEB_MACHINE_PROFILE_KNOWN_EXPANSION_COUNT == 0,
               "no assigned expansion codes");
_Static_assert(sizeof(((beeb_profile_component*)0)->identifier) == sizeof(uint32_t),
               "raw identifier width");
_Static_assert(sizeof(((beeb_profile_component*)0)->version) == sizeof(uint16_t),
               "component version width");
_Static_assert(sizeof(((beeb_profile_component*)0)->reserved) == sizeof(uint16_t),
               "reserved width");
_Static_assert(sizeof(((beeb_machine_profile*)0)->schema_version) == sizeof(uint16_t),
               "schema width");
_Static_assert(sizeof(((beeb_machine_profile*)0)->expansion_count) == sizeof(uint16_t),
               "count width");
_Static_assert(sizeof(((beeb_machine_profile*)0)->expansions) /
                       sizeof(((beeb_machine_profile*)0)->expansions[0]) ==
                   BEEB_MACHINE_PROFILE_EXPANSION_CAPACITY,
               "fixed expansion slots");

static int unused_slots_are_zero(const beeb_machine_profile* profile) {
    const beeb_profile_component zero = {0};
    for (size_t index = 0; index < BEEB_MACHINE_PROFILE_EXPANSION_CAPACITY; ++index) {
        if (memcmp(&profile->expansions[index], &zero, sizeof(zero)) != 0) return 0;
    }
    return 1;
}

int main(void) {
    const beeb_machine_profile model_b = beeb_machine_profile_model_b();
    const beeb_machine_profile repeated_model_b = beeb_machine_profile_model_b();
    const beeb_machine_profile model_b_plus_64k = beeb_machine_profile_model_b_plus_64k();

    if (!beeb_machine_profile_equal(&model_b, &repeated_model_b)) return 1;
    if (beeb_machine_profile_equal(&model_b, &model_b_plus_64k)) return 2;
    if (beeb_machine_profile_equal(NULL, &model_b) ||
        beeb_machine_profile_equal(&model_b, NULL))
        return 3;
    if (model_b.schema_version != BEEB_MACHINE_PROFILE_SCHEMA_VERSION ||
        model_b.base.identifier != BEEB_MODEL_B_BASE_IDENTIFIER ||
        model_b.base.version != BEEB_MACHINE_PROFILE_COMPONENT_VERSION ||
        model_b.base.reserved != 0 || model_b.expansion_count != 0)
        return 4;
    if (model_b_plus_64k.schema_version != BEEB_MACHINE_PROFILE_SCHEMA_VERSION ||
        model_b_plus_64k.base.identifier != BEEB_MODEL_B_PLUS_64K_BASE_IDENTIFIER ||
        model_b_plus_64k.base.version != BEEB_MACHINE_PROFILE_COMPONENT_VERSION ||
        model_b_plus_64k.base.reserved != 0 || model_b_plus_64k.expansion_count != 0)
        return 5;
    if (!unused_slots_are_zero(&model_b) || !unused_slots_are_zero(&model_b_plus_64k)) return 6;

    beeb_machine_profile caller_owned = model_b;
    caller_owned.base.identifier = UINT32_C(0xf0000001);
    if (model_b.base.identifier != BEEB_MODEL_B_BASE_IDENTIFIER) return 7;
    if (beeb_machine_profile_equal(&caller_owned, &model_b)) return 8;
    return 0;
}
C

"${target_profile_cc}" "${target_profile_common_c_flags[@]}" -c \
    "${source_path}" -o "${object_path}"
"${target_profile_cxx}" "${target_profile_common_cxx_flags[@]}" \
    "${object_path}" "${target_profile_core_sources[@]}" -o "${binary_path}"
"${binary_path}"
