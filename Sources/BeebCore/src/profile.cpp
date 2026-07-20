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

} // namespace beeb
