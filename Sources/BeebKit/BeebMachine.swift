import BeebCore
import Foundation

// C0-DOC-RATIONALE: docs/code/host-boundary.md owns Swift/C lifetime and recovery.

/// Owned raw identity for one base-machine or expansion component.
///
/// Identifiers remain raw so a later code can cross Swift without being
/// truncated to a closed enum. Assigned identifier/version pairs are stable;
/// the reserved field must be zero for a canonical schema-version-1 value.
public struct BeebMachineProfileComponent: Sendable, Equatable {
    /// Stable raw 32-bit identity code.
    public let identifier: UInt32
    /// Version of the identified component contract.
    public let version: UInt16
    /// Reserved for a later profile schema; valid version-1 values use zero.
    public let reserved: UInt16

    /// Creates an independently owned raw component value.
    /// - Parameters:
    ///   - identifier: Stable identity code, or a raw fixture value.
    ///   - version: Version of the identified component contract.
    ///   - reserved: Reserved schema field; defaults to zero.
    public init(identifier: UInt32, version: UInt16, reserved: UInt16 = 0) {
        self.identifier = identifier
        self.version = version
        self.reserved = reserved
    }

    /// Copies all semantic fields out of the imported C aggregate.
    fileprivate init(_ value: beeb_profile_component) {
        self.init(
            identifier: value.identifier,
            version: value.version,
            reserved: value.reserved
        )
    }

    /// Copies the raw Swift fields into one imported C component value.
    fileprivate var cValue: beeb_profile_component {
        var value = beeb_profile_component()
        value.identifier = identifier
        value.version = version
        value.reserved = reserved
        return value
    }
}

/// Stable support classification for one complete machine-profile value.
///
/// Identity and implementation availability remain separate: a recognised
/// profile may cross the boundary even when its machine behavior is absent.
public enum BeebMachineProfileSupport: Sendable, Equatable {
    /// The profile can construct a machine in this release.
    case supported
    /// The identity is known, but its machine behavior is not implemented.
    case recognisedUnavailable
    /// An identifier or component version has no assigned contract.
    case unknown
    /// Known components cannot be combined in the supplied roles.
    case incompatible
    /// The bounded profile envelope violates its schema invariants.
    case malformed
}

/// Immutable, owned identity requested for one machine runtime.
///
/// The Swift value preserves raw component identifiers and owns its expansion
/// array. It is not a persisted byte format and does not imply that a named
/// machine is constructible. Schema version 1 can classify at most sixteen
/// expansion entries; no expansion identity is assigned by this feature.
public struct BeebMachineProfile: Sendable, Equatable {
    /// Current bounded in-memory profile schema.
    public static let schemaVersion = UInt16(BEEB_MACHINE_PROFILE_SCHEMA_VERSION)
    /// Maximum expansion entries admitted by schema version 1.
    public static let expansionCapacity = Int(BEEB_MACHINE_PROFILE_EXPANSION_CAPACITY)

    /// Canonical BBC Microcomputer Model B identity.
    public static let modelB = BeebMachineProfile(beeb_machine_profile_model_b())
    /// Canonical BBC Model B+ 64K identity, without a machine-support claim.
    public static let modelBPlus64K =
        BeebMachineProfile(beeb_machine_profile_model_b_plus_64k())

    /// Raw profile-envelope version.
    public let schemaVersion: UInt16
    /// Owned base-machine component.
    public let base: BeebMachineProfileComponent
    /// Owned raw expansion entries in caller-supplied order.
    public let expansions: [BeebMachineProfileComponent]

    /// Creates a raw profile value without classifying or canonicalizing it.
    /// - Parameters:
    ///   - schemaVersion: Profile-envelope version.
    ///   - base: Raw base-machine identity.
    ///   - expansions: Raw expansion entries; validation enforces the versioned bound.
    public init(
        schemaVersion: UInt16,
        base: BeebMachineProfileComponent,
        expansions: [BeebMachineProfileComponent]
    ) {
        self.schemaVersion = schemaVersion
        self.base = base
        self.expansions = expansions
    }

    /// Unambiguous name for an assigned base identity or a raw unassigned
    /// identifier.
    ///
    /// Structural and compatibility failures retain the assigned base name so
    /// diagnostics can identify the user's request. Unassigned values
    /// deliberately receive no reserved future-option name.
    public var displayName: String {
        switch base.identifier {
        case Self.modelB.base.identifier:
            return "BBC Microcomputer Model B"
        case Self.modelBPlus64K.base.identifier:
            return "BBC Model B+ 64K"
        default:
            return String(format: "Unknown machine profile 0x%08X", base.identifier)
        }
    }

