#include "beeb/profile.hpp"

#include <string>

namespace beeb {

namespace {

bool isKnownBaseIdentifier(std::uint32_t identifier) noexcept {
    return identifier == modelBBaseIdentifier || identifier == modelBPlus64KBaseIdentifier;
}

std::string identifierText(std::uint32_t identifier) {
    constexpr char digits[] = "0123456789abcdef";
    std::string result = "0x00000000";
    for (std::size_t index = 0; index < 8; ++index) {
        const auto shift = static_cast<unsigned>((7 - index) * 4);
        result[index + 2] = digits[(identifier >> shift) & 0x0fu];
    }
    return result;
}

ProfileValidation malformed(std::string message) {
    return {ProfileSupport::malformed, std::move(message)};
}

ProfileValidation unknown(std::string message) {
    return {ProfileSupport::unknown, std::move(message)};
}

} // namespace

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
    // The declared count is attacker-controlled raw input. Reject it before
    // obtaining or indexing the fixed expansion storage.
    const auto expansionCount = profile.expansionCount();
    if (expansionCount > machineProfileExpansionCapacity) {
        return malformed("machine profile expansion count exceeds the 16-entry capacity");
    }

    const auto& base = profile.base();
    if (profile.schemaVersion() == 0) {
        return malformed("machine profile schema version must be non-zero");
    }
    if (base.identifier() == 0) {
        return malformed("machine profile base identifier must be non-zero");
    }
    if (base.version() == 0) {
        return malformed("machine profile base component version must be non-zero");
    }
    if (base.reserved() != 0) {
        return malformed("machine profile base reserved field must be zero");
    }

    const auto& expansions = profile.expansions();
    for (std::size_t index = 0; index < expansionCount; ++index) {
        const auto& expansion = expansions[index];
        if (expansion.identifier() == 0) {
            return malformed("used machine profile expansion identifiers must be non-zero");
        }
        if (expansion.version() == 0) {
            return malformed("used machine profile expansion versions must be non-zero");
        }
        if (expansion.reserved() != 0) {
            return malformed("used machine profile expansion reserved fields must be zero");
        }
        if (index != 0) {
            const auto& previous = expansions[index - 1];
            const bool strictlyOrdered = previous.identifier() < expansion.identifier() ||
                                         (previous.identifier() == expansion.identifier() &&
                                          previous.version() < expansion.version());
            if (!strictlyOrdered) {
                return malformed(
                    "machine profile expansions must be strictly ordered without duplicates");
            }
        }
    }
    for (std::size_t index = expansionCount; index < expansions.size(); ++index) {
        if (expansions[index] != ProfileComponentIdentity{}) {
            return malformed("unused machine profile expansion slots must be zero");
        }
    }

    // Structural defects always win. Only a canonical envelope reaches the
    // forward-compatibility and component-role classification below.
    if (profile.schemaVersion() != machineProfileSchemaVersion) {
        return unknown("machine profile schema version " + std::to_string(profile.schemaVersion()) +
                       " is unknown");
    }
    if (!isKnownBaseIdentifier(base.identifier())) {
        return unknown("machine profile base identifier " + identifierText(base.identifier()) +
                       " is unknown");
    }
    if (base.version() != machineProfileComponentVersion) {
        return unknown("machine profile base component version " + std::to_string(base.version()) +
                       " is unknown");
    }

    for (std::size_t index = 0; index < expansionCount; ++index) {
        const auto& expansion = expansions[index];
        if (!isKnownBaseIdentifier(expansion.identifier())) {
            return unknown("machine profile expansion identifier " +
                           identifierText(expansion.identifier()) + " is unknown");
        }
        if (expansion.version() != machineProfileComponentVersion) {
            return unknown("machine profile expansion component version " +
                           std::to_string(expansion.version()) + " is unknown");
        }
    }

    if (expansionCount != 0) {
        return {ProfileSupport::incompatible,
                "base-machine identifiers cannot be used as expansion components"};
    }

    if (base.identifier() == modelBBaseIdentifier) return {ProfileSupport::supported, {}};
    if (base.identifier() == modelBPlus64KBaseIdentifier) {
        return {ProfileSupport::recognisedUnavailable,
                "BBC Model B+ 64K is recognised but machine support is unavailable"};
    }

    return unknown("machine profile is not recognised");
}

} // namespace beeb
