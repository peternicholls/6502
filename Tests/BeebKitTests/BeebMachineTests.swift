import Foundation
import XCTest
import BeebCore
@testable import BeebKit

// C0-DOC-RATIONALE: docs/code/host-boundary.md owns cross-language recovery evidence.

/// Validates the Swift wrapper's typed errors, lifecycle/concurrency contract,
/// and owned CPU/video observations against the C runtime boundary.
final class BeebMachineTests: XCTestCase {
    /// Cross-boundary values retained after C storage is released, used to prove
    /// complete Swift replay determinism rather than only status-category mapping.
    private struct OutputReplay: Equatable {
        let frameNumber: UInt64
        let rgba: Data
        let audio: BeebAudioDrain
        let diagnostics: BeebOutputDiagnostics
    }

    private func validOSROM() -> Data {
        var bytes = [UInt8](repeating: 0xEA, count: 16 * 1024)
        bytes[0x3FFC] = 0x00
        bytes[0x3FFD] = 0xC0
        return Data(bytes)
    }

    private func loopingOSROM() -> Data {
        var bytes = [UInt8](validOSROM())
        bytes[0] = 0x4C
        bytes[1] = 0x00
        bytes[2] = 0xC0
        return Data(bytes)
    }

    private func outputOSROM() -> Data {
        var bytes = [UInt8](validOSROM())
        var cursor = 0
        func emit(_ byte: UInt8) { bytes[cursor] = byte; cursor += 1 }
        func load(_ value: UInt8) { emit(0xA9); emit(value) }
        func store(_ address: UInt16) {
            emit(0x8D)
            emit(UInt8(truncatingIfNeeded: address))
            emit(UInt8(truncatingIfNeeded: address >> 8))
        }
        func setCRTC(_ register: UInt8, _ value: UInt8) {
            load(register)
            store(0xFE00)
            load(value)
            store(0xFE01)
        }
        setCRTC(1, 1)
        setCRTC(6, 1)
        setCRTC(9, 0)
        setCRTC(12, 0)
        setCRTC(13, 0)
        load(0x1C)
        store(0xFE20)
        let idle = UInt16(0xC000 + cursor)
        emit(0x4C)
        emit(UInt8(truncatingIfNeeded: idle))
        emit(UInt8(truncatingIfNeeded: idle >> 8))
        return Data(bytes)
    }

    private func assertCoreStatus(
        _ expected: BeebStatusCategory,
        file: StaticString = #filePath,
        line: UInt = #line,
        _ operation: () throws -> Void
    ) {
        XCTAssertThrowsError(try operation(), file: file, line: line) { error in
            guard case let BeebError.coreStatus(category, message) = error else {
                return XCTFail("Expected coreStatus, got \(error)", file: file, line: line)
            }
            XCTAssertEqual(category, expected, file: file, line: line)
            XCTAssertFalse(message.isEmpty, file: file, line: line)
        }
    }

    /// Runs the same bounded producer/consumer sequence on one fresh runtime and
    /// returns only independently owned Swift values for exact replay comparison.
    private func captureOutputReplay() throws -> OutputReplay {
        let machine = try BeebMachine()
        try machine.loadOSROM(outputOSROM())
        try machine.reset()
        for _ in 0..<6 {
            XCTAssertTrue(try machine.runToNextFrame(maximumCycles: 200_000))
        }
        _ = try machine.run(cycles: 2_000_000)
        let frame = try machine.dequeueVideoFrame()
        let audio = try machine.drainAudio(maximumSamples: 4_096)
        return OutputReplay(
            frameNumber: frame.number,
            rgba: frame.rgba,
            audio: audio,
            diagnostics: try machine.outputDiagnostics()
        )
    }

    func testPublicVersionMatchesReleaseVersion() {
        XCTAssertEqual(BeebVersion.current, "0.3.0")
    }

