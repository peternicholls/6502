#!/usr/bin/env bash

set -euo pipefail
source "$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/testlib.sh"

target_profile_prepare_build_dir
source_path="${target_profile_build_dir}/profile-boundaries.c"
object_path="${target_profile_build_dir}/profile-boundaries.o"
binary_path="${target_profile_build_dir}/profile-boundaries"
cpp_source_path="${target_profile_build_dir}/profile-concurrency.cpp"
cpp_binary_path="${target_profile_build_dir}/profile-concurrency"

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

    beeb_machine_profile_validation validation_canary;
    memset(&validation_canary, 0xa5, sizeof(validation_canary));
    const beeb_machine_profile_validation original_validation_canary = validation_canary;
    if (beeb_validate_machine_profile(NULL, &validation_canary).code !=
            BEEB_STATUS_INVALID_ARGUMENT ||
        memcmp(&validation_canary, &original_validation_canary,
               sizeof(validation_canary)) != 0)
        return 9;

    beeb_machine_profile raw_unknown = model_b;
    raw_unknown.base.identifier = UINT32_C(0xf0000001);
    beeb_machine_profile_validation unknown_validation = {0};
    if (beeb_validate_machine_profile(&raw_unknown, &unknown_validation).code != BEEB_STATUS_OK ||
        unknown_validation.support != BEEB_MACHINE_PROFILE_UNKNOWN ||
        unknown_validation.message[0] == '\0')
        return 10;

    beeb_machine* const handle_canary = (beeb_machine*)(uintptr_t)1;
    beeb_machine* rejected_output = handle_canary;
    if (beeb_create_with_profile(&raw_unknown, &rejected_output).code !=
            BEEB_STATUS_INVALID_ARGUMENT ||
        rejected_output != handle_canary)
        return 11;

    beeb_machine_profile count_seventeen = model_b;
    count_seventeen.expansion_count = 17;
    for (size_t index = 0; index < BEEB_MACHINE_PROFILE_EXPANSION_CAPACITY; ++index) {
        count_seventeen.expansions[index].identifier =
            UINT32_C(0xf0000100) + (uint32_t)index;
        count_seventeen.expansions[index].version = BEEB_MACHINE_PROFILE_COMPONENT_VERSION;
    }
    beeb_machine_profile_validation count_validation = {0};
    if (beeb_validate_machine_profile(&count_seventeen, &count_validation).code !=
            BEEB_STATUS_OK ||
        count_validation.support != BEEB_MACHINE_PROFILE_MALFORMED)
        return 12;
    rejected_output = handle_canary;
    if (beeb_create_with_profile(&count_seventeen, &rejected_output).code !=
            BEEB_STATUS_INVALID_ARGUMENT ||
        rejected_output != handle_canary)
        return 13;
    return 0;
}
C

cat >"${cpp_source_path}" <<'CPP'
#include "beeb/profile.hpp"
#include "beeb_c.h"

#include <atomic>
#include <cstdint>
#include <cstring>
#include <latch>
#include <thread>
#include <vector>

int main() {
    beeb::MachineTargetProfile::ExpansionStorage expansions{};
    expansions.back() = {UINT32_C(0xf000000f), beeb::machineProfileComponentVersion};
    const beeb::MachineTargetProfile raw{
        beeb::machineProfileSchemaVersion,
        {UINT32_C(0xf0000001), beeb::machineProfileComponentVersion},
        17,
        expansions,
    };
    if (raw.schemaVersion() != beeb::machineProfileSchemaVersion ||
        raw.base().identifier() != UINT32_C(0xf0000001) || raw.expansionCount() != 17 ||
        raw.expansions().back().identifier() != UINT32_C(0xf000000f))
        return 20;

    beeb_machine* machine = nullptr;
    if (beeb_create(&machine).code != BEEB_STATUS_OK || !machine) return 21;
    const auto modelB = beeb_machine_profile_model_b();
    beeb_machine_profile baseline{};
    if (beeb_get_machine_profile(machine, &baseline).code != BEEB_STATUS_OK ||
        !beeb_machine_profile_equal(&baseline, &modelB))
        return 22;

    constexpr std::size_t queryCount = 32;
    std::latch ready(queryCount + 1);
    std::latch start(1);
    std::atomic<unsigned> failures{0};
    std::vector<std::thread> queries;
    queries.reserve(queryCount);
    for (std::size_t index = 0; index < queryCount; ++index) {
        queries.emplace_back([&] {
            beeb_machine_profile output;
            std::memset(&output, 0xa5, sizeof(output));
            const auto canary = output;
            ready.count_down();
            start.wait();
            const auto status = beeb_get_machine_profile(machine, &output);
            if (status.code == BEEB_STATUS_OK) {
                if (!beeb_machine_profile_equal(&output, &modelB)) ++failures;
            } else if (status.code == BEEB_STATUS_INVALID_ARGUMENT ||
                       status.code == BEEB_STATUS_UNAVAILABLE) {
                if (std::memcmp(&output, &canary, sizeof(output)) != 0) ++failures;
            } else {
                ++failures;
            }
        });
    }
    beeb_status destroyStatus{};
    std::thread destroyer([&] {
        ready.count_down();
        start.wait();
        destroyStatus = beeb_destroy(machine);
    });
    ready.wait();
    start.count_down();
    for (auto& query : queries)
        query.join();
    destroyer.join();
    if (destroyStatus.code != BEEB_STATUS_OK || failures.load() != 0) return 23;

    beeb_machine_profile staleOutput;
    std::memset(&staleOutput, 0xa5, sizeof(staleOutput));
    const auto staleCanary = staleOutput;
    if (beeb_get_machine_profile(machine, &staleOutput).code != BEEB_STATUS_INVALID_ARGUMENT ||
        std::memcmp(&staleOutput, &staleCanary, sizeof(staleOutput)) != 0)
        return 24;
    return 0;
}
CPP

"${target_profile_cc}" "${target_profile_common_c_flags[@]}" -c \
    "${source_path}" -o "${object_path}"
"${target_profile_cxx}" "${target_profile_common_cxx_flags[@]}" \
    "${object_path}" "${target_profile_core_sources[@]}" -o "${binary_path}"
"${target_profile_cxx}" "${target_profile_common_cxx_flags[@]}" \
    "${cpp_source_path}" "${target_profile_core_sources[@]}" -o "${cpp_binary_path}"

probe_status=0
if ! "${binary_path}"; then
    printf 'C target-profile boundary probe failed\n' >&2
    probe_status=1
fi
if ! "${cpp_binary_path}"; then
    printf 'C++ target-profile concurrency/representation probe failed\n' >&2
    probe_status=1
fi
if ! swift test --package-path "${repo_root}" \
    --filter BeebMachineTests.testInvalidProfileMatrixOwnsValuesMapsErrorsAndPreservesActiveModelB; then
    printf 'Swift target-profile representation probe failed\n' >&2
    probe_status=1
fi
exit "${probe_status}"
