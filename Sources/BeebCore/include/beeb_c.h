#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct beeb_machine beeb_machine;

typedef struct beeb_cpu_state {
    uint8_t a, x, y, sp, p;
    uint16_t pc;
    uint64_t cycles;
} beeb_cpu_state;

beeb_machine* beeb_create(void);
void beeb_destroy(beeb_machine* machine);
int beeb_load_os_rom(beeb_machine* machine, const uint8_t* bytes, size_t count);
int beeb_load_sideways_rom(beeb_machine* machine, uint8_t bank, const uint8_t* bytes, size_t count);
int beeb_mount_disc(beeb_machine* machine, unsigned drive, const uint8_t* bytes, size_t count,
                    int double_sided, int writable);
void beeb_reset(beeb_machine* machine);
uint64_t beeb_run_cycles(beeb_machine* machine, uint64_t cycles);
int beeb_run_until_frame(beeb_machine* machine, uint64_t maximum_cycles);
beeb_cpu_state beeb_get_cpu_state(const beeb_machine* machine);
const uint8_t* beeb_get_frame_rgba(const beeb_machine* machine, uint32_t* width, uint32_t* height, uint64_t* number);
void beeb_render_audio(beeb_machine* machine, float* mono, size_t frames, double sample_rate);
void beeb_set_key(beeb_machine* machine, uint8_t column, uint8_t row, int pressed);
void beeb_set_break(beeb_machine* machine, int pressed);

#ifdef __cplusplus
}
#endif