    func testMachineProfileValuesOwnRawIdentityAndNames() {
        func requireSendable<T: Sendable>(_: T) {}

        let raw = BeebMachineProfileComponent(
            identifier: 0xF000_0001,
            version: 23,
            reserved: 0
        )
        var source = [raw]
        let rawProfile = BeebMachineProfile(
            schemaVersion: 1,
            base: raw,
            expansions: source
        )
        source[0] = BeebMachineProfileComponent(identifier: 7, version: 1)

        requireSendable(raw)
        requireSendable(rawProfile)
        XCTAssertEqual(rawProfile.base.identifier, 0xF000_0001)
        XCTAssertEqual(rawProfile.base.version, 23)
        XCTAssertEqual(rawProfile.base.reserved, 0)
        XCTAssertEqual(rawProfile.expansions, [raw])
        XCTAssertNotEqual(rawProfile, BeebMachineProfile.modelB)

        XCTAssertEqual(BeebMachineProfile.schemaVersion, 1)
        XCTAssertEqual(BeebMachineProfile.expansionCapacity, 16)
        XCTAssertEqual(BeebMachineProfile.modelB.base.identifier, 0x0000_0001)
        XCTAssertEqual(BeebMachineProfile.modelB.base.version, 1)
        XCTAssertEqual(BeebMachineProfile.modelB.expansions, [])
        XCTAssertEqual(
            BeebMachineProfile.modelB.displayName,
            "BBC Microcomputer Model B"
        )
        XCTAssertEqual(BeebMachineProfile.modelBPlus64K.base.identifier, 0x0000_0002)
        XCTAssertEqual(BeebMachineProfile.modelBPlus64K.base.version, 1)
        XCTAssertEqual(BeebMachineProfile.modelBPlus64K.expansions, [])
        XCTAssertEqual(
            BeebMachineProfile.modelBPlus64K.displayName,
            "BBC Model B+ 64K"
        )
        XCTAssertNotEqual(BeebMachineProfile.modelB, BeebMachineProfile.modelBPlus64K)
    }

    func testLifecycleStateStartPauseAndIdempotence() throws {
        let machine = try BeebMachine()
        try machine.loadOSROM(loopingOSROM())
        try machine.reset()
        XCTAssertEqual(try machine.state, .paused)

        try machine.start()
        try machine.start()
        XCTAssertEqual(try machine.state, .running)
        try machine.pause()
        try machine.pause()
        XCTAssertEqual(try machine.state, .paused)
    }

    func testStatusCategoryIsPreservedWithDiagnostic() throws {
        let machine = try BeebMachine()
        try machine.loadOSROM(loopingOSROM())
        try machine.reset()
        try machine.start()
        assertCoreStatus(.invalidState) {
            _ = try machine.run(cycles: 1)
        }
        try machine.pause()

        var illegal = [UInt8](validOSROM())
        illegal[0] = 0x02
        try machine.loadOSROM(Data(illegal))
        try machine.reset()
        assertCoreStatus(.executionFailed) {
            _ = try machine.run(cycles: 1)
        }
    }

    func testEveryCoreStatusCategoryMapsDirectly() {
        let cases: [(beeb_status_code, BeebStatusCategory)] = [
            (BEEB_STATUS_OK, .ok),
            (BEEB_STATUS_EMPTY, .empty),
            (BEEB_STATUS_UNDERRUN, .underrun),
            (BEEB_STATUS_OVERRUN, .overrun),
            (BEEB_STATUS_CAPACITY_EXCEEDED, .capacityExceeded),
            (BEEB_STATUS_OUTPUT_FAILED, .outputProductionFailed),
            (BEEB_STATUS_INVALID_ARGUMENT, .invalidArgument),
            (BEEB_STATUS_INVALID_STATE, .invalidState),
            (BEEB_STATUS_EXECUTION_FAILED, .executionFailed),
            (BEEB_STATUS_RESOURCE_EXHAUSTED, .resourceExhausted),
            (BEEB_STATUS_UNAVAILABLE, .unavailable),
            (BEEB_STATUS_REENTRANT_CALL, .reentrantCall),
            (BEEB_STATUS_INTERNAL_FAILURE, .internalFailure),
        ]
        for (code, expected) in cases {
            XCTAssertEqual(BeebMachine.statusCategory(code), expected)
        }
    }

