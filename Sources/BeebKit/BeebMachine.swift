import BeebCore
import Foundation

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

/// Errors produced by validating input or crossing the C core boundary.
public enum BeebError: LocalizedError {
    /// The C++ machine could not be allocated or constructed.
    case coreCreationFailed
    /// A C++ exception or C boundary validation error, preserved as text.
    case coreFailure(String)
    /// The operating-system ROM was not exactly 16 KiB.
    case invalidOSROM
    /// The sideways-ROM bank or byte count was outside the supported range.
    case invalidSidewaysROM
    /// The disc bytes did not describe a supported DFS SSD or DSD geometry.
    case invalidDiscImage
    /// The drive number was not zero or one.
    case invalidDrive

    /// A user-facing description of this error.
    public var errorDescription: String? {
        switch self {
        case .coreCreationFailed: return "The emulator core could not be created."
        case let .coreFailure(message): return "The emulator core failed: \(message)"
        case .invalidOSROM: return "A BBC Model B OS ROM must be exactly 16 KiB."
        case .invalidSidewaysROM: return "Use bank 0–15 and a sideways ROM between 1 byte and 16 KiB."
        case .invalidDiscImage: return "The disc is not a valid 40/80-track SSD or DSD image."
        case .invalidDrive: return "The drive number must be 0 or 1."
        }
    }
}

/// Thread-safe Swift owner of one deterministic BBC Model B core.
///
/// The instance owns its C handle until deinitialization. Public operations are
/// serialized by an internal lock, so the class can cross concurrency domains;
/// callbacks are not invoked while that lock is held. ROM and disc inputs and
/// returned frames/audio are copied into independently owned storage.
public final class BeebMachine: @unchecked Sendable {
    private let handle: OpaquePointer
    private let lock = NSLock()

    /// Creates a machine with no ROM or disc loaded.
    /// - Throws: ``BeebError/coreCreationFailed`` if the core cannot be created.
    public init() throws {
        guard let handle = beeb_create() else { throw BeebError.coreCreationFailed }
        self.handle = handle
    }

    deinit { beeb_destroy(handle) }

    /// Validates and copies a BBC Model B operating-system ROM.
    /// - Parameter data: Exactly 16 KiB of ROM bytes.
    /// - Throws: ``BeebError/invalidOSROM`` for the wrong size, or
    ///   ``BeebError/coreFailure(_:)`` for a core diagnostic.
    public func loadOSROM(_ data: Data) throws {
        guard data.count == 16 * 1024 else { throw BeebError.invalidOSROM }
        let (loaded, coreError) = lock.withLock {
            let loaded = data.withUnsafeBytes { bytes in
                beeb_load_os_rom(handle, bytes.bindMemory(to: UInt8.self).baseAddress, bytes.count)
            }
            return (loaded, coreErrorDescription())
        }
        if let coreError { throw BeebError.coreFailure(coreError) }
        guard loaded != 0 else { throw BeebError.invalidOSROM }
    }

    /// Validates and copies a sideways ROM into a bank.
    /// - Parameters:
    ///   - data: Between one byte and 16 KiB; the machine does not retain this value.
    ///   - bank: Bank number in the range 0...15.
    /// - Throws: ``BeebError/invalidSidewaysROM`` for invalid input, or
    ///   ``BeebError/coreFailure(_:)`` for a core diagnostic.
    public func loadSidewaysROM(_ data: Data, bank: UInt8) throws {
        guard bank < 16, !data.isEmpty, data.count <= 16 * 1024 else {
            throw BeebError.invalidSidewaysROM
        }
        let (loaded, coreError) = lock.withLock {
            let loaded = data.withUnsafeBytes { bytes in
                beeb_load_sideways_rom(handle, bank, bytes.bindMemory(to: UInt8.self).baseAddress, bytes.count)
            }
            return (loaded, coreErrorDescription())
        }
        if let coreError { throw BeebError.coreFailure(coreError) }
        guard loaded != 0 else { throw BeebError.invalidSidewaysROM }
    }

    /// Validates, copies, and mounts a DFS disc image.
    /// - Parameters:
    ///   - data: Complete 1...80-track image bytes.
    ///   - drive: Drive zero or one.
    ///   - doubleSided: `true` for interleaved DSD ordering; `false` for SSD.
    ///   - writable: Whether the machine may modify its private image copy.
    /// - Throws: An input-specific ``BeebError`` or
    ///   ``BeebError/coreFailure(_:)`` for a core diagnostic.
    public func mountDisc(_ data: Data, drive: Int = 0, doubleSided: Bool, writable: Bool = false) throws {
        guard (0...1).contains(drive) else { throw BeebError.invalidDrive }
        let bytesPerTrack = 10 * 256 * (doubleSided ? 2 : 1)
        let tracks = data.count / bytesPerTrack
        guard data.count.isMultiple(of: bytesPerTrack), (1...80).contains(tracks) else {
            throw BeebError.invalidDiscImage
        }
        let (loaded, coreError) = lock.withLock {
            let loaded = data.withUnsafeBytes { bytes in
                beeb_mount_disc(handle, UInt32(drive), bytes.bindMemory(to: UInt8.self).baseAddress,
                                bytes.count, doubleSided ? 1 : 0, writable ? 1 : 0)
            }
            return (loaded, coreErrorDescription())
        }
        if let coreError { throw BeebError.coreFailure(coreError) }
        guard loaded != 0 else { throw BeebError.invalidDiscImage }
    }

