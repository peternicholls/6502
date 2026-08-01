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

#if os(macOS)
/// One physical-key translation into the BBC Model B matrix.
fileprivate struct BBCKeyPosition {
    let column: UInt8
    let row: UInt8
    let requiresShift: Bool
}

/// Focusable AppKit bridge that forwards physical key transitions to the host
/// model; it never touches the machine directly.
fileprivate struct MachineKeyboardCapture: NSViewRepresentable {
    let onKey: (BBCKeyPosition, Bool) -> Void
    let onFocus: (Bool) -> Void
    let focusRequest: Int

    final class KeyView: NSView {
        var onKey: ((BBCKeyPosition, Bool) -> Void)?
        var onFocus: ((Bool) -> Void)?
        var focusRequest = 0

        override var acceptsFirstResponder: Bool { true }

        override func viewDidMoveToWindow() {
            super.viewDidMoveToWindow()
            guard window != nil else { return }
            requestFocus()
        }

        override func becomeFirstResponder() -> Bool {
            let result = super.becomeFirstResponder()
            if result { publishFocus(true) }
            return result
        }

        override func resignFirstResponder() -> Bool {
            let result = super.resignFirstResponder()
            if result { publishFocus(false) }
            return result
        }

        private func publishFocus(_ focused: Bool) {
            DispatchQueue.main.async { [weak self] in
                guard let self else { return }
                self.onFocus?(focused)
            }
        }

        func requestFocus() {
            DispatchQueue.main.async { [weak self] in
                guard let self, self.window != nil else { return }
                self.window?.makeFirstResponder(self)
            }
        }

        override func keyDown(with event: NSEvent) {
            guard let position = Self.position(for: event) else { return }
            onKey?(position, true)
        }

        override func keyUp(with event: NSEvent) {
            guard let position = Self.position(for: event) else { return }
            onKey?(position, false)
        }

        private static func position(for event: NSEvent) -> BBCKeyPosition? {
            if event.keyCode == 36 { return BBCKeyPosition(column: 9, row: 4, requiresShift: false) }
            guard let character = event.charactersIgnoringModifiers?.uppercased() else { return nil }
            let shift = event.characters == "\""
            switch character {
            case "1": return BBCKeyPosition(column: 0, row: 3, requiresShift: false)
            case "2": return BBCKeyPosition(column: 1, row: 3, requiresShift: shift)
            case "5": return BBCKeyPosition(column: 3, row: 1, requiresShift: shift)
            case "6": return BBCKeyPosition(column: 4, row: 3, requiresShift: shift)
            case "0": return BBCKeyPosition(column: 7, row: 2, requiresShift: false)
            case "B": return BBCKeyPosition(column: 4, row: 6, requiresShift: false)
            case "E": return BBCKeyPosition(column: 2, row: 2, requiresShift: false)
            case "I": return BBCKeyPosition(column: 5, row: 2, requiresShift: false)
            case "N": return BBCKeyPosition(column: 5, row: 5, requiresShift: false)
            case "P": return BBCKeyPosition(column: 7, row: 3, requiresShift: false)
            case "R": return BBCKeyPosition(column: 3, row: 3, requiresShift: false)
            case "T": return BBCKeyPosition(column: 3, row: 2, requiresShift: false)
            case "U": return BBCKeyPosition(column: 5, row: 3, requiresShift: false)
            case " ": return BBCKeyPosition(column: 2, row: 6, requiresShift: false)
            default: return nil
            }
        }
    }

    func makeNSView(context: Context) -> KeyView {
        let view = KeyView()
        view.onKey = onKey
        view.onFocus = onFocus
        view.focusRequest = focusRequest
        return view
    }

    func updateNSView(_ nsView: KeyView, context: Context) {
        nsView.onKey = onKey
        nsView.onFocus = onFocus
        if nsView.focusRequest != focusRequest {
            nsView.focusRequest = focusRequest
            nsView.requestFocus()
        }
    }
}
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
    @Published private(set) var isDownloadingFirmware = false
    @Published var requestedProfile = MachineProfileChoice.modelB
    @Published private(set) var activeProfile: BeebMachineProfile?
    @Published private(set) var profileStatus = "No machine profile is active"
    @Published private(set) var osAssignment = "OS ROM: not assigned"
    @Published private(set) var languageAssignment = "Language ROM: not assigned"
    @Published private(set) var inputFocus = false
    @Published private(set) var presentationEpoch: UInt64 = 0
    @Published private(set) var lastPresentedFrame: UInt64 = 0

    /// Replaced only after a requested candidate constructs and reports its profile.
    private var machine: BeebMachine?
    private var timer: Timer?
    private let defaults = UserDefaults.standard
    private var hasOSROM = false
    private var hasLanguageROM = false
    let documentedProgram = "10 PRINT \"BEEB6502\""

    static let firmwareRepositoryURL = URL(
        string: "https://mdfs.net/System/ROMs/AcornMOS/BBC_120/"
    )!

    private static let downloadableFirmware: [(role: BeebFirmwareRole, name: String, url: URL)] = [
        (.operatingSystem, "MOS120.rom", firmwareRepositoryURL.appendingPathComponent("MOS120")),
        (.language, "BASIC200.rom", firmwareRepositoryURL.appendingPathComponent("BASIC200")),
    ]

    private enum BookmarkKey {
        static let os = "model-b.os-bookmark"
        static let language = "model-b.language-bookmark"
        static let osName = "model-b.os-name"
        static let languageName = "model-b.language-name"
    }

    private var bookmarkCreationOptions: URL.BookmarkCreationOptions {
        // The development host is unsigned and intentionally outside the App Sandbox.
        // Plain bookmarks keep remembered ROMs usable without a scoped-bookmark agent.
        return []
    }

    private var bookmarkResolutionOptions: URL.BookmarkResolutionOptions {
        // See bookmarkCreationOptions: this host does not have sandbox entitlements.
        return []
    }

    init() {
        installRequestedProfile()
        restoreRememberedFirmware()
    }

    /// Builds a candidate before atomically installing its runtime and active identity.
    func installRequestedProfile() {
        if activeProfile == requestedProfile.profile {
            profileStatus = "Active profile: \(requestedProfile.displayName)"
            return
        }
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
            let resourceValues = try url.resourceValues(forKeys: [.fileSizeKey, .isRegularFileKey])
            guard resourceValues.isRegularFile == true, let fileSize = resourceValues.fileSize else {
                throw CocoaError(.fileReadUnsupportedScheme)
            }
            switch role {
            case .operatingSystem where fileSize != 16 * 1024:
                throw BeebError.invalidOSROM
            case .language where !(1...16 * 1024).contains(fileSize):
                throw BeebError.invalidSidewaysROM
            default:
                break
            }
            let data = try Data(contentsOf: url)
            let bookmark = try url.bookmarkData(
                options: bookmarkCreationOptions,
                includingResourceValuesForKeys: nil,
                relativeTo: nil
            )
            try machine.loadFirmware(data, role: role)
            remember(bookmark, url: url, role: role)
            updateAssignment(url.lastPathComponent, role: role)
            try resetIfFirmwareReady()
        } catch {
            let roleName = role == .operatingSystem ? "OS ROM" : "Language ROM"
            status = "\(roleName) was not changed — \(error.localizedDescription)"
        }
    }

    func loadOS(_ url: URL) { loadFirmware(url, role: .operatingSystem) }

    /// Downloads the user-requested Model B firmware pair into private app
    /// storage, then loads it through the same runtime contract as imported ROMs.
    func downloadModelBFirmware() async {
        guard let machine, !isDownloadingFirmware else { return }
        isDownloadingFirmware = true
        status = "Downloading Model B ROMs…"
        defer { isDownloadingFirmware = false }

        do {
            let romDirectory = try firmwareDirectory()
            var downloads: [(role: BeebFirmwareRole, name: String, url: URL, data: Data)] = []

            for firmware in Self.downloadableFirmware {
                let (data, response) = try await URLSession.shared.data(from: firmware.url)
                guard let response = response as? HTTPURLResponse,
                      (200...299).contains(response.statusCode) else {
                    throw URLError(.badServerResponse)
                }
                switch firmware.role {
                case .operatingSystem where data.count != 16 * 1024:
                    throw BeebError.invalidOSROM
                case .language where !(1...16 * 1024).contains(data.count):
                    throw BeebError.invalidSidewaysROM
                default:
                    break
                }
                downloads.append((firmware.role, firmware.name,
                                  romDirectory.appendingPathComponent(firmware.name), data))
            }

            for download in downloads {
                try download.data.write(to: download.url, options: .atomic)
                try machine.loadFirmware(download.data, role: download.role)
                let bookmark = try download.url.bookmarkData(
                    options: bookmarkCreationOptions,
                    includingResourceValuesForKeys: nil,
                    relativeTo: nil
                )
                remember(bookmark, url: download.url, role: download.role)
                updateAssignment(download.name, role: download.role)
            }
            try resetIfFirmwareReady()
        } catch {
            status = "Model B ROM download failed — \(error.localizedDescription)"
        }
    }

    private func firmwareDirectory() throws -> URL {
        let base = try FileManager.default.url(
            for: .applicationSupportDirectory,
            in: .userDomainMask,
            appropriateFor: nil,
            create: true
        )
        let directory = base.appendingPathComponent("BBC Micro ROMS", isDirectory: true)
        try FileManager.default.createDirectory(at: directory, withIntermediateDirectories: true)
        return directory
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

    #if os(macOS)
    func setInputFocus(_ focused: Bool) {
        inputFocus = focused
    }

    fileprivate func handleKey(_ position: BBCKeyPosition, pressed: Bool) {
        guard let machine else { return }
        do {
            if position.requiresShift && pressed {
                try machine.setKey(column: 0, row: 0, pressed: true)
            }
            try machine.setKey(column: position.column, row: position.row, pressed: pressed)
            if position.requiresShift && !pressed {
                try machine.setKey(column: 0, row: 0, pressed: false)
            }
            status = inputFocus ? "Keyboard focus active — \(documentedProgram), Return, RUN, Return" : status
        } catch {
            status = error.localizedDescription
            stopPresentation()
        }
    }
    #endif

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
                options: bookmarkResolutionOptions,
                relativeTo: nil,
                bookmarkDataIsStale: &stale
            )
            let access = url.startAccessingSecurityScopedResource()
            defer { if access { url.stopAccessingSecurityScopedResource() } }
            try machine.loadFirmware(Data(contentsOf: url), role: role)
            if stale, let refreshed = try? url.bookmarkData(
                options: bookmarkCreationOptions,
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
            let diagnostic = String(describing: error)
            updateAssignment("recovery needed — \(diagnostic)", role: role)
            status = "\(role == .operatingSystem ? "OS" : "Language") ROM needs to be selected again — \(diagnostic)"
        }
    }

    private func resetIfFirmwareReady() throws {
        guard let machine, hasOSROM && hasLanguageROM else {
            status = "Select both a Model B OS ROM and language ROM to reach BASIC-ready."
            return
        }
        try machine.reset()
        invalidatePresentation()
        status = "Firmware ready — BASIC-ready"
        start()
    }

    func start() {
        guard timer == nil else { return }
        guard let machine else { return }
        do {
            try machine.start()
            isRunning = true
            status = "Running"
            timer = Timer.scheduledTimer(withTimeInterval: 1.0 / 50.0, repeats: true) {
                [weak self] _ in
                Task { @MainActor in self?.pollFrame() }
            }
        } catch {
            status = error.localizedDescription
            stopPresentation()
        }
    }

    private func stopPresentation() {
        timer?.invalidate()
        timer = nil
        isRunning = false
    }

    func pause() {
        guard let machine else { return }
        do {
            try machine.pause()
            stopPresentation()
            status = "Paused"
        } catch {
            status = error.localizedDescription
        }
    }

    func reset() {
        guard let machine else { return }
        let shouldResume = isRunning
        do {
            try machine.reset()
            stopPresentation()
            invalidatePresentation()
            status = "Reset complete — BASIC-ready"
            if shouldResume { start() }
        }
        catch { status = error.localizedDescription; stopPresentation() }
    }

    func breakExecution() {
        guard let machine else { return }
        do {
            try machine.setBreak(pressed: true)
            try machine.setBreak(pressed: false)
            invalidatePresentation()
            status = "BREAK accepted — presentation epoch \(presentationEpoch)"
        } catch {
            status = error.localizedDescription
        }
    }

    private func invalidatePresentation() {
        presentationEpoch &+= 1
        lastPresentedFrame = 0
        screen = nil
    }

    private func pollFrame() {
        guard let machine else { return }
        do {
            let frame = try machine.dequeueVideoFrame()
            guard frame.number > lastPresentedFrame else { return }
            lastPresentedFrame = frame.number
            screen = platformImage(frame)
            let cpu = try machine.cpuState()
            status = String(format: "Frame %llu · epoch %llu · PC %04X   %llu cycles", frame.number, presentationEpoch, cpu.programCounter, cpu.cycles)
        } catch BeebError.coreStatus(.empty, _) {
            return
        } catch {
            status = error.localizedDescription
            stopPresentation()
        }
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
    #if os(macOS)
    @State private var keyboardFocusRequest = 0
    #endif

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
                Button(model.isDownloadingFirmware ? "Downloading ROMs…" : "Download Model B ROMs") {
                    Task { await model.downloadModelBFirmware() }
                }
                .disabled(model.isDownloadingFirmware)
                Button("Mount Disc…") { model.isImportingDisc = true }
                Button("Run") { model.start() }
                    .accessibilityIdentifier("run-control")
                Button("Pause") { model.pause() }
                    .accessibilityIdentifier("pause-control")
                Button("Reset") { model.reset() }
                    .accessibilityIdentifier("reset-control")
                Button("BREAK") { model.breakExecution() }
                    .accessibilityIdentifier("break-control")
            }
            Link("BBC Micro ROM repository", destination: EmulatorModel.firmwareRepositoryURL)
                .frame(maxWidth: .infinity, alignment: .leading)
            #if os(macOS)
            Button("Focus keyboard") { keyboardFocusRequest &+= 1 }
                .accessibilityIdentifier("focus-keyboard-control")
            #endif
            Text(model.status).font(.caption.monospaced()).frame(maxWidth: .infinity, alignment: .leading)
            Text(model.osAssignment).frame(maxWidth: .infinity, alignment: .leading)
            Text(model.languageAssignment).frame(maxWidth: .infinity, alignment: .leading)
            Text("Keyboard focus: \(model.inputFocus ? "active" : "not active")")
                .frame(maxWidth: .infinity, alignment: .leading)
                .accessibilityLabel("Machine keyboard focus")
                .accessibilityValue(model.inputFocus ? "Active" : "Not active")
            #if os(macOS)
            MachineKeyboardCapture(
                onKey: { position, pressed in model.handleKey(position, pressed: pressed) },
                onFocus: { focused in model.setInputFocus(focused) },
                focusRequest: keyboardFocusRequest
            )
            .frame(height: 1)
            .accessibilityLabel("Machine keyboard input")
            #endif
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
