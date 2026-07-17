# C2 Xcode Project Contract

The repository contains `Beeb6502.xcodeproj` as maintained source. It provides
shared schemes for the macOS app, iOS Simulator app, and package tests. A clean
checkout can list, build, and test those schemes with `xcodebuild` and no
user-specific workspace state, absolute source path, signing identity, or
pre-existing derived data.

Validation rejects tracked or unignored user state but tolerates ignored local
`xcuserdata` created by ordinary Xcode use. It uses owned temporary derived data,
does not delete contributor settings, and verifies that the supported toolchain
does not rewrite maintained project metadata.

The project references the existing `BeebCore`, `BeebKit`, `BeebDemo`, and test
sources/package products. It does not copy core sources, replace
`Package.swift`, or become a prerequisite of the portable Makefile. Xcode Cloud,
distribution signing, archives, App Store metadata, and host output-framework
integration remain outside C2.
