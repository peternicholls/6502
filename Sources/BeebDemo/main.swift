import BeebKit

// C0-DOC-RATIONALE: Sources/BeebKit/Documentation.docc/BeebKit.md owns host usage.
import SwiftUI
import UniformTypeIdentifiers

#if os(macOS)
import AppKit
/// Native image type selected for the macOS build.
typealias PlatformImage = NSImage
#else
import UIKit
/// Native image type selected for the UIKit build.
typealias PlatformImage = UIImage
#endif

/// Main-actor UI model owning the optional machine and 50 Hz display timer.
/// It translates imported user files and runtime errors into view state.
@MainActor
final class EmulatorModel: ObservableObject {
    /// Native-picker choice kept separate from the active runtime profile.
    enum MachineProfileChoice: String, CaseIterable, Identifiable {
        case modelB
        case modelBPlus64K

        var id: Self { self }
        var profile: BeebMachineProfile {
            switch self {
            case .modelB: return .modelB
            case .modelBPlus64K: return .modelBPlus64K
            }
        }
        var displayName: String { profile.displayName }
    }

    @Published var screen: PlatformImage?
    @Published var status = "Choose a user-supplied BBC Model B OS ROM"
    @Published var isRunning = false
    @Published var isImportingOS = false
    @Published var isImportingDisc = false
    @Published var requestedProfile = MachineProfileChoice.modelB
    @Published private(set) var activeProfile: BeebMachineProfile?
    @Published private(set) var profileStatus = "No machine profile is active"

    /// Replaced only after a requested candidate constructs and reports its profile.
    private var machine: BeebMachine?
    private var timer: Timer?

    init() {
        installRequestedProfile()
    }

    /// Builds a candidate before atomically installing its runtime and active identity.
    func installRequestedProfile() {
        do {
            let candidate = try BeebMachine(profile: requestedProfile.profile)
            let candidateProfile = try candidate.profile
            machine = candidate
            activeProfile = candidateProfile
            profileStatus = "Active profile: \(candidateProfile.displayName)"
        } catch let error as BeebError {
            if case let .machineProfileUnavailable(profile) = error {
                let activeName = activeProfile?.displayName ?? "None"
                profileStatus = "\(profile.displayName) is recognised, but machine support is " +
                    "not yet available. Active profile remains: \(activeName)"
            } else {
                profileStatus = error.localizedDescription
                if machine == nil { status = "The emulator core could not start" }
            }
        } catch {
            profileStatus = error.localizedDescription
            if machine == nil { status = "The emulator core could not start" }
        }
    }

    func loadOS(_ url: URL) {
        guard let machine else { return }
        do {
            let access = url.startAccessingSecurityScopedResource()
            defer { if access { url.stopAccessingSecurityScopedResource() } }
            try machine.loadOSROM(Data(contentsOf: url))
            try machine.reset()
            status = "OS loaded — running"
            start()
        } catch { status = error.localizedDescription }
    }

    func loadDisc(_ url: URL) {
        guard let machine else { return }
        do {
            let access = url.startAccessingSecurityScopedResource()
            defer { if access { url.stopAccessingSecurityScopedResource() } }
            try machine.mountDisc(Data(contentsOf: url), doubleSided: url.pathExtension.lowercased() == "dsd")
            status = "Disc mounted in drive 0"
        } catch { status = error.localizedDescription }
    }

    func start() {
        guard timer == nil else { return }
        isRunning = true
        timer = Timer.scheduledTimer(withTimeInterval: 1.0 / 50.0, repeats: true) { [weak self] _ in
            Task { @MainActor in self?.stepFrame() }
        }
    }

    func stop() {
        timer?.invalidate()
        timer = nil
        isRunning = false
    }

    func reset() {
        guard let machine else { return }
        do { try machine.reset() }
        catch { status = error.localizedDescription; stop() }
    }

    private func stepFrame() {
        guard let machine else { return }
        do {
            _ = try machine.runToNextFrame()
            if let frame = try machine.videoFrame() { screen = platformImage(frame) }
            let cpu = try machine.cpuState()
            status = String(format: "PC %04X   %,llu cycles", cpu.programCounter, cpu.cycles)
        } catch { status = error.localizedDescription; stop() }
    }

