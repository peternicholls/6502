#include "beeb_c.h"

#include "beeb/machine.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <exception>
#include <new>
#include <span>

namespace {

constexpr std::size_t errorCapacity = 256;

} // namespace

struct beeb_machine {
    beeb::BBCMicro value;
    std::array<char, errorCapacity> lastError{};
};

namespace {

void setError(beeb_machine* machine, const char* message) noexcept {
    if (!machine) return;
    const auto length = std::min(std::strlen(message), machine->lastError.size() - 1);
    std::memcpy(machine->lastError.data(), message, length);
    machine->lastError[length] = '\0';
}

template <typename Result, typename Callable>
Result operation(beeb_machine* machine, Result failure, Callable&& callable) noexcept {
    if (!machine) return failure;
    machine->lastError[0] = '\0';
    try {
        return callable();
    } catch (const std::exception& error) {
        setError(machine, error.what());
    } catch (...) {
        setError(machine, "unknown C++ exception");
    }
    return failure;
}

template <typename Callable>
void operation(beeb_machine* machine, Callable&& callable) noexcept {
    if (!machine) return;
    machine->lastError[0] = '\0';
    try {
        callable();
    } catch (const std::exception& error) {
        setError(machine, error.what());
    } catch (...) {
        setError(machine, "unknown C++ exception");
    }
}

} // namespace

extern "C" {

const char* beeb_version_string(void) { return BEEB_VERSION_STRING; }

beeb_machine* beeb_create(void) {
    try { return new beeb_machine; } catch (...) { return nullptr; }
}

void beeb_destroy(beeb_machine* machine) { delete machine; }

const char* beeb_last_error(const beeb_machine* machine) {
    return machine ? machine->lastError.data() : nullptr;
}

int beeb_load_os_rom(beeb_machine* machine, const uint8_t* bytes, size_t count) {
    return operation(machine, 0, [=] {
        if (!bytes) {
            setError(machine, "OS ROM data is null");
            return 0;
        }
        if (!machine->value.loadOSROM(std::span(bytes, count))) {
            setError(machine, "OS ROM must be exactly 16384 bytes");
            return 0;
        }
        return 1;
    });
}

int beeb_load_sideways_rom(beeb_machine* machine, uint8_t bank, const uint8_t* bytes, size_t count) {
    return operation(machine, 0, [=] {
        if (!bytes) {
            setError(machine, "sideways ROM data is null");
            return 0;
        }
        if (!machine->value.loadSidewaysROM(bank, std::span(bytes, count))) {
            setError(machine, "invalid sideways ROM bank or size");
            return 0;
        }
        return 1;
    });
}

int beeb_mount_disc(beeb_machine* machine, unsigned drive, const uint8_t* bytes, size_t count,
                    int double_sided, int writable) {
    return operation(machine, 0, [=] {
        if (!bytes) {
            setError(machine, "disc image data is null");
            return 0;
        }
        const auto layout = double_sided ? beeb::DiscImage::Layout::DSD : beeb::DiscImage::Layout::SSD;
        if (!machine->value.mountDisc(drive, std::span(bytes, count), layout, writable != 0)) {
            setError(machine, "invalid drive or disc image");
            return 0;
        }
        return 1;
    });
}

void beeb_reset(beeb_machine* machine) {
    operation(machine, [=] { machine->value.reset(); });
}

uint64_t beeb_run_cycles(beeb_machine* machine, uint64_t cycles) {
    return operation(machine, std::uint64_t{0}, [=] { return machine->value.runFor(cycles); });
}

int beeb_run_until_frame(beeb_machine* machine, uint64_t maximum_cycles) {
    return operation(machine, -1, [=] { return machine->value.runUntilFrame(maximum_cycles) ? 1 : 0; });
}

beeb_cpu_state beeb_get_cpu_state(const beeb_machine* machine) {
    if (!machine) return {};
    const auto state = machine->value.cpu().state();
    return {state.a, state.x, state.y, state.sp, state.p, state.pc, state.cycles};
}

const uint8_t* beeb_get_frame_rgba(const beeb_machine* machine, uint32_t* width, uint32_t* height, uint64_t* number) {
    if (!machine) return nullptr;
    const auto& frame = machine->value.frame();
    if (width) *width = frame.width;
    if (height) *height = frame.height;
    if (number) *number = frame.number;
    return frame.rgba.empty() ? nullptr : frame.rgba.data();
}

void beeb_render_audio(beeb_machine* machine, float* mono, size_t frames, double sample_rate) {
    operation(machine, [=] {
        if (!mono) {
            setError(machine, "audio output buffer is null");
            return;
        }
        if (!std::isfinite(sample_rate) || sample_rate <= 0) {
            setError(machine, "audio sample rate must be finite and positive");
            return;
        }
        machine->value.sound().render(mono, frames, sample_rate);
    });
}

void beeb_set_key(beeb_machine* machine, uint8_t column, uint8_t row, int pressed) {
    operation(machine, [=] { machine->value.setKey(column, row, pressed != 0); });
}

void beeb_set_break(beeb_machine* machine, int pressed) {
    operation(machine, [=] { machine->value.setBreak(pressed != 0); });
}

} // extern "C"
