import BeebCore
import Foundation

public struct BeebCPUState: Sendable {
    public let a: UInt8
    public let x: UInt8
    public let y: UInt8
    public let stackPointer: UInt8
    public let status: UInt8
    public let programCounter: UInt16
    public let cycles: UInt64
}

public struct BeebVideoFrame: Sendable {
    public let width: Int
    public let height: Int
    public let number: UInt64
    public let rgba: Data
}

public enum BeebError: LocalizedError {
    case coreCreationFailed
    case invalidOSROM
    case invalidSidewaysROM
    case invalidDiscImage

    public var errorDescription: String? {
        switch self {
        case .coreCreationFailed: return "The emulator core could not be created."
        case .invalidOSROM: return "A BBC Model B OS ROM must be exactly 16 KiB."
        case .invalidSidewaysROM: return "A sideways ROM must be no larger than 16 KiB."
        case .invalidDiscImage: return "The disc is not a valid 40/80-track SSD or DSD image."
        }
    }
}

public final class BeebMachine: @unchecked Sendable {
    private let handle: OpaquePointer
    private let lock = NSLock()

    public init() throws {
        guard let handle = beeb_create() else { throw BeebError.coreCreationFailed }
        self.handle = handle
    }

    deinit { beeb_destroy(handle) }

    public func loadOSROM(_ data: Data) throws {
        let loaded = data.withUnsafeBytes { bytes in
            beeb_load_os_rom(handle, bytes.bindMemory(to: UInt8.self).baseAddress, bytes.count)
        }
        guard loaded != 0 else { throw BeebError.invalidOSROM }
    }

    public func loadSidewaysROM(_ data: Data, bank: UInt8) throws {
        let loaded = data.withUnsafeBytes { bytes in
            beeb_load_sideways_rom(handle, bank, bytes.bindMemory(to: UInt8.self).baseAddress, bytes.count)
        }
        guard loaded != 0 else { throw BeebError.invalidSidewaysROM }
    }

    public func mountDisc(_ data: Data, drive: Int = 0, doubleSided: Bool, writable: Bool = false) throws {
        let loaded = data.withUnsafeBytes { bytes in
            beeb_mount_disc(handle, UInt32(drive), bytes.bindMemory(to: UInt8.self).baseAddress,
                            bytes.count, doubleSided ? 1 : 0, writable ? 1 : 0)
        }
        guard loaded != 0 else { throw BeebError.invalidDiscImage }
    }

    public func reset() {
        lock.withLock { beeb_reset(handle) }
    }

    @discardableResult
    public func run(cycles: UInt64) -> UInt64 {
        lock.withLock { beeb_run_cycles(handle, cycles) }
    }

    @discardableResult
    public func runToNextFrame(maximumCycles: UInt64 = 100_000) -> Bool {
        lock.withLock { beeb_run_until_frame(handle, maximumCycles) != 0 }
    }

    public var cpuState: BeebCPUState {
        lock.withLock {
            let state = beeb_get_cpu_state(handle)
            return BeebCPUState(a: state.a, x: state.x, y: state.y, stackPointer: state.sp,
                                status: state.p, programCounter: state.pc, cycles: state.cycles)
        }
    }

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

    public func renderAudio(frames: Int, sampleRate: Double) -> [Float] {
        lock.withLock {
            var output = Array(repeating: Float.zero, count: frames)
            output.withUnsafeMutableBufferPointer {
                beeb_render_audio(handle, $0.baseAddress, $0.count, sampleRate)
            }
            return output
        }
    }

    public func setKey(column: UInt8, row: UInt8, pressed: Bool) {
        lock.withLock { beeb_set_key(handle, column, row, pressed ? 1 : 0) }
    }

    public func setBreak(pressed: Bool) {
        lock.withLock { beeb_set_break(handle, pressed ? 1 : 0) }
    }
}

private extension NSLock {
    func withLock<T>(_ operation: () throws -> T) rethrows -> T {
        lock()
        defer { unlock() }
        return try operation()
    }
}
