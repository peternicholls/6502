import Foundation
import XCTest
@testable import BeebKit

final class BeebMachineTests: XCTestCase {
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
}
