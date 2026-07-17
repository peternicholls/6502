#!/usr/bin/env bash

set -euo pipefail
source "$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/testlib.sh"

c2_prepare_paths
source_path="${c2_fixture_dir}/output-replay.cpp"
binary_path="${c2_build_dir}/output-replay"
cat >"${source_path}" <<'CPP'
#include "beeb_c.h"
#include "beeb/runtime.hpp"

#include <array>
#include <cstdint>
#include <vector>

namespace {

std::vector<std::uint8_t> closedLoopROM() {
    std::vector<std::uint8_t> rom(0x4000, 0xEA);
    rom[0] = 0x4C;
    rom[1] = 0x00;
    rom[2] = 0xC0;
    rom[0x3FFC] = 0x00;
    rom[0x3FFD] = 0xC0;
    return rom;
}

std::vector<std::uint8_t> outputROM() {
    auto rom = closedLoopROM();
    std::size_t cursor = 0;
    const auto emit = [&](std::uint8_t byte) { rom[cursor++] = byte; };
    const auto load = [&](std::uint8_t value) {
        emit(0xA9);
        emit(value);
    };
    const auto store = [&](std::uint16_t address) {
        emit(0x8D);
        emit(static_cast<std::uint8_t>(address));
        emit(static_cast<std::uint8_t>(address >> 8));
    };
    const auto setCRTC = [&](std::uint8_t reg, std::uint8_t value) {
        load(reg);
        store(0xFE00);
        load(value);
        store(0xFE01);
    };
    setCRTC(1, 1);
    setCRTC(6, 1);
    setCRTC(9, 0);
    setCRTC(12, 0);
    setCRTC(13, 0);
    load(0x1C);
    store(0xFE20);
    const auto idle = static_cast<std::uint16_t>(0xC000 + cursor);
    emit(0x4C);
    emit(static_cast<std::uint8_t>(idle));
    emit(static_cast<std::uint8_t>(idle >> 8));
    return rom;
}

beeb::AudioChunk capture() {
    beeb::MachineRuntime runtime;
    if (!runtime.loadOSROM(closedLoopROM()).isOK()) return {};
    if (!runtime.reset().isOK()) return {};
    for (unsigned slice = 0; slice < 60; ++slice) {
        const auto result = runtime.runFor(100'000);
        if (!result.status.isOK() || !result.value) return {};
    }

    auto full = runtime.drainAudio(beeb::audioSampleCapacity);
    if (full.status.code != beeb::OutputStatusCode::ok ||
        full.chunk.samples.size() != beeb::audioSampleCapacity || full.chunk.shortfall != 0)
        return {};
    const auto empty = runtime.drainAudio(1);
    if (empty.status.code != beeb::OutputStatusCode::underrun || empty.chunk.shortfall != 1 ||
        !empty.chunk.samples.empty())
        return {};
    return full.chunk;
}

/// Complete C-boundary replay observation with all borrowed C storage copied
/// before release so two independent runtimes can be compared exactly.
struct CReplay {
    bool valid = false;
    std::uint64_t frameNumber = 0;
    std::vector<std::uint8_t> rgba;
    std::uint64_t firstSample = 0;
    std::vector<float> samples;
    beeb_output_diagnostics diagnostics{};
};

CReplay captureC() {
    CReplay capture;
    beeb_machine* machine = nullptr;
    if (beeb_create(&machine).code != BEEB_STATUS_OK || !machine) return capture;
    const auto rom = outputROM();
    if (beeb_load_os_rom(machine, rom.data(), rom.size()).code != BEEB_STATUS_OK ||
        beeb_reset(machine).code != BEEB_STATUS_OK) {
        (void)beeb_destroy(machine);
        return capture;
    }
    for (unsigned frame = 0; frame < 6; ++frame) {
        int completed = 0;
        if (beeb_run_until_frame(machine, 200'000, &completed).code != BEEB_STATUS_OK ||
            !completed) {
            (void)beeb_destroy(machine);
            return capture;
        }
    }
    std::uint64_t actual = 0;
    if (beeb_run_cycles(machine, 2'000'000, &actual).code != BEEB_STATUS_OK) {
        (void)beeb_destroy(machine);
        return capture;
    }

    beeb_frame frame{};
    if (beeb_dequeue_frame(machine, &frame).code != BEEB_STATUS_OK || !frame.rgba) {
        (void)beeb_destroy(machine);
        return capture;
    }
    capture.frameNumber = frame.number;
    capture.rgba.assign(frame.rgba, frame.rgba + frame.rgba_size);
    (void)beeb_frame_release(&frame);

    capture.samples.resize(beeb::audioSampleCapacity);
    beeb_audio_drain_result audio{};
    if (beeb_drain_audio(machine, capture.samples.data(), capture.samples.size(), &audio).code !=
            BEEB_STATUS_OK ||
        audio.copied != capture.samples.size() || audio.shortfall != 0 ||
        beeb_get_output_diagnostics(machine, &capture.diagnostics).code != BEEB_STATUS_OK) {
        (void)beeb_destroy(machine);
        return capture;
    }
    capture.firstSample = audio.first_sample;
    capture.valid = beeb_destroy(machine).code == BEEB_STATUS_OK;
    return capture;
}

bool sameDiagnostics(const beeb_output_diagnostics& left,
                     const beeb_output_diagnostics& right) {
    return left.total_cycles == right.total_cycles &&
           left.latest_frame_number == right.latest_frame_number &&
           left.frame_depth == right.frame_depth && left.frame_capacity == right.frame_capacity &&
           left.audio_depth == right.audio_depth && left.audio_capacity == right.audio_capacity &&
           left.audio_demand == right.audio_demand &&
           left.frames_produced == right.frames_produced &&
           left.frames_consumed == right.frames_consumed &&
           left.frames_dropped == right.frames_dropped &&
           left.audio_samples_produced == right.audio_samples_produced &&
           left.audio_samples_consumed == right.audio_samples_consumed &&
           left.audio_samples_overrun == right.audio_samples_overrun &&
           left.audio_samples_underrun == right.audio_samples_underrun &&
           left.last_status == right.last_status;
}

} // namespace

int main() {
    const auto first = capture();
    const auto second = capture();
    if (first.samples.size() != beeb::audioSampleCapacity) return 1;
    if (first.format != beeb::OutputSampleFormat::monoFloat32) return 2;
    if (first.firstSample == 0 || first.firstSample != second.firstSample) return 3;
    if (first.samples != second.samples) return 4;

    const auto cFirst = captureC();
    const auto cSecond = captureC();
    if (!cFirst.valid || !cSecond.valid || cFirst.frameNumber == 0) return 5;
    if (cFirst.frameNumber != cSecond.frameNumber || cFirst.rgba != cSecond.rgba) return 6;
    if (cFirst.firstSample == 0 || cFirst.firstSample != cSecond.firstSample ||
        cFirst.samples != cSecond.samples)
        return 7;
    return sameDiagnostics(cFirst.diagnostics, cSecond.diagnostics) ? 0 : 8;
}
CPP

"${c2_cxx}" "${c2_common_flags[@]}" \
    "${c2_core_sources[@]}" "${source_path}" -o "${binary_path}"
"${binary_path}"