    /// Pure support classification copied from the C/core validator.
    ///
    /// A later imported C enum value is contained as `unknown` and can never
    /// be interpreted as support by an older Swift wrapper.
    public var support: BeebMachineProfileSupport {
        classification.support
    }

    /// Copies one pure C classification into Swift-owned category and text.
    fileprivate var classification: (
        status: beeb_status,
        support: BeebMachineProfileSupport,
        message: String
    ) {
        var profile = cValue
        var validation = beeb_machine_profile_validation()
        let status = beeb_validate_machine_profile(&profile, &validation)
        guard status.code == BEEB_STATUS_OK else { return (status, .unknown, "") }
        let support: BeebMachineProfileSupport
        switch validation.support {
        case .BEEB_MACHINE_PROFILE_SUPPORTED: support = .supported
        case .BEEB_MACHINE_PROFILE_RECOGNISED_UNAVAILABLE: support = .recognisedUnavailable
        case .BEEB_MACHINE_PROFILE_UNKNOWN: support = .unknown
        case .BEEB_MACHINE_PROFILE_INCOMPATIBLE: support = .incompatible
        case .BEEB_MACHINE_PROFILE_MALFORMED: support = .malformed
        @unknown default: support = .unknown
        }
        var message = validation.message
        let ownedMessage = withUnsafePointer(to: &message) { pointer in
            pointer.withMemoryRebound(
                to: CChar.self,
                capacity: Int(BEEB_STATUS_MESSAGE_CAPACITY)
            ) {
                String(cString: $0)
            }
        }
        return (status, support, ownedMessage)
    }

    /// Copies a canonical fixed C aggregate into Swift-owned fields.
    fileprivate init(_ value: beeb_machine_profile) {
        var storage = value.expansions
        let usedCount = min(Int(value.expansion_count), Self.expansionCapacity)
        let copiedExpansions = withUnsafeBytes(of: &storage) { bytes in
            Array(bytes.bindMemory(to: beeb_profile_component.self).prefix(usedCount))
                .map(BeebMachineProfileComponent.init)
        }
        self.init(
            schemaVersion: value.schema_version,
            base: BeebMachineProfileComponent(value.base),
            expansions: copiedExpansions
        )
    }

    /// Copies the complete Swift value into fixed C storage without truncating
    /// the declared count used by later validation.
    fileprivate var cValue: beeb_machine_profile {
        var value = beeb_machine_profile()
        value.schema_version = schemaVersion
        value.base = base.cValue
        value.expansion_count = UInt16(clamping: expansions.count)
        withUnsafeMutableBytes(of: &value.expansions) { bytes in
            let slots = bytes.bindMemory(to: beeb_profile_component.self)
            for (index, expansion) in expansions.prefix(Self.expansionCapacity).enumerated() {
                slots[index] = expansion.cValue
            }
        }
        return value
    }
}

/// Stable Swift mapping of the C runtime status categories.
public enum BeebStatusCategory: Sendable, Equatable {
    /// Operation completed successfully.
    case ok
    /// No complete output value is currently retained.
    case empty
    /// Partial output was returned with an exact demand shortfall.
    case underrun
    /// Oldest bounded output was discarded under producer pressure.
    case overrun
    /// A caller or fixed output capacity was exceeded.
    case capacityExceeded
    /// Output conversion or production failed without corrupting the runtime.
    case outputProductionFailed
    /// A pointer, size, value, or output was invalid.
    case invalidArgument
    /// The command was not legal in the current lifecycle state.
    case invalidState
    /// Emulated execution faulted at a safe point.
    case executionFailed
    /// A required allocation or capacity could not be obtained.
    case resourceExhausted
    /// Runtime or requested capability is unavailable.
    case unavailable
    /// Reserved mapping for an owner-thread producer that would have deadlocked.
    case reentrantCall
    /// An unexpected implementation failure was contained.
    case internalFailure
}

/// Lifecycle state observed at a serialized runtime safe point.
public enum BeebRuntimeState: Sendable, Equatable {
    /// Quiescent and accepting commands.
    case paused
    /// Executing deterministic slices.
    case running
    /// Execution failed and reset is required.
    case faulted
    /// Acceptance stopped while accepted work drains.
    case shuttingDown
}