    private func platformImage(_ frame: BeebVideoFrame) -> PlatformImage? {
        guard let provider = CGDataProvider(data: frame.rgba as CFData),
              let image = CGImage(width: frame.width, height: frame.height,
                                  bitsPerComponent: 8, bitsPerPixel: 32,
                                  bytesPerRow: frame.width * 4,
                                  space: CGColorSpaceCreateDeviceRGB(),
                                  bitmapInfo: CGBitmapInfo(rawValue: CGImageAlphaInfo.last.rawValue),
                                  provider: provider, decode: nil,
                                  shouldInterpolate: false, intent: .defaultIntent) else { return nil }
        #if os(macOS)
        return NSImage(cgImage: image, size: NSSize(width: frame.width, height: frame.height))
        #else
        return UIImage(cgImage: image)
        #endif
    }
}

/// Root SwiftUI view that binds user actions to `EmulatorModel` and renders
/// the latest owned frame plus import controls.
struct ContentView: View {
    @StateObject private var model = EmulatorModel()

    var body: some View {
        VStack(spacing: 12) {
            Picker("Machine profile", selection: $model.requestedProfile) {
                ForEach(EmulatorModel.MachineProfileChoice.allCases) { choice in
                    Text(choice.displayName).tag(choice)
                }
            }
            .onChange(of: model.requestedProfile) { _ in model.installRequestedProfile() }
            .accessibilityLabel("Machine profile")
            .accessibilityValue(model.requestedProfile.displayName)
            .accessibilityIdentifier("machine-profile-picker")

            Text("Requested profile: \(model.requestedProfile.displayName)")
                .frame(maxWidth: .infinity, alignment: .leading)
                .accessibilityLabel("Requested machine profile")
                .accessibilityValue(model.requestedProfile.displayName)
                .accessibilityIdentifier("requested-machine-profile")

            Text("Active profile: \(model.activeProfile?.displayName ?? "None")")
                .frame(maxWidth: .infinity, alignment: .leading)
                .accessibilityLabel("Active machine profile")
                .accessibilityValue(model.activeProfile?.displayName ?? "None")
                .accessibilityIdentifier("active-machine-profile")

            Text(model.profileStatus)
                .frame(maxWidth: .infinity, alignment: .leading)
                .accessibilityLabel("Machine profile status")
                .accessibilityValue(model.profileStatus)
                .accessibilityIdentifier("machine-profile-status")

            Group {
                if let screen = model.screen {
                    #if os(macOS)
                    Image(nsImage: screen).interpolation(.none).resizable().scaledToFit()
                    #else
                    Image(uiImage: screen).interpolation(.none).resizable().scaledToFit()
                    #endif
                } else {
                    Rectangle().fill(.black).overlay(Text("BBC MICRO").foregroundStyle(.white).monospaced())
                }
            }
            .aspectRatio(4.0 / 3.0, contentMode: .fit)

            HStack {
                Button("Open OS ROM…") { model.isImportingOS = true }
                Button("Mount Disc…") { model.isImportingDisc = true }
                Button(model.isRunning ? "Pause" : "Run") { model.isRunning ? model.stop() : model.start() }
                Button("Break") { model.reset() }
            }
            Text(model.status).font(.caption.monospaced()).frame(maxWidth: .infinity, alignment: .leading)
        }
        .padding()
        .fileImporter(isPresented: $model.isImportingOS, allowedContentTypes: [.data]) { result in
            if case let .success(url) = result { model.loadOS(url) }
        }
        .fileImporter(isPresented: $model.isImportingDisc, allowedContentTypes: [.data]) { result in
            if case let .success(url) = result { model.loadDisc(url) }
        }
    }
}

@main
/// Application root owning the single-window scene for the demo.
struct BeebDemoApp: App {
    var body: some Scene {
        #if os(macOS)
        WindowGroup { ContentView() }.defaultSize(width: 900, height: 700)
        #else
        WindowGroup { ContentView() }
        #endif
    }
}
