#!/usr/bin/env bash

set -euo pipefail
source "$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/testlib.sh"

c2_prepare_paths
source_path="${c2_fixture_dir}/output-contract.c"
object_path="${c2_build_dir}/output-contract.o"
binary_path="${c2_build_dir}/output-contract"
cat >"${source_path}" <<'C'
#include "beeb_c.h"

#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* Private failure-injection seam implemented by the C++ adapter test build. */
void beeb_test_fail_next_frame_storage_allocation(void);

static int expect(beeb_status status, beeb_status_code code) {
    return status.code == code && status.message[BEEB_STATUS_MESSAGE_CAPACITY - 1] == '\0';
}

static int produce_frame(beeb_machine* machine) {
    int completed = 0;
    return expect(beeb_run_until_frame(machine, 200000, &completed), BEEB_STATUS_OK) && completed;
}

static void emit(uint8_t* rom, size_t* cursor, uint8_t byte) {
    rom[(*cursor)++] = byte;
}

static void load_immediate(uint8_t* rom, size_t* cursor, uint8_t value) {
    emit(rom, cursor, 0xA9);
    emit(rom, cursor, value);
}

static void store_absolute(uint8_t* rom, size_t* cursor, uint16_t address) {
    emit(rom, cursor, 0x8D);
    emit(rom, cursor, (uint8_t)address);
    emit(rom, cursor, (uint8_t)(address >> 8));
}

static void set_crtc(uint8_t* rom, size_t* cursor, uint8_t reg, uint8_t value) {
    load_immediate(rom, cursor, reg);
    store_absolute(rom, cursor, 0xFE00);
    load_immediate(rom, cursor, value);
    store_absolute(rom, cursor, 0xFE01);
}