/// A value snapshot of the emulated 6502's programmer-visible state.
public struct BeebCPUState: Sendable {
    /// Accumulator register.
    public let a: UInt8
    /// X index register.
    public let x: UInt8
    /// Y index register.
    public let y: UInt8
    /// Stack pointer register.
    public let stackPointer: UInt8
    /// Processor status flags.
    public let status: UInt8
    /// Program counter.
    public let programCounter: UInt16
    /// Total completed CPU cycles since the current state was established.
    public let cycles: UInt64
}

/// An independently owned copy of one rendered video frame.
public struct BeebVideoFrame: Sendable {
    /// Pixel width.
    public let width: Int
    /// Pixel height.
    public let height: Int
    /// Monotonic CRTC frame number.
    public let number: UInt64
    /// Packed 8-bit RGBA pixels in row-major order.
    public let rgba: Data
}

/// Independently owned result of one continuous 48 kHz mono audio drain.
public struct BeebAudioDrain: Sendable, Equatable {
    /// Emulated sequence of the first copied sample.
    public let firstSample: UInt64
    /// Requested maximum sample count.
    public let requested: Int
    /// Valid owned mono Float32 samples in FIFO order.
    public let samples: [Float]
    /// Exact requested samples unavailable during this operation.
    public let shortfall: Int
    /// Post-drain samples needed to reach the 2,048-sample target.
    public let demand: Int
    /// Cumulative samples discarded at capacity or reset.
    public let overrunCount: UInt64
    /// Cumulative exact requested-sample shortfall.
    public let underrunCount: UInt64
}

/// Exact monotonic accounting carried by one output diagnostic observation.
public struct BeebOutputCounters: Sendable, Equatable {
    /// Complete frames offered to the bounded FIFO.
    public let framesProduced: UInt64
    /// Complete frames transferred to consumers.
    public let framesConsumed: UInt64
    /// Frames discarded at capacity or reset.
    public let framesDropped: UInt64
    /// Continuous audio samples offered to the bounded FIFO.
    public let audioSamplesProduced: UInt64
    /// Continuous audio samples transferred to consumers.
    public let audioSamplesConsumed: UInt64
    /// Audio samples discarded at capacity or reset.
    public let audioSamplesOverrun: UInt64
    /// Exact requested audio-sample shortfall.
    public let audioSamplesUnderrun: UInt64
}

/// Owned owner-consistent observation of progress and bounded output pressure.
public struct BeebOutputDiagnostics: Sendable, Equatable {
    /// Completed emulated CPU cycles.
    public let totalCycles: UInt64
    /// Latest complete output-frame identity.
    public let latestFrameNumber: UInt64
    /// Complete frames currently retained.
    public let frameDepth: Int
    /// Fixed completed-frame capacity.
    public let frameCapacity: Int
    /// Continuous audio samples currently retained.
    public let audioDepth: Int
    /// Fixed continuous-audio capacity.
    public let audioCapacity: Int
    /// Samples needed to reach the target depth.
    public let audioDemand: Int
    /// Exact frame and audio flow accounting.
    public let counters: BeebOutputCounters
    /// Latest recoverable output outcome at this boundary.
    public let lastStatus: BeebStatusCategory
}

/// Identity of one completed-instruction and fully advanced-device boundary.
public struct BeebSafePoint: Sendable, Equatable {
    /// Total completed CPU cycles.
    public let cpuCycles: UInt64
    /// Latest completed frame number.
    public let frameNumber: UInt64
    /// Runtime lifecycle state at the boundary.
    public let state: BeebRuntimeState
    /// Latest total command/execution ledger identity.
    public let ledgerSequence: UInt64
}

/// Retained execution failure available until reset succeeds.
public struct BeebRuntimeFault: Sendable, Equatable {
    /// Stable diagnostic from the execution failure.
    public let message: String
    /// Safe point at which fault detail was observed.
    public let safePoint: BeebSafePoint
}

/// The two user-owned firmware roles required by the first Model B workflow.
public enum BeebFirmwareRole: Sendable, Equatable {
    /// The fixed 16 KiB operating-system region.
    case operatingSystem
    /// The language ROM installed by the M1 host workflow.
    case language
}