    func testResourceAndShutdownStatusesUseProductionThrowMapping() {
        for (code, expected) in [
            (BEEB_STATUS_RESOURCE_EXHAUSTED, BeebStatusCategory.resourceExhausted),
            (BEEB_STATUS_UNAVAILABLE, BeebStatusCategory.unavailable),
        ] {
            var status = beeb_status()
            status.code = code
            XCTAssertThrowsError(try BeebMachine.check(status)) { error in
                guard case let BeebError.coreStatus(category, _) = error else {
                    return XCTFail("Expected coreStatus, got \(error)")
                }
                XCTAssertEqual(category, expected)
            }
        }
    }

    func testInvalidROMDiscAudioAndInputMapToSpecificSwiftErrors() throws {
        let machine = try BeebMachine()

        XCTAssertThrowsError(try machine.loadOSROM(Data())) { error in
            guard case BeebError.invalidOSROM = error else {
                return XCTFail("Expected invalidOSROM, got \(error)")
            }
        }
        XCTAssertThrowsError(try machine.loadSidewaysROM(Data(), bank: 0)) { error in
            guard case BeebError.invalidSidewaysROM = error else {
                return XCTFail("Expected invalidSidewaysROM, got \(error)")
            }
        }
        XCTAssertThrowsError(try machine.loadSidewaysROM(Data([0]), bank: 16)) { error in
            guard case BeebError.invalidSidewaysROM = error else {
                return XCTFail("Expected invalidSidewaysROM, got \(error)")
            }
        }
        XCTAssertThrowsError(try machine.mountDisc(Data([0]), doubleSided: false)) { error in
            guard case BeebError.invalidDiscImage = error else {
                return XCTFail("Expected invalidDiscImage, got \(error)")
            }
        }
        XCTAssertThrowsError(
            try machine.mountDisc(
                Data(repeating: 0, count: 40 * 10 * 256),
                drive: 2,
                doubleSided: false
            )
        ) { error in
            guard case BeebError.invalidDrive = error else {
                return XCTFail("Expected invalidDrive, got \(error)")
            }
        }
        XCTAssertThrowsError(try machine.renderAudio(frames: -1, sampleRate: 48_000)) {
            guard case BeebError.invalidAudioRequest = $0 else {
                return XCTFail("Expected invalidAudioRequest, got \($0)")
            }
        }
        XCTAssertThrowsError(try machine.renderAudio(frames: 16, sampleRate: .nan)) {
            guard case BeebError.invalidAudioRequest = $0 else {
                return XCTFail("Expected invalidAudioRequest, got \($0)")
            }
        }
        XCTAssertThrowsError(try machine.setKey(column: 16, row: 0, pressed: true)) {
            guard case BeebError.invalidKey = $0 else {
                return XCTFail("Expected invalidKey, got \($0)")
            }
        }
    }

    func testFaultDetailAndResetRecoveryRemainTyped() throws {
        let machine = try BeebMachine()
        var illegal = [UInt8](validOSROM())
        illegal[0] = 0x02
        try machine.loadOSROM(Data(illegal))
        try machine.reset()
        assertCoreStatus(.executionFailed) {
            _ = try machine.run(cycles: 1)
        }
        XCTAssertEqual(try machine.state, .faulted)
        let fault = try XCTUnwrap(machine.fault())
        XCTAssertTrue(fault.message.contains("unsupported NMOS 6502 opcode"))
        XCTAssertEqual(fault.safePoint.state, .faulted)
        assertCoreStatus(.invalidState) { try machine.start() }

        try machine.reset()
        XCTAssertEqual(try machine.state, .paused)
        XCTAssertNil(try machine.fault())
        try machine.loadOSROM(validOSROM())
        try machine.reset()
        let executed = try machine.run(cycles: 1)
        XCTAssertGreaterThanOrEqual(executed, 1)
        XCTAssertEqual(try machine.cpuState().programCounter, 0xC001)
        XCTAssertNil(try machine.videoFrame())
    }