int main(void) {
    beeb_frame untouched = {0};
    untouched.width = 77;
    if (!expect(beeb_dequeue_frame(NULL, &untouched), BEEB_STATUS_INVALID_ARGUMENT)) return 1;
    if (untouched.width != 77) return 2;

    beeb_machine* machine = NULL;
    if (!expect(beeb_create(&machine), BEEB_STATUS_OK) || machine == NULL) return 3;
    if (!expect(beeb_dequeue_frame(machine, NULL), BEEB_STATUS_INVALID_ARGUMENT)) return 4;

    beeb_frame occupied = {0};
    occupied.release_context = &occupied;
    if (!expect(beeb_dequeue_frame(machine, &occupied), BEEB_STATUS_INVALID_ARGUMENT) ||
        occupied.release_context != &occupied)
        return 5;

    untouched.width = 77;
    if (!expect(beeb_dequeue_frame(machine, &untouched), BEEB_STATUS_EMPTY)) return 5;
    if (untouched.width != 77 || untouched.rgba != NULL) return 6;

    uint8_t rom[0x4000];
    memset(rom, 0xEA, sizeof(rom));
    size_t cursor = 0;
    set_crtc(rom, &cursor, 1, 1);
    set_crtc(rom, &cursor, 6, 1);
    set_crtc(rom, &cursor, 9, 0);
    set_crtc(rom, &cursor, 12, 0);
    set_crtc(rom, &cursor, 13, 0);
    load_immediate(rom, &cursor, 0x1C);
    store_absolute(rom, &cursor, 0xFE20);
    uint16_t idle = (uint16_t)(0xC000 + cursor);
    emit(rom, &cursor, 0x4C);
    emit(rom, &cursor, (uint8_t)idle);
    emit(rom, &cursor, (uint8_t)(idle >> 8));
    rom[0x3FFC] = 0x00;
    rom[0x3FFD] = 0xC0;
    if (!expect(beeb_load_os_rom(machine, rom, sizeof(rom)), BEEB_STATUS_OK)) return 7;
    if (!expect(beeb_reset(machine), BEEB_STATUS_OK)) return 8;
    if (!produce_frame(machine)) return 8;

    beeb_output_diagnostics before_frame_failure = {0};
    if (!expect(beeb_get_output_diagnostics(machine, &before_frame_failure), BEEB_STATUS_OK))
        return 9;
    beeb_frame failed_frame = {0};
    failed_frame.width = 77;
    beeb_test_fail_next_frame_storage_allocation();
    if (!expect(beeb_dequeue_frame(machine, &failed_frame), BEEB_STATUS_RESOURCE_EXHAUSTED))
        return 10;
    if (failed_frame.width != 77 || failed_frame.rgba != NULL) return 11;
    beeb_output_diagnostics after_frame_failure = {0};
    if (!expect(beeb_get_output_diagnostics(machine, &after_frame_failure), BEEB_STATUS_OK))
        return 12;
    if (after_frame_failure.frame_depth != before_frame_failure.frame_depth ||
        after_frame_failure.frames_consumed != before_frame_failure.frames_consumed)
        return 13;

    beeb_frame first = {0};
    if (!expect(beeb_dequeue_frame(machine, &first), BEEB_STATUS_OK)) return 14;
    if (!first.available || first.rgba == NULL || first.rgba_size == 0 ||
        first.release_context == NULL)
        return 15;
    beeb_output_diagnostics after_frame_transfer = {0};
    if (!expect(beeb_get_output_diagnostics(machine, &after_frame_transfer), BEEB_STATUS_OK) ||
        after_frame_transfer.frames_consumed != before_frame_failure.frames_consumed + 1 ||
        after_frame_transfer.frame_depth + 1 != before_frame_failure.frame_depth)
        return 16;
    uint8_t* retained = malloc(first.rgba_size);
    if (retained == NULL) return 11;
    memcpy(retained, first.rgba, first.rgba_size);

    if (!produce_frame(machine)) return 12;
    beeb_frame second = {0};
    if (!expect(beeb_dequeue_frame(machine, &second), BEEB_STATUS_OK)) return 13;
    if (!second.available || second.rgba == NULL || second.rgba == first.rgba) return 14;
    if (memcmp(first.rgba, retained, first.rgba_size) != 0) return 15;
    if (!expect(beeb_dequeue_frame(machine, &first), BEEB_STATUS_INVALID_ARGUMENT)) return 16;

    free(retained);
    if (!expect(beeb_frame_release(&first), BEEB_STATUS_OK)) return 17;
    if (first.available || first.rgba != NULL || first.rgba_size != 0 ||
        first.release_context != NULL)
        return 18;
    if (!expect(beeb_frame_release(&second), BEEB_STATUS_OK)) return 19;

    uint64_t actual_cycles = 0;
    if (!expect(beeb_run_cycles(machine, 2000000, &actual_cycles), BEEB_STATUS_OK)) return 20;
    float* audio = malloc(5000 * sizeof(float));
    if (audio == NULL) return 21;
    for (size_t index = 0; index < 5000; ++index) audio[index] = 99.0f;
    beeb_audio_drain_result audio_result = {0};
    audio_result.copied = 77;
    if (!expect(beeb_drain_audio(NULL, audio, 5000, &audio_result),
                BEEB_STATUS_INVALID_ARGUMENT))
        return 22;
    if (audio_result.copied != 77 || audio[0] != 99.0f) return 23;
    if (!expect(beeb_drain_audio(machine, NULL, 5000, &audio_result),
                BEEB_STATUS_INVALID_ARGUMENT))
        return 24;
    if (!expect(beeb_drain_audio(machine, audio, 5000, NULL), BEEB_STATUS_INVALID_ARGUMENT))
        return 25;

    memset(&audio_result, 0, sizeof(audio_result));
    if (!expect(beeb_drain_audio(machine, audio, 5000, &audio_result), BEEB_STATUS_UNDERRUN))
        return 26;
    if (audio_result.copied != 4096 || audio_result.shortfall != 904) return 27;
    if (audio_result.demand != 2048 || audio_result.overrun_count == 0) return 28;
    if (audio_result.underrun_count < audio_result.shortfall) return 29;
    if (audio[0] == 99.0f || audio[4095] == 99.0f || audio[4096] != 99.0f) return 30;

    const uint64_t underrun_before = audio_result.underrun_count;
    if (!expect(beeb_drain_audio(machine, audio, 1, &audio_result), BEEB_STATUS_UNDERRUN))
        return 31;
    if (audio_result.copied != 0 || audio_result.shortfall != 1 ||
        audio_result.underrun_count != underrun_before + 1)
        return 32;

    beeb_output_diagnostics untouched_diagnostics = {0};
    untouched_diagnostics.frame_depth = 77;
    if (!expect(beeb_get_output_diagnostics(NULL, &untouched_diagnostics),
                BEEB_STATUS_INVALID_ARGUMENT))
        return 33;
    if (untouched_diagnostics.frame_depth != 77) return 34;
    if (!expect(beeb_get_output_diagnostics(machine, NULL), BEEB_STATUS_INVALID_ARGUMENT))
        return 35;

    beeb_output_diagnostics diagnostics = {0};
    if (!expect(beeb_get_output_diagnostics(machine, &diagnostics), BEEB_STATUS_OK)) return 36;
    if (diagnostics.total_cycles == 0 || diagnostics.latest_frame_number < 2) return 37;
    if (diagnostics.frame_capacity != 3 || diagnostics.frame_depth > 3) return 38;
    if (diagnostics.audio_capacity != 4096 || diagnostics.audio_depth != 0 ||
        diagnostics.audio_demand != 2048)
        return 39;
    if (diagnostics.frames_produced != diagnostics.frames_consumed +
                                               diagnostics.frames_dropped +
                                               diagnostics.frame_depth)
        return 40;
    if (diagnostics.audio_samples_produced != diagnostics.audio_samples_consumed +
                                                     diagnostics.audio_samples_overrun +
                                                     diagnostics.audio_depth)
        return 41;
    if (diagnostics.audio_samples_overrun != audio_result.overrun_count ||
        diagnostics.audio_samples_underrun != audio_result.underrun_count ||
        diagnostics.last_status != BEEB_STATUS_UNDERRUN)
        return 42;

    beeb_output_diagnostics repeated = {0};
    if (!expect(beeb_get_output_diagnostics(machine, &repeated), BEEB_STATUS_OK) ||
        memcmp(&diagnostics, &repeated, sizeof(diagnostics)) != 0)
        return 43;

    beeb_output_diagnostics before = {0};
    beeb_output_diagnostics after = {0};
    before.total_cycles = 2000000;
    after.total_cycles = 6000000;
    double rate = 77.0;
    if (!expect(beeb_calculate_emulation_rate(&before, &after, 2.0, &rate), BEEB_STATUS_OK))
        return 44;
    if (fabs(rate - 1.0) > 0.001) return 45;

    const double invalid_intervals[] = {0.0, -1.0, NAN, INFINITY};
    for (size_t index = 0; index < sizeof(invalid_intervals) / sizeof(invalid_intervals[0]);
         ++index) {
        rate = 77.0;
        if (!expect(beeb_calculate_emulation_rate(&before, &after, invalid_intervals[index],
                                                  &rate),
                    BEEB_STATUS_INVALID_ARGUMENT) ||
            rate != 77.0)
            return 46;
    }
    rate = 77.0;
    if (!expect(beeb_calculate_emulation_rate(&after, &before, 1.0, &rate),
                BEEB_STATUS_INVALID_ARGUMENT) ||
        rate != 77.0)
        return 47;
    if (!expect(beeb_calculate_emulation_rate(NULL, &after, 1.0, &rate),
                BEEB_STATUS_INVALID_ARGUMENT) ||
        !expect(beeb_calculate_emulation_rate(&before, NULL, 1.0, &rate),
                BEEB_STATUS_INVALID_ARGUMENT) ||
        !expect(beeb_calculate_emulation_rate(&before, &after, 1.0, NULL),
                BEEB_STATUS_INVALID_ARGUMENT))
        return 48;

    free(audio);
    return expect(beeb_destroy(machine), BEEB_STATUS_OK) ? 0 : 49;
}
C

"${CC:-cc}" -std=c11 -Wall -Wextra -Wpedantic -Werror \
    "-I${repo_root}/Sources/BeebCore/include" -c "${source_path}" -o "${object_path}"
"${c2_cxx}" "${c2_common_flags[@]}" \
    "${c2_core_sources[@]}" "${object_path}" -o "${binary_path}"
"${binary_path}"