/// Errors produced by validating input or crossing the C runtime boundary.
public enum BeebError: LocalizedError {
    /// The operating-system ROM was not exactly 16 KiB.
    case invalidOSROM
    /// The sideways-ROM bank or byte count was outside the supported range.
    case invalidSidewaysROM
    /// The disc bytes did not describe a supported DFS SSD or DSD geometry.
    case invalidDiscImage
    /// The drive number was not zero or one.
    case invalidDrive
    /// The audio frame count or sample rate was invalid.
    case invalidAudioRequest
    /// The keyboard matrix coordinates were outside 0...15.
    case invalidKey
    /// A recognised machine identity has no implementation in this release.
    case machineProfileUnavailable(BeebMachineProfile)
    /// The bounded profile envelope violates its schema invariants.
    case malformedMachineProfile(BeebMachineProfile, String)
    /// The profile contains an identifier or version with no assigned contract.
    case unknownMachineProfile(BeebMachineProfile, String)
    /// Known components cannot be combined in their supplied roles.
    case incompatibleMachineProfile(BeebMachineProfile, String)
    /// Recoverable audio pressure carrying every valid partial sample and counter.
    case audioPressure(BeebStatusCategory, BeebAudioDrain)
    /// A C status category and its operation-scoped diagnostic.
    case coreStatus(BeebStatusCategory, String)

    /// A user-facing description of this error.
    public var errorDescription: String? {
        switch self {
        case .invalidOSROM:
            return "A BBC Model B OS ROM must be exactly 16 KiB."
        case .invalidSidewaysROM:
            return "Use bank 0–15 and a sideways ROM between 1 byte and 16 KiB."
        case .invalidDiscImage:
            return "The disc is not a valid 40/80-track SSD or DSD image."
        case .invalidDrive:
            return "The drive number must be 0 or 1."
        case .invalidAudioRequest:
            return "Audio needs a positive frame count and finite positive sample rate."
        case .invalidKey:
            return "Keyboard row and column must both be in 0...15."
        case let .machineProfileUnavailable(profile):
            return "\(profile.displayName) is recognised, but machine support is not yet available."
        case let .malformedMachineProfile(profile, message):
            return "\(profile.displayName) is malformed: \(message)"
        case let .unknownMachineProfile(profile, message):
            return "\(profile.displayName) is unknown: \(message)"
        case let .incompatibleMachineProfile(profile, message):
            return "\(profile.displayName) is incompatible: \(message)"
        case let .audioPressure(category, drain):
            return "Audio reported \(category) after copying \(drain.samples.count) samples " +
                "with a shortfall of \(drain.shortfall)."
        case let .coreStatus(category, message):
            return "The emulator core reported \(category): \(message)"
        }
    }
}

/// Concurrent Swift owner of one deterministic BBC Model B runtime.
///
/// The C++ owner serializes all machine operations. This wrapper adds no second
/// lock: each method submits one synchronous C command and immediately maps its
/// operation-owned status or copies its successful output into a Swift value.
/// Callers may retain the instance across concurrency domains. Deinitialization
/// performs blocking shutdown after the final strong reference is released.
public final class BeebMachine: @unchecked Sendable {
    /// Fixed sideways-ROM bank used by the first Model B language workflow.
    public static let languageROMBank: UInt8 = 12

    /// Opaque C token whose registry admission keeps concurrent calls alive through return.
    private let handle: OpaquePointer

    /// Creates a paused canonical Model B machine with no ROM or disc loaded.
    ///
    /// This convenience delegates to ``init(profile:)`` and is never used as a
    /// fallback for an invalid explicit profile.
    /// - Throws: ``BeebError/coreStatus(_:_:)`` if runtime creation fails.
    public convenience init() throws {
        try self.init(profile: .modelB)
    }

    /// Creates a paused machine for one explicit profile.
    /// - Parameter profile: Complete immutable identity copied into the C boundary.
    /// - Throws: A profile-specific ``BeebError`` for a classified rejection,
    ///   or ``BeebError/coreStatus(_:_:)`` when validation or creation itself fails.
    public init(profile: BeebMachineProfile) throws {
        let classification = profile.classification
        try Self.check(classification.status)
        switch classification.support {
        case .supported:
            break
        case .recognisedUnavailable:
            throw BeebError.machineProfileUnavailable(profile)
        case .malformed:
            throw BeebError.malformedMachineProfile(profile, classification.message)
        case .unknown:
            throw BeebError.unknownMachineProfile(profile, classification.message)
        case .incompatible:
            throw BeebError.incompatibleMachineProfile(profile, classification.message)
        }

        var cProfile = profile.cValue
        var created: OpaquePointer?
        let status = beeb_create_with_profile(&cProfile, &created)
        try Self.check(status)
        guard let created else {
            throw BeebError.coreStatus(
                .internalFailure, "C runtime succeeded without returning a handle")
        }
        handle = created
    }

    deinit { _ = beeb_destroy(handle) }

