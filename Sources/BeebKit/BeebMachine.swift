import BeebCore
import Foundation

/// Stable Swift mapping of the C runtime status categories.
public enum BeebStatusCategory: Sendable, Equatable {
    /// A pointer, size, value, or output was invalid.
    case invalidArgument
    /// The command was not legal in the current lifecycle state.
    case invalidState
    /// Emulated execution faulted at a safe point.
    case executionFailed
    /// A required allocation or capacity could not be obtained.
    case resourceExhausted
    /// The runtime was shutting down or no longer accepted work.
    case unavailable
    /// Owner-thread re-entry would have deadlocked.
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
    /// Opaque C token whose registry admission keeps concurrent calls alive through return.
    private let handle: OpaquePointer

    /// Creates a paused machine with no ROM or disc loaded.
    /// - Throws: ``BeebError/coreStatus(_:_:)`` if runtime creation fails.
    public init() throws {
        var created: OpaquePointer?
        try Self.check(beeb_create(&created))
        guard let created else {
            throw BeebError.coreStatus(
                .internalFailure, "C runtime succeeded without returning a handle")
        }
        handle = created
    }

    deinit { _ = beeb_destroy(handle) }

    /// Lifecycle state from one FIFO safe point.
    public var state: BeebRuntimeState {
        get throws {
            var state = BEEB_RUNTIME_STATE_PAUSED
            try Self.check(beeb_get_runtime_state(handle, &state))
            return Self.runtimeState(state)
        }
    }

    /// Starts sustained deterministic execution; running is idempotent.
    public func start() throws { try Self.check(beeb_start(handle)) }

    /// Pauses at a safe point; paused is idempotent.
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

    /// Resets CPU and devices, clears a fault, retains media, and finishes paused.
    public func reset() throws { try Self.check(beeb_reset(handle)) }

    /// Executes whole instructions while paused until the cycle budget is met.
    /// - Parameter cycles: Minimum CPU-cycle budget; zero performs no work.
    /// - Returns: Actual cycles, which may exceed the request by one instruction.
    @discardableResult
    public func run(cycles: UInt64) throws -> UInt64 {
        var actual: UInt64 = 0
        try Self.check(beeb_run_cycles(handle, cycles, &actual))
        return actual
    }

    /// Executes while paused until a frame completes or the budget is met.
    /// - Parameter maximumCycles: Maximum cycle budget before returning `false`.
    /// - Returns: `true` when a new frame completed.
    @discardableResult
    public func runToNextFrame(maximumCycles: UInt64 = 100_000) throws -> Bool {
        var completed: Int32 = 0
        try Self.check(beeb_run_until_frame(handle, maximumCycles, &completed))
        return completed != 0
    }

    /// Copies CPU registers and cycle count from one safe point.
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

    /// Renders independently owned mono samples without advancing CPU time.
    /// - Parameters:
    ///   - frames: Positive number of samples.
    ///   - sampleRate: Finite positive sample rate in hertz.
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

    /// Changes one keyboard-matrix bit in FIFO order.
    public func setKey(column: UInt8, row: UInt8, pressed: Bool) throws {
        guard column < 16, row < 16 else { throw BeebError.invalidKey }
        try Self.check(beeb_set_key(handle, column, row, pressed ? 1 : 0))
    }

    /// Changes BREAK state without inventing a lifecycle transition.
    public func setBreak(pressed: Bool) throws {
        try Self.check(beeb_set_break(handle, pressed ? 1 : 0))
    }

    /// Returns the current completed-instruction/device-tick identity.
    public func safePoint() throws -> BeebSafePoint {
        var point = beeb_safe_point()
        try Self.check(beeb_get_safe_point(handle, &point))
        return Self.safePoint(point)
    }

    /// Returns retained execution-fault detail, or `nil` outside faulted state.
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
    private static func check(_ status: beeb_status) throws {
        guard status.code != BEEB_STATUS_OK else { return }
        throw BeebError.coreStatus(
            statusCategory(status.code), statusMessage(status))
    }

    /// Maps the closed C category vocabulary into the public Swift vocabulary.
    private static func statusCategory(_ code: beeb_status_code) -> BeebStatusCategory {
        switch code {
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