    /// Resets CPU and device state while retaining loaded media.
    public func reset() {
        lock.withLock { beeb_reset(handle) }
    }

    @discardableResult
    /// Executes whole instructions until at least the cycle budget has elapsed.
    /// - Parameter cycles: Minimum CPU-cycle budget; zero performs no work.
    /// - Returns: Actual cycles, which may exceed the budget by one instruction.
    /// - Throws: ``BeebError/coreFailure(_:)`` if the core rejects execution.
    public func run(cycles: UInt64) throws -> UInt64 {
        let (executed, coreError) = lock.withLock {
            let executed = beeb_run_cycles(handle, cycles)
            return (executed, coreErrorDescription())
        }
        if let coreError { throw BeebError.coreFailure(coreError) }
        return executed
    }

    @discardableResult
    /// Runs until a frame completes or the instruction-cycle limit is reached.
    /// - Parameter maximumCycles: Maximum cycle budget before returning `false`.
    /// - Returns: `true` when a new frame completed.
    /// - Throws: ``BeebError/coreFailure(_:)`` if execution fails.
    public func runToNextFrame(maximumCycles: UInt64 = 100_000) throws -> Bool {
        let (result, coreError) = lock.withLock {
            let result = beeb_run_until_frame(handle, maximumCycles)
            return (result, coreErrorDescription())
        }
        if let coreError { throw BeebError.coreFailure(coreError) }
        return result > 0
    }

    /// A value copy of the current CPU registers and cycle counter.
    public var cpuState: BeebCPUState {
        lock.withLock {
            let state = beeb_get_cpu_state(handle)
            return BeebCPUState(a: state.a, x: state.x, y: state.y, stackPointer: state.sp,
                                status: state.p, programCounter: state.pc, cycles: state.cycles)
        }
    }

    /// Copies the latest machine-owned C frame buffer into Swift-owned storage.
    ///
    /// The copy is completed while the machine lock is held, before the C
    /// buffer can be invalidated by another operation.
    /// - Returns: A complete frame, or `nil` before a frame has been rendered.
    public func videoFrame() -> BeebVideoFrame? {
        lock.withLock {
            var width: UInt32 = 0
            var height: UInt32 = 0
            var number: UInt64 = 0
            guard let pointer = beeb_get_frame_rgba(handle, &width, &height, &number), width > 0, height > 0 else {
                return nil
            }
            let byteCount = Int(width) * Int(height) * 4
            return BeebVideoFrame(width: Int(width), height: Int(height), number: number,
                                  rgba: Data(bytes: pointer, count: byteCount))
        }
    }

    /// Renders mono samples without advancing CPU time.
    /// - Parameters:
    ///   - frames: Positive number of samples to render.
    ///   - sampleRate: Finite, positive sample rate in hertz.
    /// - Returns: Swift-owned samples, or an empty array for invalid arguments.
    public func renderAudio(frames: Int, sampleRate: Double) -> [Float] {
        guard frames > 0, sampleRate.isFinite, sampleRate > 0 else { return [] }
        return lock.withLock {
            var output = Array(repeating: Float.zero, count: frames)
            output.withUnsafeMutableBufferPointer {
                beeb_render_audio(handle, $0.baseAddress, $0.count, sampleRate)
            }
            return output
        }
    }

    /// Changes one key in the emulated keyboard matrix.
    /// - Parameters:
    ///   - column: Matrix column; values outside 0...15 are ignored by the core.
    ///   - row: Matrix row; values outside 0...15 are ignored by the core.
    ///   - pressed: Whether the key is pressed.
    public func setKey(column: UInt8, row: UInt8, pressed: Bool) {
        lock.withLock { beeb_set_key(handle, column, row, pressed ? 1 : 0) }
    }

    /// Changes BREAK state; a released-to-pressed transition resets the machine.
    /// - Parameter pressed: Whether BREAK is held.
    public func setBreak(pressed: Bool) {
        lock.withLock { beeb_set_break(handle, pressed ? 1 : 0) }
    }

    private func coreErrorDescription() -> String? {
        guard let error = beeb_last_error(handle), error.pointee != 0 else { return nil }
        return String(cString: error)
    }
}

private extension NSLock {
    func withLock<T>(_ operation: () throws -> T) rethrows -> T {
        lock()
        defer { unlock() }
        return try operation()
    }
}