    func testConcurrentLifecycleMutationAndObservation() async throws {
        let machine = try BeebMachine()
        try machine.loadOSROM(loopingOSROM())
        try machine.reset()

        try await withThrowingTaskGroup(of: Void.self) { group in
            for index in 0..<100 {
                group.addTask {
                    switch index % 5 {
                    case 0: try machine.start()
                    case 1: try machine.pause()
                    case 2: _ = try machine.state
                    case 3:
                        try machine.setKey(
                            column: UInt8(index % 16),
                            row: UInt8((index / 16) % 16),
                            pressed: index.isMultiple(of: 2)
                        )
                    default: _ = try machine.cpuState()
                    }
                }
            }
            try await group.waitForAll()
        }

        try machine.pause()
        XCTAssertEqual(try machine.state, .paused)
        XCTAssertGreaterThan(try machine.safePoint().cpuCycles, 0)
    }

    func testConcurrentFaultQueryResetRecoveryAndFinalRelease() async throws {
        weak var releasedMachine: BeebMachine?
        do {
            let machine = try BeebMachine()
            releasedMachine = machine
            var illegal = [UInt8](validOSROM())
            illegal[0] = 0x02
            try machine.loadOSROM(Data(illegal))
            try machine.reset()
            assertCoreStatus(.executionFailed) { _ = try machine.run(cycles: 1) }

            try await withThrowingTaskGroup(of: Void.self) { group in
                for index in 0..<64 {
                    group.addTask {
                        switch index % 4 {
                        case 0: _ = try machine.state
                        case 1: _ = try machine.cpuState()
                        case 2: _ = try machine.fault()
                        default: try machine.reset()
                        }
                    }
                }
                try await group.waitForAll()
            }
            try machine.reset()
            XCTAssertEqual(try machine.state, .paused)
            XCTAssertNil(try machine.fault())
        }
        XCTAssertNil(releasedMachine)
    }

    func testValidAudioAndBreakCallsRemainRecoverable() throws {
        let machine = try BeebMachine()
        try machine.loadOSROM(validOSROM())
        try machine.reset()

        XCTAssertEqual(try machine.renderAudio(frames: 8, sampleRate: 48_000).count, 8)
        try machine.setKey(column: 1, row: 2, pressed: true)
        try machine.setBreak(pressed: true)
        try machine.setBreak(pressed: false)
        XCTAssertNoThrow(try machine.run(cycles: 1))
    }

    func testC2DequeuedFrameIsIndependentlyOwned() throws {
        let machine = try BeebMachine()
        try machine.loadOSROM(outputOSROM())
        try machine.reset()
        XCTAssertTrue(try machine.runToNextFrame(maximumCycles: 200_000))

        let retained = try machine.dequeueVideoFrame()
        let retainedPixels = retained.rgba
        let retainedNumber = retained.number
        for _ in 0..<5 {
            XCTAssertTrue(try machine.runToNextFrame(maximumCycles: 200_000))
        }

        XCTAssertEqual(retained.number, retainedNumber)
        XCTAssertEqual(retained.rgba, retainedPixels)
        XCTAssertGreaterThan(try machine.dequeueVideoFrame().number, retainedNumber)
    }

    func testC2FrameEmptyAndFaultLifecycleAreTyped() throws {
        let machine = try BeebMachine()
        assertCoreStatus(.empty) {
            _ = try machine.dequeueVideoFrame()
        }

        var illegal = [UInt8](validOSROM())
        illegal[0] = 0x02
        try machine.loadOSROM(Data(illegal))
        try machine.reset()
        assertCoreStatus(.executionFailed) {
            _ = try machine.run(cycles: 1)
        }
        assertCoreStatus(.invalidState) {
            _ = try machine.dequeueVideoFrame()
        }
    }

