#include "beeb/profile.hpp"

namespace beeb {

MachineTargetProfile MachineTargetProfile::modelB() noexcept {
    return MachineTargetProfile{
        machineProfileSchemaVersion,
        ProfileComponentIdentity{modelBBaseIdentifier, machineProfileComponentVersion}};
}

MachineTargetProfile MachineTargetProfile::modelBPlus64K() noexcept {
    return MachineTargetProfile{
        machineProfileSchemaVersion,
        ProfileComponentIdentity{modelBPlus64KBaseIdentifier, machineProfileComponentVersion}};
}

ProfileValidation validateMachineTargetProfile(const MachineTargetProfile& profile) {
    if (profile == MachineTargetProfile::modelB()) return {ProfileSupport::supported, {}};
    if (profile == MachineTargetProfile::modelBPlus64K()) {
        return {ProfileSupport::recognisedUnavailable,
                "BBC Model B+ 64K is recognised but machine support is unavailable"};
    }
    return {ProfileSupport::unknown, "machine profile is not recognised"};
}

} // namespace beeb
