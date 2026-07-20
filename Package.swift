// swift-tools-version: 5.9
import PackageDescription

var products: [Product] = [
    .library(name: "BeebKit", targets: ["BeebKit"]),
]

// C0-DOC-RATIONALE: docs/code/host-boundary.md owns the portable core and
// Apple-only host boundary; Linux must not resolve the SwiftUI demo target.
var targets: [Target] = [
    .target(
        name: "BeebCore",
        path: "Sources/BeebCore",
        publicHeadersPath: "include",
        cxxSettings: [.headerSearchPath("include")]
    ),
    .target(name: "BeebKit", dependencies: ["BeebCore"]),
]

#if os(macOS)
products.append(.executable(name: "BeebDemo", targets: ["BeebDemo"]))
targets.append(.executableTarget(name: "BeebDemo", dependencies: ["BeebKit"]))
#endif

targets.append(
    .testTarget(
        name: "BeebKitTests",
        dependencies: ["BeebKit"],
        path: "Tests/BeebKitTests"
    )
)

let package = Package(
    name: "Beeb6502",
    platforms: [
        .macOS(.v13),
        .iOS(.v16),
    ],
    products: products,
    dependencies: [
        // Command plugin only: this does not enter any runtime target graph.
        .package(
            url: "https://github.com/swiftlang/swift-docc-plugin.git",
            exact: "1.5.0"
        ),
    ],
    targets: targets,
    cxxLanguageStandard: .cxx20
)
