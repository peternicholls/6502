#!/usr/bin/env bash

set -euo pipefail
source "$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/testlib.sh"

target_profile_prepare_build_dir
source_path="${target_profile_build_dir}/profile-contract.cpp"
object_path="${target_profile_build_dir}/profile-contract.o"
binary_path="${target_profile_build_dir}/profile-contract"

cat >"${source_path}" <<'CPP'
#include "beeb/profile.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <type_traits>

static_assert(beeb::machineProfileSchemaVersion == 1);
static_assert(beeb::modelBBaseIdentifier == UINT32_C(0x00000001));
static_assert(beeb::modelBPlus64KBaseIdentifier == UINT32_C(0x00000002));
static_assert(beeb::machineProfileComponentVersion == 1);
static_assert(beeb::machineProfileExpansionCapacity == 16);
static_assert(beeb::knownMachineProfileExpansionCount == 0);

static_assert(std::is_same_v<decltype(beeb::ProfileComponentIdentity{}.identifier()),
                             std::uint32_t>);
static_assert(std::is_same_v<decltype(beeb::ProfileComponentIdentity{}.version()),
                             std::uint16_t>);
static_assert(std::is_same_v<decltype(beeb::ProfileComponentIdentity{}.reserved()),
                             std::uint16_t>);
static_assert(std::is_same_v<beeb::MachineTargetProfile::ExpansionStorage,
                             std::array<beeb::ProfileComponentIdentity, 16>>);
static_assert(std::is_same_v<decltype(beeb::MachineTargetProfile{}.schemaVersion()),
                             std::uint16_t>);
static_assert(std::is_same_v<decltype(beeb::MachineTargetProfile{}.expansionCount()),
                             std::uint16_t>);
static_assert(std::is_copy_constructible_v<beeb::ProfileComponentIdentity>);
static_assert(std::is_copy_constructible_v<beeb::MachineTargetProfile>);

constexpr beeb::ProfileComponentIdentity rawComponent{UINT32_C(0xf0000001), 23, 0};
static_assert(rawComponent.identifier() == UINT32_C(0xf0000001));
static_assert(rawComponent.version() == 23);
static_assert(rawComponent.reserved() == 0);

int main() {
    const auto modelB = beeb::MachineTargetProfile::modelB();
    const auto repeatedModelB = beeb::MachineTargetProfile::modelB();
    const auto modelBPlus64K = beeb::MachineTargetProfile::modelBPlus64K();

    if (modelB != repeatedModelB || modelB == modelBPlus64K) return 1;
    if (modelB.schemaVersion() != beeb::machineProfileSchemaVersion) return 2;
    if (modelB.base() != beeb::ProfileComponentIdentity{
                             beeb::modelBBaseIdentifier,
                             beeb::machineProfileComponentVersion})
        return 3;
    if (modelBPlus64K.base() != beeb::ProfileComponentIdentity{
                                      beeb::modelBPlus64KBaseIdentifier,
                                      beeb::machineProfileComponentVersion})
        return 4;
    if (modelB.expansionCount() != 0 || modelBPlus64K.expansionCount() != 0) return 5;
    for (const auto& expansion : modelB.expansions()) {
        if (expansion != beeb::ProfileComponentIdentity{}) return 6;
    }
    for (const auto& expansion : modelBPlus64K.expansions()) {
        if (expansion != beeb::ProfileComponentIdentity{}) return 7;
    }
    return 0;
}
CPP

"${target_profile_cxx}" "${target_profile_common_cxx_flags[@]}" -c \
    "${source_path}" -o "${object_path}"
"${target_profile_cxx}" "${target_profile_common_cxx_flags[@]}" \
    "${object_path}" "${repo_root}/Sources/BeebCore/src/profile.cpp" \
    -o "${binary_path}"
"${binary_path}"
