# Changelog

All notable changes to Beeb6502 are documented in this file. The project uses
[Semantic Versioning](https://semver.org/spec/v2.0.0.html) and follows the
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/) structure.

## [Unreleased]

### Added

- Swift package regression tests for the public host boundary.
- A public runtime version contract and `--version` command.
- A documented release checklist and version-consistency check.

## [0.1.0] - 2026-07-15

### Added

- Complete documented NMOS 6502 instruction set with binary and decimal
  arithmetic tests.
- BBC Model B memory map, VIAs, CRTC, Video ULA, Mode 7, SN76489 and logical
  8271 disc support.
- Dependency-free C++20 core, C ABI, Swift wrapper, SwiftUI demo and headless
  command-line runner.
- SSD/DSD media support and clean-room demonstration ROM tooling.

### Fixed

- Prevented C++ exceptions from escaping across the C ABI into Swift hosts.
- Serialized ROM and disc mutation with emulation execution in `BeebMachine`.
- Corrected nonzero CRTC vertical-adjust frame timing.
- Rendered Mode 7 control-code cells using the active background colour.

[Unreleased]: https://github.com/peternicholls/6502/compare/v0.1.0...HEAD
[0.1.0]: https://github.com/peternicholls/6502/releases/tag/v0.1.0
