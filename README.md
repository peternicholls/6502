# Beeb6502

Beeb6502 is a clean, portable BBC Micro emulator core with a Swift front end for
macOS and iOS. Model B is the verified profile; the delivery plan commits Model
B+ 64K as a separately selectable profile and reserves an extensible path for
later BBC Micro-family machines and expansions. It continues the earlier
`Swift6502-Package`
prototype, but moves the hardware engine into dependency-free C++20 behind a
small C API. That keeps the core testable on non-Apple hosts while remaining
straightforward to call from Swift, Metal and AVAudioEngine.

Delivery is currently desktop-first: the macOS product will move to an
AppKit-first machine window and gain a bare terminal connection over the same
runtime before the committed iPhone/iPad product adaptation. This sequencing
does not add Apple dependencies to the C++ core.

No operating-system, BASIC, filing-system, game or character ROM is included.
The demo deliberately loads user-supplied ROMs and media through the platform
document picker.

The current development version is **0.3.0**. Public releases use Semantic
Versioning and `vMAJOR.MINOR.PATCH` Git tags.

## What works now

- All 151 documented NMOS MOS 6502 opcodes and addressing modes.
- Binary and NMOS decimal `ADC`/`SBC`, IRQ, NMI, BRK/RTI, stack behaviour,
  zero-page wrapping, page-cross penalties, read-modify-write dummy writes and
  the indirect-`JMP` page-wrap hardware bug.
- The Klaus Dormann 6502 functional binary reaches its success trap at `$3469`.
- BBC Model B 32 KiB RAM, 16 KiB OS ROM, sixteen 16 KiB sideways banks and the
  `$FE30` ROM latch.
- Two 6522 VIAs with ports, data-direction registers, Timer 1/2, interrupt
  flags/enables and CA1/CB1 edges.
- 6845 registers and frame timing, Video ULA control and palette writes,
  interleaved 1/2/4-bit bitmap decoding and screen-memory wrap.
- Mode 7 text, colour controls, flash and mosaics using an original clean-room
  host font rather than a third-party character ROM.
- SN76489 three-tone-plus-noise register protocol and 32-bit float audio
  generation.
- SSD and interleaved DSD images plus a pragmatic 8271 sector protocol for
  reads, writes, seeks, drive status and special registers.
- A C API, Swift wrapper, multiplatform SwiftUI shell, user file importers and
  a headless command-line runner.
- Owner-serialized capacity-three completed-frame output, continuous bounded
  48 kHz mono audio, and exact pressure/progress diagnostics across C++/C/Swift.

The core has also booted a privately supplied OS 1.20 plus BASIC II ROM pair to
the familiar `BBC COMPUTER 32K` / `BASIC` Mode 7 screen. Those ROMs are not
part of this repository.

## Build and test on Linux

```sh
make test
make all
```

The tests use only the standard library. They cover exhaustive binary
arithmetic, every valid two-digit BCD combination, NMOS decimal flag edge
vectors, opcode decoding, interrupts, stack/addressing quirks, VIA/CRTC/sound,
video, disc layouts and 8271 transfers.

Build and run the repository's original ROM demo without any proprietary
firmware:

```sh
make demo-rom all
.build/cpp/beeb-headless --os .build/cpp/cleanroom-demo.rom \
  --cycles 100000 --frame .build/cpp/cleanroom-demo.ppm
```

For the independent processor suite:

```sh
chmod +x scripts/run-klaus.sh
scripts/run-klaus.sh
```

The script downloads Klaus Dormann's GPL test image to the temporary directory;
it does not add it to this MIT-licensed project.

## Run a user-supplied BBC ROM set

```sh
.build/cpp/beeb-headless \
  --os /path/to/os12.rom \
  --rom 14 /path/to/basic2.rom \
  --cycles 5000000 \
  --frame boot.ppm
```