    func testC2AudioDrainOwnsSamplesAndCarriesTypedPressure() throws {
        let machine = try BeebMachine()
        try machine.loadOSROM(loopingOSROM())
        try machine.reset()
        _ = try machine.run(cycles: 2_000_000)

        var retained: BeebAudioDrain?
        XCTAssertThrowsError(try machine.drainAudio(maximumSamples: 5_000)) { error in
            guard case let BeebError.audioPressure(category, drain) = error else {
                return XCTFail("Expected audioPressure, got \(error)")
            }
            XCTAssertEqual(category, .underrun)
            XCTAssertEqual(drain.samples.count, 4_096)
            XCTAssertEqual(drain.shortfall, 904)
            XCTAssertEqual(drain.demand, 2_048)
            XCTAssertGreaterThan(drain.overrunCount, 0)
            XCTAssertGreaterThanOrEqual(drain.underrunCount, UInt64(drain.shortfall))
            retained = drain
        }

        let retainedSamples = try XCTUnwrap(retained).samples
        _ = try machine.run(cycles: 100_000)
        XCTAssertEqual(retained?.samples, retainedSamples)

        _ = try machine.drainAudio(maximumSamples: 2_048)
        XCTAssertThrowsError(try machine.drainAudio(maximumSamples: 4_096)) { error in
            guard case let BeebError.audioPressure(category, drain) = error else {
                return XCTFail("Expected recoverable underrun, got \(error)")
            }
            XCTAssertEqual(category, .underrun)
            XCTAssertEqual(drain.samples.count + drain.shortfall, 4_096)
            XCTAssertGreaterThanOrEqual(drain.demand, 0)
        }
    }

    func testC2DiagnosticMappingRecoveryAndRateTolerance() throws {
        let machine = try BeebMachine()
        try machine.loadOSROM(outputOSROM())
        try machine.reset()
        XCTAssertTrue(try machine.runToNextFrame(maximumCycles: 200_000))
        _ = try machine.run(cycles: 2_000_000)

        XCTAssertThrowsError(try machine.drainAudio(maximumSamples: 5_000)) { error in
            guard case let BeebError.audioPressure(category, drain) = error else {
                return XCTFail("Expected recoverable pressure, got \(error)")
            }
            XCTAssertEqual(category, .underrun)
            XCTAssertEqual(drain.samples.count + drain.shortfall, 5_000)
        }

        let pressured = try machine.outputDiagnostics()
        XCTAssertEqual(pressured.totalCycles, try machine.cpuState().cycles)
        XCTAssertGreaterThanOrEqual(pressured.latestFrameNumber, 1)
        XCTAssertLessThanOrEqual(pressured.frameDepth, pressured.frameCapacity)
        XCTAssertEqual(pressured.frameCapacity, 3)
        XCTAssertEqual(pressured.audioDepth, 0)
        XCTAssertEqual(pressured.audioCapacity, 4_096)
        XCTAssertEqual(pressured.audioDemand, 2_048)
        XCTAssertEqual(pressured.lastStatus, .underrun)
        XCTAssertEqual(
            pressured.counters.framesProduced,
            pressured.counters.framesConsumed + pressured.counters.framesDropped
                + UInt64(pressured.frameDepth)
        )
        XCTAssertEqual(
            pressured.counters.audioSamplesProduced,
            pressured.counters.audioSamplesConsumed + pressured.counters.audioSamplesOverrun
                + UInt64(pressured.audioDepth)
        )
        XCTAssertGreaterThan(pressured.counters.audioSamplesOverrun, 0)
        XCTAssertGreaterThan(pressured.counters.audioSamplesUnderrun, 0)
        XCTAssertEqual(try machine.outputDiagnostics(), pressured)

        let before = pressured
        _ = try machine.run(cycles: 4_000_000)
        let after = try machine.outputDiagnostics()
        let delta = after.totalCycles - before.totalCycles
        let hostSeconds = Double(delta) / 3_000_000.0
        XCTAssertEqual(
            try BeebMachine.emulationRate(from: before, to: after, hostSeconds: hostSeconds),
            1.5,
            accuracy: 0.0015
        )

        for invalid in [0.0, -1.0, Double.nan, Double.infinity] {
            assertCoreStatus(.invalidArgument) {
                _ = try BeebMachine.emulationRate(
                    from: before, to: after, hostSeconds: invalid)
            }
        }
        assertCoreStatus(.invalidArgument) {
            _ = try BeebMachine.emulationRate(from: after, to: before, hostSeconds: 1.0)
        }
    }

