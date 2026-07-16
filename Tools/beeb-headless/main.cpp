#include "beeb/cpu6502.hpp"
#include "beeb/version.h"
#include "beeb_c.h"

#include <algorithm>
#include <array>
#include <charconv>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

// Documentation rationale: docs/code/host-boundary.md owns structured status,
// runtime-handle, and caller-owned frame rules shared by command-line hosts.

void requireStatus(const beeb_status& status) {
    if (status.code == BEEB_STATUS_OK) return;
    throw std::runtime_error(status.message[0] != '\0' ? status.message
                                                       : "emulator core operation failed");
}

/// Releases a C runtime when the headless command leaves scope.
struct MachineDeleter final {
    void operator()(beeb_machine* machine) const noexcept {
        if (machine) (void)beeb_destroy(machine);
    }
};

/// Owning C-handle alias for one headless runtime; destruction releases exactly
/// the adopted handle through beeb_destroy.
using Machine = std::unique_ptr<beeb_machine, MachineDeleter>;

/// Releases the caller-owned C frame after output or error handling completes.
struct OwnedFrame final {
    beeb_frame value{};
    ~OwnedFrame() { (void)beeb_frame_release(&value); }
};

std::vector<std::uint8_t> readFile(const std::string& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) throw std::runtime_error("cannot open " + path);
    input.seekg(0, std::ios::end);
    const auto length = input.tellg();
    input.seekg(0, std::ios::beg);
    if (length < 0) throw std::runtime_error("cannot size " + path);
    std::vector<std::uint8_t> bytes(static_cast<std::size_t>(length));
    input.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    if (!input && !bytes.empty()) throw std::runtime_error("cannot read " + path);
    return bytes;
}

void writeFrame(const beeb_frame& frame, const std::string& path) {
    if (!frame.available || !frame.rgba) {
        throw std::runtime_error("no video frame is available");
    }
    std::ofstream output(path, std::ios::binary);
    if (!output) throw std::runtime_error("cannot create " + path);
    output << "P6\n" << frame.width << ' ' << frame.height << "\n255\n";
    for (std::size_t offset = 0; offset < frame.rgba_size; offset += 4) {
        output.write(reinterpret_cast<const char*>(frame.rgba + offset), 3);
    }
}

std::uint64_t number(std::string_view text) {
    int base = 10;
    if (text.starts_with("0x") || text.starts_with("0X")) {
        base = 16;
        text.remove_prefix(2);
    } else if (text.starts_with('$')) {
        base = 16;
        text.remove_prefix(1);
    }
    std::uint64_t value = 0;
    const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value, base);
    if (error != std::errc{} || end != text.data() + text.size())
        throw std::runtime_error("invalid number");
    return value;
}

std::uint64_t boundedNumber(std::string_view text, std::string_view option, std::uint64_t maximum) {
    const auto value = number(text);
    if (value > maximum) {
        throw std::runtime_error(std::string(option) + " must be in 0..." +
                                 std::to_string(maximum));
    }
    return value;
}

void printState(const beeb::CPUState& s) {
    std::cout << std::hex << std::uppercase << std::setfill('0') << "PC=$" << std::setw(4) << s.pc
              << " A=$" << std::setw(2) << static_cast<unsigned>(s.a) << " X=$" << std::setw(2)
              << static_cast<unsigned>(s.x) << " Y=$" << std::setw(2) << static_cast<unsigned>(s.y)
              << " SP=$" << std::setw(2) << static_cast<unsigned>(s.sp) << " P=$" << std::setw(2)
              << static_cast<unsigned>(s.p) << std::dec << " cycles=" << s.cycles << '\n';
}

void printState(const beeb_cpu_state& s) {
    std::cout << std::hex << std::uppercase << std::setfill('0') << "PC=$" << std::setw(4) << s.pc
              << " A=$" << std::setw(2) << static_cast<unsigned>(s.a) << " X=$" << std::setw(2)
              << static_cast<unsigned>(s.x) << " Y=$" << std::setw(2) << static_cast<unsigned>(s.y)
              << " SP=$" << std::setw(2) << static_cast<unsigned>(s.sp) << " P=$" << std::setw(2)
              << static_cast<unsigned>(s.p) << std::dec << " cycles=" << s.cycles << '\n';
}

/// Side-effect-free 64 KiB adapter for functional CPU images. It intentionally
/// advances no devices; tests supply already materialized memory and observe
/// only CPU reads/writes.
class FlatBus final : public beeb::Bus {
  public:
    std::array<std::uint8_t, 65536> memory{};
    std::uint8_t read(std::uint16_t address) override { return memory[address]; }
    void write(std::uint16_t address, std::uint8_t value) override { memory[address] = value; }
};

