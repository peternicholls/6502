#include "beeb_c.h"

#include "beeb/machine.hpp"

#include <new>
#include <span>

struct beeb_machine { beeb::BBCMicro value; };

extern "C" {

beeb_machine* beeb_create(void) {
    try { return new beeb_machine; } catch (...) { return nullptr; }
}

void beeb_destroy(beeb_machine* machine) { delete machine; }

int beeb_load_os_rom(beeb_machine* machine, const uint8_t* bytes, size_t count) {
    return machine && bytes && machine->value.loadOSROM(std::span(bytes, count));
}

int beeb_load_sideways_rom(beeb_machine* machine, uint8_t bank, const uint8_t* bytes, size_t count) {
    return machine && bytes && machine->value.loadSidewaysROM(bank, std::span(bytes, count));
}

int beeb_mount_disc(beeb_machine* machine, unsigned drive, const uint8_t* bytes, size_t count,
                    int double_sided, int writable) {
    if (!machine || !bytes) return 0;
    const auto layout = double_sided ? beeb::DiscImage::Layout::DSD : beeb::DiscImage::Layout::SSD;
    return machine->value.mountDisc(drive, std::span(bytes, count), layout, writable != 0);
}

void beeb_reset(beeb_machine* machine) { if (machine) machine->value.reset(); }

uint64_t beeb_run_cycles(beeb_machine* machine, uint64_t cycles) {
    return machine ? machine->value.runFor(cycles) : 0;
}

int beeb_run_until_frame(beeb_machine* machine, uint64_t maximum_cycles) {
    return machine && machine->value.runUntilFrame(maximum_cycles);
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
    if (machine && mono && sample_rate > 0) machine->value.sound().render(mono, frames, sample_rate);
}

void beeb_set_key(beeb_machine* machine, uint8_t column, uint8_t row, int pressed) {
    if (machine) machine->value.setKey(column, row, pressed != 0);
}

void beeb_set_break(beeb_machine* machine, int pressed) {
    if (machine) machine->value.setBreak(pressed != 0);
}

} // extern "C"