    /// Immutable active identity copied through the runtime owner.
    ///
    /// The returned value owns every component and query storage. Reading it
    /// neither caches host truth nor advances emulated time.
    /// - Throws: ``BeebError/coreStatus(_:_:)`` if the runtime is unavailable.
    public var profile: BeebMachineProfile {
        get throws {
            var profile = beeb_machine_profile()
            try Self.check(beeb_get_machine_profile(handle, &profile))
            return BeebMachineProfile(profile)
        }
    }

    /// Lifecycle state from one FIFO safe point.
    /// - Throws: ``BeebError/coreStatus(_:_:)`` if the runtime is unavailable.
    public var state: BeebRuntimeState {
        get throws {
            var state = BEEB_RUNTIME_STATE_PAUSED
            try Self.check(beeb_get_runtime_state(handle, &state))
            return Self.runtimeState(state)
        }
    }

    /// Validates and installs user-owned firmware for one typed Model B role.
    ///
    /// The language role deliberately uses the fixed M1 bank so the first host
    /// workflow does not add bank-selection UI. The runtime copies bytes before
    /// accepting each owner-serialized command; rejected data leaves the prior
    /// firmware installation unchanged.
    /// - Parameters:
    ///   - data: User-owned ROM bytes copied into the runtime.
    ///   - role: Operating-system or language-ROM assignment.
    /// - Throws: A role-specific validation error or typed core status.
    public func loadFirmware(_ data: Data, role: BeebFirmwareRole) throws {
        switch role {
        case .operatingSystem:
            try loadOSROM(data)
        case .language:
            try loadSidewaysROM(data, bank: Self.languageROMBank)
        }
    }

    /// Starts sustained deterministic execution; running is idempotent.
    /// - Throws: ``BeebError/coreStatus(_:_:)`` for lifecycle or runtime failures.
    public func start() throws { try Self.check(beeb_start(handle)) }

    /// Pauses at a safe point; paused is idempotent.
    /// - Throws: ``BeebError/coreStatus(_:_:)`` for lifecycle or runtime failures.
    public func pause() throws { try Self.check(beeb_pause(handle)) }

    /// Validates and copies a BBC Model B operating-system ROM.
    /// - Parameter data: Exactly 16 KiB of ROM bytes.
    /// - Throws: ``BeebError/invalidOSROM`` or a typed core status.
    public func loadOSROM(_ data: Data) throws {
        guard data.count == 16 * 1024 else { throw BeebError.invalidOSROM }
        let status = data.withUnsafeBytes { bytes in
            beeb_load_os_rom(
                handle, bytes.bindMemory(to: UInt8.self).baseAddress, bytes.count)
        }
        try Self.check(status)
    }

    /// Validates and copies a sideways ROM into one bank.
    /// - Parameters:
    ///   - data: Between one byte and 16 KiB.
    ///   - bank: Bank number in the range 0...15.
    /// - Throws: ``BeebError/invalidSidewaysROM`` or a typed core status.
    public func loadSidewaysROM(_ data: Data, bank: UInt8) throws {
        guard bank < 16, !data.isEmpty, data.count <= 16 * 1024 else {
            throw BeebError.invalidSidewaysROM
        }
        let status = data.withUnsafeBytes { bytes in
            beeb_load_sideways_rom(
                handle, bank, bytes.bindMemory(to: UInt8.self).baseAddress, bytes.count)
        }
        try Self.check(status)
    }

    /// Validates, copies, and mounts a DFS disc image.
    /// - Parameters:
    ///   - data: Complete 1...80-track image bytes.
    ///   - drive: Drive zero or one.
    ///   - doubleSided: `true` for interleaved DSD ordering; `false` for SSD.
    ///   - writable: Whether the runtime may modify its private image copy.
    /// - Throws: An input-specific ``BeebError`` or typed core status.
    public func mountDisc(
        _ data: Data,
        drive: Int = 0,
        doubleSided: Bool,
        writable: Bool = false
    ) throws {
        guard (0...1).contains(drive) else { throw BeebError.invalidDrive }
        let bytesPerTrack = 10 * 256 * (doubleSided ? 2 : 1)
        let tracks = data.count / bytesPerTrack
        guard data.count.isMultiple(of: bytesPerTrack), (1...80).contains(tracks) else {
            throw BeebError.invalidDiscImage
        }
        let status = data.withUnsafeBytes { bytes in
            beeb_mount_disc(
                handle,
                UInt32(drive),
                bytes.bindMemory(to: UInt8.self).baseAddress,
                bytes.count,
                doubleSided ? 1 : 0,
                writable ? 1 : 0
            )
        }
        try Self.check(status)
    }

