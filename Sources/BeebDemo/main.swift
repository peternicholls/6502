import BeebKit

// C0-DOC-RATIONALE: Sources/BeebKit/Documentation.docc/BeebKit.md owns host usage.
import Foundation
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
    @Published var isImportingLanguage = false
    @Published var isImportingDisc = false
    @Published var requestedProfile = MachineProfileChoice.modelB
    @Published private(set) var activeProfile: BeebMachineProfile?
    @Published private(set) var profileStatus = "No machine profile is active"
    @Published private(set) var osAssignment = "OS ROM: not assigned"
    @Published private(set) var languageAssignment = "Language ROM: not assigned"

    /// Replaced only after a requested candidate constructs and reports its profile.
    private var machine: BeebMachine?
    private var timer: Timer?
    private let defaults = UserDefaults.standard
    private var hasOSROM = false
    private var hasLanguageROM = false

    private enum BookmarkKey {
        static let os = "model-b.os-bookmark"
        static let language = "model-b.language-bookmark"
        static let osName = "model-b.os-name"
        static let languageName = "model-b.language-name"
    }

    init() {
        installRequestedProfile()
        restoreRememberedFirmware()
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

    func loadFirmware(_ url: URL, role: BeebFirmwareRole) {
        guard let machine else { return }
        do {
            let access = url.startAccessingSecurityScopedResource()
            defer { if access { url.stopAccessingSecurityScopedResource() } }
            let data = try Data(contentsOf: url)
            let bookmark = try? url.bookmarkData(
                options: [.withSecurityScope, .securityScopeAllowOnlyReadAccess],
                includingResourceValuesForKeys: nil,
                relativeTo: nil
            )
            try machine.loadFirmware(data, role: role)
            remember(bookmark, url: url, role: role)
            updateAssignment(url.lastPathComponent, role: role)
            try resetIfFirmwareReady()
        } catch {
            updateAssignment("recovery needed — \(error.localizedDescription)", role: role)
            status = error.localizedDescription
        }
    }

    func loadOS(_ url: URL) { loadFirmware(url, role: .operatingSystem) }

    func loadDisc(_ url: URL) {
        guard let machine else { return }
        do {
            let access = url.startAccessingSecurityScopedResource()
            defer { if access { url.stopAccessingSecurityScopedResource() } }
            try machine.mountDisc(Data(contentsOf: url), doubleSided: url.pathExtension.lowercased() == "dsd")
            status = "Disc mounted in drive 0"
        } catch { status = error.localizedDescription }
    }

    private func key(for role: BeebFirmwareRole) -> (bookmark: String, name: String) {
        switch role {
        case .operatingSystem: return (BookmarkKey.os, BookmarkKey.osName)
        case .language: return (BookmarkKey.language, BookmarkKey.languageName)
        }
    }

    private func remember(_ bookmark: Data?, url: URL, role: BeebFirmwareRole) {
        let keys = key(for: role)
        if let bookmark { defaults.set(bookmark, forKey: keys.bookmark) }
        defaults.set(url.lastPathComponent, forKey: keys.name)
    }

    private func updateAssignment(_ name: String, role: BeebFirmwareRole) {
        switch role {
        case .operatingSystem:
            osAssignment = "OS ROM: \(name)"
            hasOSROM = !name.contains("recovery needed")
        case .language:
            languageAssignment = "Language ROM (bank 12): \(name)"
            hasLanguageROM = !name.contains("recovery needed")
        }
    }

    private func restoreRememberedFirmware() {
        restoreRememberedFirmware(role: .operatingSystem)
        restoreRememberedFirmware(role: .language)
        do { try resetIfFirmwareReady() } catch { status = error.localizedDescription }
    }

    private func restoreRememberedFirmware(role: BeebFirmwareRole) {
        guard let machine else { return }
        let keys = key(for: role)
        guard let bookmark = defaults.data(forKey: keys.bookmark) else { return }
        do {
            var stale = false
            let url = try URL(
                resolvingBookmarkData: bookmark,
                options: [.withSecurityScope],
                relativeTo: nil,
                bookmarkDataIsStale: &stale
            )
            let access = url.startAccessingSecurityScopedResource()
            defer { if access { url.stopAccessingSecurityScopedResource() } }
            try machine.loadFirmware(Data(contentsOf: url), role: role)
            if stale, let refreshed = try? url.bookmarkData(
                options: [.withSecurityScope, .securityScopeAllowOnlyReadAccess],
                includingResourceValuesForKeys: nil,
                relativeTo: nil
            ) {
                defaults.set(refreshed, forKey: keys.bookmark)
            }
            updateAssignment(
                defaults.string(forKey: keys.name) ?? url.lastPathComponent,
                role: role
            )
        } catch {
            updateAssignment("recovery needed", role: role)
            status = "\(role == .operatingSystem ? "OS" : "Language") ROM needs to be selected again."
        }
    }

    private func resetIfFirmwareReady() throws {
        guard let machine, hasOSROM && hasLanguageROM else {
            status = "Select both a Model B OS ROM and language ROM to reach BASIC-ready."
            return
        }
        try machine.reset()
        status = "Firmware ready — BASIC-ready"
        start()
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
                Button("Open Language ROM…") { model.isImportingLanguage = true }
                Button("Mount Disc…") { model.isImportingDisc = true }
                Button(model.isRunning ? "Pause" : "Run") { model.isRunning ? model.stop() : model.start() }
                Button("Break") { model.reset() }
            }
            Text(model.status).font(.caption.monospaced()).frame(maxWidth: .infinity, alignment: .leading)
            Text(model.osAssignment).frame(maxWidth: .infinity, alignment: .leading)
            Text(model.languageAssignment).frame(maxWidth: .infinity, alignment: .leading)
        }
        .padding()
        .fileImporter(isPresented: $model.isImportingOS, allowedContentTypes: [.data]) { result in
            if case let .success(url) = result { model.loadOS(url) }
        }
        .fileImporter(isPresented: $model.isImportingLanguage, allowedContentTypes: [.data]) { result in
            if case let .success(url) = result { model.loadFirmware(url, role: .language) }
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