int runFunctional(const std::string& path, std::uint16_t pc, std::uint64_t maximum,
                  std::optional<std::uint16_t> expectedSuccess, bool trace) {
    const auto image = readFile(path);
    if (image.size() != 65536)
        throw std::runtime_error("functional image must be exactly 65536 bytes");
    FlatBus bus;
    std::copy(image.begin(), image.end(), bus.memory.begin());
    beeb::CPU6502 cpu(bus);
    beeb::CPUState initial;
    initial.pc = pc;
    initial.sp = 0xFF;
    initial.p = beeb::CPU6502::Unused;
    cpu.setState(initial);
    if (trace)
        cpu.setTraceCallback([](const auto& state, auto opcode) {
            std::cout << std::hex << std::uppercase << std::setfill('0') << '$' << std::setw(4)
                      << static_cast<unsigned>(state.pc - 1) << "  " << std::setw(2)
                      << static_cast<unsigned>(opcode) << "  ";
            printState(state);
        });

    std::uint16_t previous = 0xFFFF;
    unsigned stationary = 0;
    for (std::uint64_t instruction = 0; instruction < maximum; ++instruction) {
        cpu.step();
        const auto current = cpu.state().pc;
        stationary = current == previous ? stationary + 1 : 0;
        previous = current;
        if (stationary >= 100) {
            printState(cpu.state());
            if (expectedSuccess && current != *expectedSuccess) {
                std::cerr << "Functional test trapped at $" << std::hex << std::uppercase << current
                          << "; expected success at $" << *expectedSuccess << '\n';
                return 2;
            }
            std::cout << "Functional test reached stable trap at $" << std::hex << std::uppercase
                      << current << '\n';
            return 0;
        }
    }
    printState(cpu.state());
    std::cerr << "Functional test instruction limit reached\n";
    return 3;
}

int runBBC(const std::string& osPath, const std::vector<std::pair<unsigned, std::string>>& roms,
           std::uint64_t cycles, bool trace, const std::string& framePath) {
    if (trace) {
        throw std::runtime_error("--trace is supported only with --functional");
    }
    beeb_machine* rawMachine = nullptr;
    requireStatus(beeb_create(&rawMachine));
    Machine machine(rawMachine);
    const auto os = readFile(osPath);
    requireStatus(beeb_load_os_rom(machine.get(), os.data(), os.size()));
    for (const auto& [bank, path] : roms) {
        if (bank > 15) throw std::runtime_error("sideways ROM bank must be in 0...15");
        const auto bytes = readFile(path);
        requireStatus(beeb_load_sideways_rom(machine.get(), static_cast<std::uint8_t>(bank),
                                             bytes.data(), bytes.size()));
    }
    requireStatus(beeb_reset(machine.get()));
    std::uint64_t actualCycles = 0;
    requireStatus(beeb_run_cycles(machine.get(), cycles, &actualCycles));
    beeb_cpu_state state{};
    requireStatus(beeb_get_cpu_state(machine.get(), &state));
    printState(state);
    OwnedFrame frame;
    requireStatus(beeb_get_frame(machine.get(), &frame.value));
    std::cout << "frame=" << frame.value.number << " " << frame.value.width << 'x'
              << frame.value.height << '\n';
    if (!framePath.empty()) {
        writeFrame(frame.value, framePath);
        std::cout << "wrote " << framePath << '\n';
    }
    return 0;
}

void usage() {
    std::cout << "beeb-headless --version\n"
                 "beeb-headless --os MOS.rom [--rom BANK ROM] [--cycles N] [--frame output.ppm]\n"
                 "beeb-headless --functional 6502_functional_test.bin [--pc 0x0400]\n"
                 "              [--success 0x3469] [--max-instructions N] [--trace]\n";
}

} // namespace

int main(int argc, char** argv) {
    try {
        std::string osPath;
        std::string functionalPath;
        std::vector<std::pair<unsigned, std::string>> roms;
        std::uint64_t cycles = 2'000'000;
        std::uint64_t maximum = 100'000'000;
        std::uint16_t pc = 0x0400;
        std::optional<std::uint16_t> success;
        std::string framePath;
        bool trace = false;

        for (int i = 1; i < argc; ++i) {
            const std::string_view argument = argv[i];
            const auto next = [&]() -> std::string {
                if (++i >= argc) throw std::runtime_error("missing option value");
                return argv[i];
            };
            if (argument == "--os")
                osPath = next();
            else if (argument == "--functional")
                functionalPath = next();
            else if (argument == "--cycles")
                cycles = number(next());
            else if (argument == "--max-instructions")
                maximum = number(next());
            else if (argument == "--pc")
                pc = static_cast<std::uint16_t>(
                    boundedNumber(next(), "--pc", std::numeric_limits<std::uint16_t>::max()));
            else if (argument == "--success")
                success = static_cast<std::uint16_t>(
                    boundedNumber(next(), "--success", std::numeric_limits<std::uint16_t>::max()));
            else if (argument == "--frame")
                framePath = next();
            else if (argument == "--rom") {
                const auto bank = static_cast<unsigned>(boundedNumber(next(), "--rom BANK", 15));
                roms.emplace_back(bank, next());
            } else if (argument == "--trace")
                trace = true;
            else if (argument == "--version") {
                std::cout << "Beeb6502 " BEEB_VERSION_STRING "\n";
                return 0;
            } else if (argument == "--help" || argument == "-h") {
                usage();
                return 0;
            } else
                throw std::runtime_error("unknown option " + std::string(argument));
        }

        if (!functionalPath.empty())
            return runFunctional(functionalPath, pc, maximum, success, trace);
        if (!osPath.empty()) return runBBC(osPath, roms, cycles, trace, framePath);
        usage();
        return 1;
    } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << '\n';
        return 1;
    }
}
