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
    @Published var screen: PlatformImage?
    @Published var status = "Choose a user-supplied BBC Model B OS ROM"
    @Published var isRunning = false
    @Published var isImportingOS = false
    @Published var isImportingDisc = false

    private let machine: BeebMachine?
    private var timer: Timer?

    init() {
        machine = try? BeebMachine()
        if machine == nil { status = "The emulator core could not start" }
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
    var body: some Scene { WindowGroup { ContentView() }.defaultSize(width: 900, height: 700) }
}