    /// Resets CPU/devices and fractional audio timing, discards retained output with
    /// exact monotonic accounting, clears a fault, retains media, and finishes paused.
    /// - Throws: ``BeebError/coreStatus(_:_:)`` if reset cannot complete.
    public func reset() throws { try Self.check(beeb_reset(handle)) }

    /// Executes whole instructions while paused until the cycle budget is met.
    /// - Parameter cycles: Minimum CPU-cycle budget; zero performs no work.
    /// - Returns: Actual cycles, which may exceed the request by one instruction.
    /// - Throws: ``BeebError/coreStatus(_:_:)`` when bounded execution is invalid or faults.
    @discardableResult
    public func run(cycles: UInt64) throws -> UInt64 {
        var actual: UInt64 = 0
        try Self.check(beeb_run_cycles(handle, cycles, &actual))
        return actual
    }

    /// Executes while paused until a frame completes or the budget is met.
    /// - Parameter maximumCycles: Maximum cycle budget before returning `false`.
    /// - Returns: `true` when a new frame completed.
    /// - Throws: ``BeebError/coreStatus(_:_:)`` when bounded execution is invalid or faults.
    @discardableResult
    public func runToNextFrame(maximumCycles: UInt64 = 100_000) throws -> Bool {
        var completed: Int32 = 0
        try Self.check(beeb_run_until_frame(handle, maximumCycles, &completed))
        return completed != 0
    }

    /// Copies CPU registers and cycle count from one safe point.
    /// - Throws: ``BeebError/coreStatus(_:_:)`` if the runtime is unavailable.
    public func cpuState() throws -> BeebCPUState {
        var state = beeb_cpu_state()
        try Self.check(beeb_get_cpu_state(handle, &state))
        return BeebCPUState(
            a: state.a,
            x: state.x,
            y: state.y,
            stackPointer: state.sp,
            status: state.p,
            programCounter: state.pc,
            cycles: state.cycles
        )
    }

    /// Copies the latest C-owned frame into independently owned Swift storage.
    /// - Returns: A complete frame, or `nil` before one has rendered.
    /// - Throws: ``BeebError/coreStatus(_:_:)`` for runtime or ownership failures.
    public func videoFrame() throws -> BeebVideoFrame? {
        var frame = beeb_frame()
        try Self.check(beeb_get_frame(handle, &frame))
        defer { _ = beeb_frame_release(&frame) }
        guard frame.available != 0 else { return nil }
        guard let bytes = frame.rgba else {
            throw BeebError.coreStatus(
                .internalFailure, "available C frame did not contain RGBA storage")
        }
        let width = Int(frame.width)
        let height = Int(frame.height)
        let expectedSize = width * height * 4
        guard frame.rgba_size == expectedSize else {
            throw BeebError.coreStatus(
                .internalFailure, "C frame byte count did not match its dimensions")
        }
        return BeebVideoFrame(
            width: width,
            height: height,
            number: frame.number,
            rgba: Data(bytes: bytes, count: expectedSize)
        )
    }

    /// Transfers the oldest retained completed frame into independent Swift storage.
    /// - Returns: A complete packed-RGBA frame owned by the returned value.
    /// - Throws: ``BeebError/coreStatus(_:_:)`` with typed empty, lifecycle,
    /// allocation, or output-failure status. No runtime or C storage is borrowed.
    public func dequeueVideoFrame() throws -> BeebVideoFrame {
        var frame = beeb_frame()
        try Self.check(beeb_dequeue_frame(handle, &frame))
        defer { _ = beeb_frame_release(&frame) }
        guard frame.available != 0, let bytes = frame.rgba else {
            throw BeebError.coreStatus(
                .internalFailure, "C frame dequeue succeeded without RGBA storage")
        }
        let width = Int(frame.width)
        let height = Int(frame.height)
        let expectedSize = width * height * 4
        guard frame.rgba_size == expectedSize else {
            throw BeebError.coreStatus(
                .internalFailure, "C frame byte count did not match its dimensions")
        }
        return BeebVideoFrame(
            width: width,
            height: height,
            number: frame.number,
            rgba: Data(bytes: bytes, count: expectedSize)
        )
    }

    /// Renders independently owned mono samples without advancing CPU time.
    /// - Parameters:
    ///   - frames: Positive number of samples.
    ///   - sampleRate: Finite positive sample rate in hertz.
    /// - Throws: ``BeebError/invalidAudioRequest`` or a typed core status.
    public func renderAudio(frames: Int, sampleRate: Double) throws -> [Float] {
        guard frames > 0, sampleRate.isFinite, sampleRate > 0 else {
            throw BeebError.invalidAudioRequest
        }
        var output = Array(repeating: Float.zero, count: frames)
        let status = output.withUnsafeMutableBufferPointer { buffer in
            beeb_render_audio(handle, buffer.baseAddress, buffer.count, sampleRate)
        }
        try Self.check(status)
        return output
    }