Add other sideways ROMs with another `--rom BANK FILE` pair. The BBC-mode
headless path rejects `--trace` before runtime creation; tracing is an
instruction-level diagnostic available only through standalone functional-test
mode. The core itself accepts
SSD/DSD images; the Swift demo currently exposes mounting through its file
importer. Supported host ownership and concurrency are described in the
[host-boundary guide](docs/code/host-boundary.md).

Inspect the installed runtime version with:

```sh
.build/cpp/beeb-headless --version
```

## Build on Apple platforms

Open `Beeb6502.xcodeproj` in Xcode. Its shared `BeebDemo-macOS`,
`BeebDemo-iOS`, and `Beeb6502-Tests` schemes are the maintained Apple app and
test entry points. Select the macOS scheme to run the desktop app, or the iOS
scheme with an iOS Simulator destination. The apps declare macOS 13 and iOS 16
minimums; signing is not required for simulator builds or tests.

The Xcode project consumes the local `BeebKit` package rather than copying its
sources. Swift Package Manager remains an independent build surface and can be
verified without opening the project:

```sh
swift build
swift test
```

The portable core remains independently buildable with `make test all`.
Run `make test-c2-portable` for the Linux-compatible bounded-output aggregate
and `make test-c2-xcode` for the maintained Apple project contract. Ordinary
ignored `xcuserdata` is permitted; tracked or unignored user state is not.

## Accuracy boundary

This is a booting development core, not yet a preservation-grade emulator.

- Instruction results and aggregate cycle counts are accurate, but bus reads
  and writes are not yet scheduled on individual half-cycles. Hardware is
  therefore advanced after an instruction, with up to a seven-cycle timing
  granularity.
- The CRTC models ordinary frame timing and the renderer models normal screen
  modes. Mid-scanline register tricks, interlace details, exact cursor shape
  and hardware-specific CRTC variants remain.
- Mode 7 double-height continuation, hold graphics and exact SAA5050 glyphs
  remain. The clean-room font is intentionally recognisable rather than a ROM
  facsimile.
- The 8271 implementation is logical-sector based. It does not model flux,
  deleted-data details, CRC faults, index timing or copy-protection behaviour.
- The 6850 ACIA/Serial ULA, UEF/WAV cassette path, analogue converter, Tube,
  Econet and speech are not implemented.
- Audio register behaviour is present; the Swift demo still needs an
  AVAudioEngine ring-buffer output path.

Start with the
[Machine application delivery plan](docs/product/MACHINE_DELIVERY_PLAN.md), the
sole authority for programme direction, feature order and delivery gates. The
[delivery ledger](docs/product/MACHINE_DELIVERY_PLAN.md#current-ledger)
clearly separates completed foundations from active, next, undelivered and
reserved work. The
[documentation index](docs/README.md) links supporting product intent, core
constraints, verified [status](docs/STATUS.md) and
[architecture](docs/ARCHITECTURE.md). The agreed native-window, safety,
program-input and terminal behavior is recorded in the
[desktop experience direction](docs/product/DESKTOP_EXPERIENCE.md).

## Releases and compatibility

The version is exposed to C and Swift hosts as well as the command-line tool.
Changes are recorded in [CHANGELOG.md](CHANGELOG.md); the release procedure and
compatibility policy are documented in [docs/RELEASING.md](docs/RELEASING.md).
Run `make check-version` before tagging a release.

Contributions should follow [CONTRIBUTING.md](CONTRIBUTING.md), the
[project constitution](.specify/memory/constitution.md), and the
[Spec Kit feature workflow](specs/README.md). Continuous integration builds the
warning-clean C++ core and the portable C2 aggregate under required
ThreadSanitizer on Linux. It independently verifies the Swift package, macOS
app, iOS Simulator app, and shared Xcode test scheme on macOS.

## Licence and references

Project code is MIT licensed. The implementation was checked against original
MOS/Intel/Acorn documentation, transistor-level decimal observations and the
Klaus functional tests. Reference links and licence boundaries are recorded in
[docs/REFERENCES.md](docs/REFERENCES.md).