    func testC2OutputReplayIsExactAcrossFreshSwiftRuntimes() throws {
        let first = try captureOutputReplay()
        let second = try captureOutputReplay()
        XCTAssertGreaterThan(first.frameNumber, 0)
        XCTAssertFalse(first.rgba.isEmpty)
        XCTAssertEqual(first, second)
    }

    func testC2ResetDoesNotExposePreResetOutput() throws {
        let machine = try BeebMachine()
        try machine.loadOSROM(outputOSROM())
        try machine.reset()
        XCTAssertTrue(try machine.runToNextFrame(maximumCycles: 200_000))
        _ = try machine.run(cycles: 2_000_000)

        let before = try machine.outputDiagnostics()
        XCTAssertGreaterThan(before.frameDepth, 0)
        XCTAssertGreaterThan(before.audioDepth, 0)
        try machine.reset()

        let after = try machine.outputDiagnostics()
        XCTAssertEqual(after.frameDepth, 0)
        XCTAssertEqual(after.audioDepth, 0)
        XCTAssertEqual(after.audioDemand, 2_048)
        XCTAssertEqual(after.latestFrameNumber, before.latestFrameNumber)
        XCTAssertEqual(
            after.counters.framesDropped,
            before.counters.framesDropped + UInt64(before.frameDepth)
        )
        XCTAssertEqual(
            after.counters.audioSamplesOverrun,
            before.counters.audioSamplesOverrun + UInt64(before.audioDepth)
        )
        XCTAssertEqual(after.lastStatus, .ok)
        assertCoreStatus(.empty) { _ = try machine.dequeueVideoFrame() }
        XCTAssertThrowsError(try machine.drainAudio(maximumSamples: 1)) { error in
            guard case let BeebError.audioPressure(.underrun, drain) = error else {
                return XCTFail("Expected empty post-reset audio underrun, got \(error)")
            }
            XCTAssertTrue(drain.samples.isEmpty)
            XCTAssertEqual(drain.shortfall, 1)
        }
    }

    func testC2ConcurrentSwiftOutputProductionAndConsumption() async throws {
        let machine = try BeebMachine()
        try machine.loadOSROM(outputOSROM())
        try machine.reset()

        try await withThrowingTaskGroup(of: Void.self) { group in
            group.addTask {
                for _ in 0..<200 { _ = try machine.run(cycles: 2_048) }
            }
            for _ in 0..<4 {
                group.addTask {
                    for _ in 0..<200 {
                        _ = try machine.outputDiagnostics()
                        do {
                            _ = try machine.drainAudio(maximumSamples: 64)
                        } catch BeebError.audioPressure(.underrun, _) {
                            // Partial owned samples are the documented recoverable result.
                        }
                        do {
                            _ = try machine.dequeueVideoFrame()
                        } catch BeebError.coreStatus(.empty, _) {
                            // Polling before the next complete frame is ordinary pressure.
                        }
                    }
                }
            }
            try await group.waitForAll()
        }

        let diagnostics = try machine.outputDiagnostics()
        XCTAssertEqual(
            diagnostics.counters.framesProduced,
            diagnostics.counters.framesConsumed + diagnostics.counters.framesDropped
                + UInt64(diagnostics.frameDepth)
        )
        XCTAssertEqual(
            diagnostics.counters.audioSamplesProduced,
            diagnostics.counters.audioSamplesConsumed
                + diagnostics.counters.audioSamplesOverrun + UInt64(diagnostics.audioDepth)
        )
    }
}