    /// Drains continuous 48 kHz mono output into an independently owned value.
    /// - Parameter maximumSamples: Non-negative maximum sample count to copy.
    /// - Returns: Owned FIFO samples and the atomic post-drain pressure observation.
    /// - Throws: ``BeebError/audioPressure(_:_:)`` with a valid partial value on
    /// underrun, ``BeebError/invalidAudioRequest`` for a negative count, or a
    /// typed core status for lifecycle and resource failures.
    public func drainAudio(maximumSamples: Int) throws -> BeebAudioDrain {
        guard maximumSamples >= 0 else { throw BeebError.invalidAudioRequest }
        var output = Array(repeating: Float.zero, count: maximumSamples)
        var result = beeb_audio_drain_result()
        let status = output.withUnsafeMutableBufferPointer { buffer in
            beeb_drain_audio(handle, buffer.baseAddress, buffer.count, &result)
        }
        if status.code != BEEB_STATUS_OK, status.code != BEEB_STATUS_UNDERRUN {
            try Self.check(status)
        }
        guard result.copied <= output.count,
              result.shortfall == output.count - result.copied
        else {
            throw BeebError.coreStatus(
                .internalFailure, "C audio drain returned inconsistent copy accounting")
        }
        let drain = BeebAudioDrain(
            firstSample: result.first_sample,
            requested: maximumSamples,
            samples: Array(output.prefix(result.copied)),
            shortfall: result.shortfall,
            demand: result.demand,
            overrunCount: result.overrun_count,
            underrunCount: result.underrun_count
        )
        if status.code == BEEB_STATUS_UNDERRUN {
            throw BeebError.audioPressure(.underrun, drain)
        }
        try Self.check(status)
        return drain
    }

    /// Returns one owner-consistent progress and bounded-output observation.
    /// - Throws: ``BeebError/coreStatus(_:_:)`` if the runtime cannot provide
    /// the observation.
    public func outputDiagnostics() throws -> BeebOutputDiagnostics {
        var value = beeb_output_diagnostics()
        try Self.check(beeb_get_output_diagnostics(handle, &value))
        return BeebOutputDiagnostics(
            totalCycles: value.total_cycles,
            latestFrameNumber: value.latest_frame_number,
            frameDepth: value.frame_depth,
            frameCapacity: value.frame_capacity,
            audioDepth: value.audio_depth,
            audioCapacity: value.audio_capacity,
            audioDemand: value.audio_demand,
            counters: BeebOutputCounters(
                framesProduced: value.frames_produced,
                framesConsumed: value.frames_consumed,
                framesDropped: value.frames_dropped,
                audioSamplesProduced: value.audio_samples_produced,
                audioSamplesConsumed: value.audio_samples_consumed,
                audioSamplesOverrun: value.audio_samples_overrun,
                audioSamplesUnderrun: value.audio_samples_underrun
            ),
            lastStatus: Self.statusCategory(value.last_status)
        )
    }

    /// Calculates informational emulation speed from two host observations.
    /// - Parameters:
    ///   - before: Earlier owner-consistent diagnostic observation.
    ///   - after: Later observation with a nondecreasing cycle count.
    ///   - hostSeconds: Positive finite elapsed host-observation seconds.
    /// - Returns: Emulated seconds per host second.
    /// - Throws: ``BeebError/coreStatus(_:_:)`` with `invalidArgument` for an
    /// invalid interval or regressing observation.
    public static func emulationRate(
        from before: BeebOutputDiagnostics,
        to after: BeebOutputDiagnostics,
        hostSeconds: Double
    ) throws -> Double {
        var cBefore = beeb_output_diagnostics()
        cBefore.total_cycles = before.totalCycles
        var cAfter = beeb_output_diagnostics()
        cAfter.total_cycles = after.totalCycles
        var rate = 0.0
        try check(beeb_calculate_emulation_rate(&cBefore, &cAfter, hostSeconds, &rate))
        return rate
    }

    /// Changes one keyboard-matrix bit in FIFO order.
    /// - Parameters:
    ///   - column: Matrix column in the inclusive range `0...15`.
    ///   - row: Matrix row in the inclusive range `0...15`.
    ///   - pressed: `true` presses the key; `false` releases it.
    /// The request is serialized by the runtime owner and does not change
    /// lifecycle state.
    /// - Throws: ``BeebError/invalidKey`` or a typed core status.
    public func setKey(column: UInt8, row: UInt8, pressed: Bool) throws {
        guard column < 16, row < 16 else { throw BeebError.invalidKey }
        try Self.check(beeb_set_key(handle, column, row, pressed ? 1 : 0))
    }

