// swift-tools-version: 5.9
import PackageDescription

let package = Package(
    name: "Beeb6502",
    platforms: [
        .macOS(.v13),
        .iOS(.v16),
    ],
    products: [
        .library(name: "BeebKit", targets: ["BeebKit"]),
        .executable(name: "BeebDemo", targets: ["BeebDemo"]),
    ],
    targets: [
        .target(
            name: "BeebCore",
            path: "Sources/BeebCore",
            publicHeadersPath: "include",
            cxxSettings: [.headerSearchPath("include")]
        ),
        .target(name: "BeebKit", dependencies: ["BeebCore"]),
        .executableTarget(name: "BeebDemo", dependencies: ["BeebKit"]),
        .testTarget(
            name: "BeebKitTests",
            dependencies: ["BeebKit"],
            path: "Tests/BeebKitTests"
        ),
    ],
    cxxLanguageStandard: .cxx20
)
