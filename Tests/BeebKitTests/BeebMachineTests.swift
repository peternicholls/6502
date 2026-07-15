import Foundation
import XCTest
@testable import BeebKit

final class BeebMachineTests: XCTestCase {
    private func validOSROM() -> Data {
        var bytes = [UInt8](repeating: 0xEA, count: 16 * 1024)
        bytes[0x3FFC] = 0x00
        bytes[0x3FFD] = 0xC0
        return Data(bytes)
    }

    func testPublicVersionMatchesReleaseVersion() {
        XCTAssertEqual(BeebVersion.current, "0.1.0")
    }

    func testUnsupportedOpcodeIsReportedAsSwiftError() throws {
        let machine = try BeebMachine()

        XCTAssertThrowsError(try machine.run(cycles: 1)) { error in
            guard case let BeebError.coreFailure(message) = error else {
                return XCTFail("Expected a core failure, got \(error)")
            }
            XCTAssertTrue(message.contains("unsupported NMOS 6502 opcode"))
        }
    }

    func testInvalidDriveIsRejectedWithoutIntegerTrap() throws {
        let machine = try BeebMachine()
        let disc = Data(repeating: 0, count: 40 * 10 * 256)

        XCTAssertThrowsError(
            try machine.mountDisc(disc, drive: -1, doubleSided: false)
        ) { error in
            guard case BeebError.invalidDrive = error else {
                return XCTFail("Expected invalidDrive, got \(error)")
            }
        }
    }

    func testInvalidAudioRequestsReturnNoSamples() throws {
        let machine = try BeebMachine()

        XCTAssertEqual(machine.renderAudio(frames: -1, sampleRate: 48_000), [])
        XCTAssertEqual(machine.renderAudio(frames: 16, sampleRate: .nan), [])
    }

    func testInvalidROMAndDiscInputsMapToSpecificSwiftErrors() throws {
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
            try machine.mountDisc(Data(repeating: 0, count: 40 * 10 * 256),
                                  drive: 2, doubleSided: false)
        ) { error in
            guard case BeebError.invalidDrive = error else {
                return XCTFail("Expected invalidDrive, got \(error)")
            }
        }
    }

    func testCoreErrorClearsAfterAValidRecoveryOperation() throws {
        let machine = try BeebMachine()

        XCTAssertThrowsError(try machine.runToNextFrame(maximumCycles: 1)) { error in
            guard case let BeebError.coreFailure(message) = error else {
                return XCTFail("Expected coreFailure, got \(error)")
            }
            XCTAssertTrue(message.contains("unsupported NMOS 6502 opcode"))
        }

        try machine.loadOSROM(validOSROM())
        machine.reset()
        let executed = try machine.run(cycles: 1)
        XCTAssertGreaterThanOrEqual(executed, 1)
        XCTAssertEqual(machine.cpuState.programCounter, 0xC001)
        XCTAssertNil(machine.videoFrame())
    }

    func testValidAudioAndInputCallsRemainRecoverable() throws {
        let machine = try BeebMachine()
        try machine.loadOSROM(validOSROM())
        machine.reset()

        XCTAssertEqual(machine.renderAudio(frames: 8, sampleRate: 48_000).count, 8)
        machine.setKey(column: 255, row: 255, pressed: true)
        machine.setBreak(pressed: true)
        machine.setBreak(pressed: false)
        XCTAssertNoThrow(try machine.run(cycles: 1))
    }
}
