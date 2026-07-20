#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

namespace beeb {

/// Version of the bounded in-memory machine-profile contract.
inline constexpr std::uint16_t machineProfileSchemaVersion = 1;
/// Permanent base identifier for the BBC Microcomputer Model B.
inline constexpr std::uint32_t modelBBaseIdentifier = UINT32_C(0x00000001);
/// Permanent base identifier for the BBC Model B+ 64K.
inline constexpr std::uint32_t modelBPlus64KBaseIdentifier = UINT32_C(0x00000002);
/// Version shared by the two base identities assigned in schema version 1.
inline constexpr std::uint16_t machineProfileComponentVersion = 1;
/// Maximum expansion entries carried by the version-1 in-memory value.
inline constexpr std::size_t machineProfileExpansionCapacity = 16;
/// Number of expansion identities assigned by this feature.
inline constexpr std::size_t knownMachineProfileExpansionCount = 0;

/// Immutable raw identity for one base machine or expansion component.
///
/// Raw identifiers remain representable so validation can reject later or
/// unknown values without losing information. Identifier/version pairs, once
/// assigned, are permanent. The reserved field is part of the semantic value
/// and must be zero for schema version 1.
class ProfileComponentIdentity final {
  public:
    /// Creates a raw component value without classifying it.
    /// @param identifier Stable 32-bit identity code, or zero in a malformed fixture.
    /// @param version Version of the identified component contract.
    /// @param reserved Reserved schema field; valid version-1 values use zero.
    constexpr ProfileComponentIdentity(std::uint32_t identifier = 0, std::uint16_t version = 0,
                                       std::uint16_t reserved = 0) noexcept
        : identifier_(identifier), version_(version), reserved_(reserved) {}

    /// Returns the unmodified raw identity code.
    [[nodiscard]] constexpr std::uint32_t identifier() const noexcept { return identifier_; }
    /// Returns the unmodified component-contract version.
    [[nodiscard]] constexpr std::uint16_t version() const noexcept { return version_; }
    /// Returns the versioned reserved field.
    [[nodiscard]] constexpr std::uint16_t reserved() const noexcept { return reserved_; }

    /// Compares the complete raw semantic value.
    friend constexpr bool operator==(const ProfileComponentIdentity&,
                                     const ProfileComponentIdentity&) = default;

  private:
    std::uint32_t identifier_ = 0;
    std::uint16_t version_ = 0;
    std::uint16_t reserved_ = 0;
};

/// Immutable bounded identity requested for one machine runtime.
///
/// This value is an in-process semantic carrier, not a persisted byte format.
/// It deliberately provides no mutators: callers construct a complete raw
/// value for later validation or use one of the two canonical constructors.
/// Unknown raw values remain intact, while unused expansion slots participate
/// in equality so canonical values have exactly one representation.
class MachineTargetProfile final {
  public:
    /// Fixed storage used by every version-1 profile value.
    using ExpansionStorage = std::array<ProfileComponentIdentity, machineProfileExpansionCapacity>;

    /// Creates a raw profile value without validating or canonicalizing it.
    /// @param schemaVersion Profile-envelope version.
    /// @param base Raw base-machine component.
    /// @param expansionCount Declared used entries; may exceed capacity in malformed fixtures.
    /// @param expansions Fixed storage copied into this independently owned value.
    constexpr MachineTargetProfile(std::uint16_t schemaVersion = 0,
                                   ProfileComponentIdentity base = {},
                                   std::uint16_t expansionCount = 0,
                                   ExpansionStorage expansions = {}) noexcept
        : schemaVersion_(schemaVersion), base_(base), expansionCount_(expansionCount),
          expansions_(expansions) {}

    /// Returns the canonical BBC Microcomputer Model B identity.
    [[nodiscard]] static MachineTargetProfile modelB() noexcept;
    /// Returns the canonical BBC Model B+ 64K identity without claiming machine support.
    [[nodiscard]] static MachineTargetProfile modelBPlus64K() noexcept;

    /// Returns the raw profile-envelope version.
    [[nodiscard]] constexpr std::uint16_t schemaVersion() const noexcept { return schemaVersion_; }
    /// Returns the independently owned base-machine component.
    [[nodiscard]] constexpr const ProfileComponentIdentity& base() const noexcept { return base_; }
    /// Returns the declared expansion count without indexing storage.
    [[nodiscard]] constexpr std::uint16_t expansionCount() const noexcept {
        return expansionCount_;
    }
    /// Returns all fixed expansion slots, including required zero-filled unused slots.
    [[nodiscard]] constexpr const ExpansionStorage& expansions() const noexcept {
        return expansions_;
    }

    /// Compares every semantic field, including unused fixed storage.
    friend constexpr bool operator==(const MachineTargetProfile&,
                                     const MachineTargetProfile&) = default;

  private:
    std::uint16_t schemaVersion_ = 0;
    ProfileComponentIdentity base_;
    std::uint16_t expansionCount_ = 0;
    ExpansionStorage expansions_{};
};

/// Stable support classification, separate from the transported identity.
enum class ProfileSupport {
    supported,             ///< The canonical profile can construct a machine.
    recognisedUnavailable, ///< Identity is known but machine behavior is not implemented.
    unknown,               ///< A structurally usable code or version is unassigned.
    incompatible,          ///< Known components cannot be combined in their supplied roles.
    malformed,             ///< The bounded envelope violates schema invariants.
};

/// Owned result of pure machine-profile classification.
struct ProfileValidation {
    ProfileSupport support = ProfileSupport::malformed; ///< Stable result category.
    std::string message; ///< Owned diagnostic; empty only for a supported value.

    /// Compares the complete operation-owned classification.
    friend bool operator==(const ProfileValidation&, const ProfileValidation&) = default;
};

/// Classifies a profile without constructing or mutating a machine.
///
/// Canonical Model B is supported and canonical Model B+ 64K is recognised but
/// unavailable. Later validation work refines malformed, unknown and
/// incompatible inputs without changing either committed identity.
/// @param profile Complete independently owned profile value.
/// @return Supported with no message for canonical Model B, or an owned rejection.
[[nodiscard]] ProfileValidation validateMachineTargetProfile(const MachineTargetProfile& profile);

} // namespace beeb
