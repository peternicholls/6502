#include "beeb_c.h"

#include <charconv>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

// Documentation rationale: docs/code/host-boundary.md owns structured status,
// runtime-handle, and caller-owned frame rules shared by command-line hosts.

struct Output {
    enum class Kind { State, Frame };
    Kind kind;
    std::string path;
};

struct Options {
    std::string romPath;
    std::string workload;
    std::uint64_t requestedCycles = 0;
    std::vector<Output> outputs;
};

void requireStatus(const beeb_status& status) {
    if (status.code == BEEB_STATUS_OK) return;
    throw std::runtime_error(status.message[0] != '\0' ? status.message
                                                       : "emulator core operation failed");
}

/// Releases the evidence tool's C runtime on every exit path.
struct MachineDeleter final {
    void operator()(beeb_machine* machine) const noexcept {
        if (machine) (void)beeb_destroy(machine);
    }
};

using Machine = std::unique_ptr<beeb_machine, MachineDeleter>;

/// Retains one caller-owned frame through all requested evidence writes.
struct OwnedFrame final {
    beeb_frame value{};
    ~OwnedFrame() { (void)beeb_frame_release(&value); }
};

std::vector<std::uint8_t> readFile(const std::string& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) throw std::runtime_error("cannot open ROM: " + path);
    input.seekg(0, std::ios::end);
    const auto length = input.tellg();
    input.seekg(0, std::ios::beg);
    if (length < 0) throw std::runtime_error("cannot size ROM: " + path);
    std::vector<std::uint8_t> bytes(static_cast<std::size_t>(length));
    input.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    if (!input && !bytes.empty()) throw std::runtime_error("cannot read ROM: " + path);
    return bytes;
}

std::uint64_t positiveNumber(std::string_view text) {
    std::uint64_t value = 0;
    const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value);
    if (error != std::errc{} || end != text.data() + text.size() || value == 0) {
        throw std::runtime_error("cycles must be a positive integer");
    }
    return value;
}

Output parseOutput(const std::string& value) {
    const auto separator = value.find(':');
    if (separator == std::string::npos || separator + 1 == value.size()) {
        throw std::runtime_error("output must be KIND:PATH");
    }
    const auto kind = value.substr(0, separator);
    const auto path = value.substr(separator + 1);
    if (kind == "state") return {Output::Kind::State, path};
    if (kind == "frame") return {Output::Kind::Frame, path};
    throw std::runtime_error("unknown output kind: " + kind);
}

Options parseOptions(int argc, char** argv) {
    Options options;
    for (int index = 1; index < argc; ++index) {
        const std::string_view argument = argv[index];
        const auto next = [&]() -> std::string {
            if (++index >= argc) throw std::runtime_error("missing option value");
            return argv[index];
        };
        if (argument == "--rom")
            options.romPath = next();
        else if (argument == "--workload")
            options.workload = next();
        else if (argument == "--cycles")
            options.requestedCycles = positiveNumber(next());
        else if (argument == "--output")
            options.outputs.push_back(parseOutput(next()));
        else if (argument == "--help" || argument == "-h") {
            std::cout << "beeb-evidence --rom ROM --workload bitmap|mode7 --cycles N "
                         "--output state:PATH [--output frame:PATH]\n";
            std::exit(0);
        } else {
            throw std::runtime_error("unknown option: " + std::string(argument));
        }
    }

    if (options.workload.empty()) throw std::runtime_error("workload is required");
    if (options.workload != "bitmap" && options.workload != "mode7") {
        throw std::runtime_error("unknown workload: " + options.workload);
    }
    if (options.romPath.empty()) throw std::runtime_error("ROM path is required");
    if (options.requestedCycles == 0) throw std::runtime_error("cycles must be a positive integer");
    if (options.outputs.empty()) throw std::runtime_error("at least one output is required");
    return options;
}

void writeState(const std::string& path, const Options& options, const beeb_cpu_state& state,
                std::uint64_t actualCycles, std::uint32_t width, std::uint32_t height,
                std::uint64_t frameNumber) {
    std::ofstream output(path);
    if (!output) throw std::runtime_error("cannot write state output: " + path);
    output << "schema=beeb-c0-evidence-v1\n"
           << "workload=" << options.workload << '\n'
           << "requested_cycles=" << options.requestedCycles << '\n'
           << "actual_cycles=" << actualCycles << '\n'
           << std::hex << std::uppercase << std::setfill('0') << "pc=$" << std::setw(4) << state.pc
           << '\n'
           << "a=$" << std::setw(2) << static_cast<unsigned>(state.a) << '\n'
           << "x=$" << std::setw(2) << static_cast<unsigned>(state.x) << '\n'
           << "y=$" << std::setw(2) << static_cast<unsigned>(state.y) << '\n'
           << "sp=$" << std::setw(2) << static_cast<unsigned>(state.sp) << '\n'
           << "p=$" << std::setw(2) << static_cast<unsigned>(state.p) << '\n'
           << std::dec << "frame_number=" << frameNumber << '\n'
           << "frame_width=" << width << '\n'
           << "frame_height=" << height << '\n';
    if (!output) throw std::runtime_error("cannot write state output: " + path);
}

void writeFrame(const std::string& path, const std::uint8_t* rgba, std::uint32_t width,
                std::uint32_t height) {
    if (!rgba || width == 0 || height == 0) throw std::runtime_error("no frame is available");
    std::ofstream output(path, std::ios::binary);
    if (!output) throw std::runtime_error("cannot write frame output: " + path);
    output << "P6\n" << width << ' ' << height << "\n255\n";
    const auto pixelCount = static_cast<std::size_t>(width) * height;
    for (std::size_t pixel = 0; pixel < pixelCount; ++pixel) {
        output.write(reinterpret_cast<const char*>(rgba + pixel * 4), 3);
    }
    if (!output) throw std::runtime_error("cannot write frame output: " + path);
}

int run(const Options& options) {
    const auto rom = readFile(options.romPath);
    beeb_machine* rawMachine = nullptr;
    requireStatus(beeb_create(&rawMachine));
    Machine machine(rawMachine);
    requireStatus(beeb_load_os_rom(machine.get(), rom.data(), rom.size()));
    requireStatus(beeb_reset(machine.get()));
    std::uint64_t actualCycles = 0;
    requireStatus(beeb_run_cycles(machine.get(), options.requestedCycles, &actualCycles));

    beeb_cpu_state state{};
    requireStatus(beeb_get_cpu_state(machine.get(), &state));
    OwnedFrame frame;
    requireStatus(beeb_get_frame(machine.get(), &frame.value));

    for (const auto& output : options.outputs) {
        if (output.kind == Output::Kind::State) {
            writeState(output.path, options, state, actualCycles, frame.value.width,
                       frame.value.height, frame.value.number);
        } else {
            writeFrame(output.path, frame.value.rgba, frame.value.width, frame.value.height);
        }
    }

    std::cout << "workload=" << options.workload << " requested_cycles=" << options.requestedCycles
              << " actual_cycles=" << actualCycles << " frame=" << frame.value.number << ' '
              << frame.value.width << 'x' << frame.value.height << '\n';
    return 0;
}

} // namespace

int main(int argc, char** argv) {
    try {
        return run(parseOptions(argc, argv));
    } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << '\n';
        return 1;
    }
}