    /// Changes BREAK state without inventing a lifecycle transition.
    /// - Parameter pressed: `true` asserts BREAK; `false` releases it.
    /// The request is FIFO-serialized and never starts, pauses, or resets the machine.
    /// - Throws: ``BeebError/coreStatus(_:_:)`` for runtime failures.
    public func setBreak(pressed: Bool) throws {
        try Self.check(beeb_set_break(handle, pressed ? 1 : 0))
    }

    /// Returns the current completed-instruction/device-tick identity.
    /// - Throws: ``BeebError/coreStatus(_:_:)`` if the runtime is unavailable.
    public func safePoint() throws -> BeebSafePoint {
        var point = beeb_safe_point()
        try Self.check(beeb_get_safe_point(handle, &point))
        return Self.safePoint(point)
    }

    /// Returns retained execution-fault detail, or `nil` outside faulted state.
    /// - Throws: ``BeebError/coreStatus(_:_:)`` if the runtime is unavailable.
    public func fault() throws -> BeebRuntimeFault? {
        var detail = beeb_fault_detail()
        try Self.check(beeb_get_fault(handle, &detail))
        guard detail.available != 0 else { return nil }
        return BeebRuntimeFault(
            message: Self.faultMessage(detail),
            safePoint: Self.safePoint(detail.safe_point)
        )
    }

    /// Converts one non-OK C status into its lossless typed Swift error.
    static func check(_ status: beeb_status) throws {
        guard status.code != BEEB_STATUS_OK else { return }
        throw BeebError.coreStatus(
            statusCategory(status.code), statusMessage(status))
    }

    /// Maps the closed C category vocabulary into the public Swift vocabulary.
    static func statusCategory(_ code: beeb_status_code) -> BeebStatusCategory {
        switch code {
        case BEEB_STATUS_OK: return .ok
        case BEEB_STATUS_EMPTY: return .empty
        case BEEB_STATUS_UNDERRUN: return .underrun
        case BEEB_STATUS_OVERRUN: return .overrun
        case BEEB_STATUS_CAPACITY_EXCEEDED: return .capacityExceeded
        case BEEB_STATUS_OUTPUT_FAILED: return .outputProductionFailed
        case BEEB_STATUS_INVALID_ARGUMENT: return .invalidArgument
        case BEEB_STATUS_INVALID_STATE: return .invalidState
        case BEEB_STATUS_EXECUTION_FAILED: return .executionFailed
        case BEEB_STATUS_RESOURCE_EXHAUSTED: return .resourceExhausted
        case BEEB_STATUS_UNAVAILABLE: return .unavailable
        case BEEB_STATUS_REENTRANT_CALL: return .reentrantCall
        default: return .internalFailure
        }
    }

    /// Maps a C lifecycle value without introducing mirrored mutable state.
    private static func runtimeState(_ state: beeb_runtime_state) -> BeebRuntimeState {
        switch state {
        case BEEB_RUNTIME_STATE_PAUSED: return .paused
        case BEEB_RUNTIME_STATE_RUNNING: return .running
        case BEEB_RUNTIME_STATE_FAULTED: return .faulted
        default: return .shuttingDown
        }
    }

    /// Copies a C safe-point aggregate into a Swift-owned value.
    private static func safePoint(_ point: beeb_safe_point) -> BeebSafePoint {
        BeebSafePoint(
            cpuCycles: point.cpu_cycles,
            frameNumber: point.frame_number,
            state: runtimeState(point.state),
            ledgerSequence: point.ledger_sequence
        )
    }

    /// Copies the fixed status tuple before the operation-scoped C value leaves scope.
    private static func statusMessage(_ status: beeb_status) -> String {
        var message = status.message
        return withUnsafePointer(to: &message) { pointer in
            pointer.withMemoryRebound(to: CChar.self, capacity: 256) {
                String(cString: $0)
            }
        }
    }

    /// Copies the fixed retained-fault tuple into Swift-owned string storage.
    private static func faultMessage(_ fault: beeb_fault_detail) -> String {
        var message = fault.message
        return withUnsafePointer(to: &message) { pointer in
            pointer.withMemoryRebound(to: CChar.self, capacity: 256) {
                String(cString: $0)
            }
        }
    }
}
