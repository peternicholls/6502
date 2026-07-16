import Foundation
import XCTest
import BeebCore
@testable import BeebKit

/// Validates the Swift wrapper's typed errors, lifecycle/concurrency contract,
/// and owned CPU/video observations against the C runtime boundary.
final class BeebMachineTests: XCTestCase {
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

    func testPublicVersionMatchesReleaseVersion() {
        XCTAssertEqual(BeebVersion.current, "0.2.0")
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
}
