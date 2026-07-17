#!/usr/bin/env bash

set -euo pipefail
source "$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/testlib.sh"

c2_prepare_paths
source_path="${c2_fixture_dir}/output-races.cpp"
binary_path="${c2_build_dir}/output-races"
tsan_binary_path="${c2_build_dir}/output-races-tsan"
cat >"${source_path}" <<'CPP'
#include "beeb_c.h"
#include "beeb/runtime.hpp"

#include <array>
#include <atomic>
#include <cstdint>
#include <thread>
#include <vector>

namespace {

std::array<std::uint8_t, 0x4000> closedLoopROM() {
    std::array<std::uint8_t, 0x4000> rom{};
    rom.fill(0xEA);
    rom[0] = 0x4C;
    rom[1] = 0x00;
    rom[2] = 0xC0;
    rom[0x3FFC] = 0x00;
    rom[0x3FFD] = 0xC0;
    return rom;
}

bool valid(const beeb::OutputDiagnostics& value) {
    return value.frameCapacity == beeb::completedFrameCapacity &&
           value.audioCapacity == beeb::audioSampleCapacity &&
           value.frameDepth <= value.frameCapacity && value.audioDepth <= value.audioCapacity &&
           value.audioDemand ==
               (value.audioDepth < beeb::audioTargetDepth
                    ? beeb::audioTargetDepth - value.audioDepth
                    : 0) &&
           value.counters.framesProduced == value.counters.framesConsumed +
                                                value.counters.framesDropped + value.frameDepth &&
           value.counters.audioSamplesProduced ==
               value.counters.audioSamplesConsumed + value.counters.audioSamplesOverrun +
                   value.audioDepth;
}

bool valid(const beeb_output_diagnostics& value) {
    return value.frame_depth <= value.frame_capacity &&
           value.audio_depth <= value.audio_capacity &&
           value.frames_produced ==
               value.frames_consumed + value.frames_dropped + value.frame_depth &&
           value.audio_samples_produced == value.audio_samples_consumed +
                                               value.audio_samples_overrun + value.audio_depth;
}

/// Exercises the public C admission/serialization boundary with independent
/// caller-owned result storage on every producer and consumer thread.
bool runCBoundaryRace() {
    beeb_machine* machine = nullptr;
    if (beeb_create(&machine).code != BEEB_STATUS_OK || !machine) return false;
    const auto rom = closedLoopROM();
    if (beeb_load_os_rom(machine, rom.data(), rom.size()).code != BEEB_STATUS_OK ||
        beeb_reset(machine).code != BEEB_STATUS_OK) {
        (void)beeb_destroy(machine);
        return false;
    }

    std::atomic<unsigned> failures{0};
    std::jthread producer([&] {
        for (unsigned iteration = 0; iteration < 200; ++iteration) {
            std::uint64_t actual = 0;
            const auto result = beeb_run_cycles(machine, 2'048, &actual);
            if (result.code != BEEB_STATUS_OK || actual < 2'048) ++failures;
        }
    });
    std::array<std::jthread, 4> consumers;
    for (auto& consumer : consumers) {
        consumer = std::jthread([&] {
            std::uint64_t previousCycles = 0;
            for (unsigned iteration = 0; iteration < 200; ++iteration) {
                beeb_output_diagnostics observed{};
                if (beeb_get_output_diagnostics(machine, &observed).code != BEEB_STATUS_OK ||
                    !valid(observed) || observed.total_cycles < previousCycles) {
                    ++failures;
                } else {
                    previousCycles = observed.total_cycles;
                }

                std::array<float, 64> audio{};
                beeb_audio_drain_result drained{};
                const auto audioStatus = beeb_drain_audio(machine, audio.data(), audio.size(),
                                                          &drained);
                if ((audioStatus.code != BEEB_STATUS_OK &&
                     audioStatus.code != BEEB_STATUS_UNDERRUN) ||
                    drained.copied + drained.shortfall != audio.size())
                    ++failures;

                beeb_frame frame{};
                const auto frameStatus = beeb_dequeue_frame(machine, &frame);
                if (frameStatus.code == BEEB_STATUS_OK) {
                    if (!frame.available || !frame.rgba ||
                        beeb_frame_release(&frame).code != BEEB_STATUS_OK)
                        ++failures;
                } else if (frameStatus.code != BEEB_STATUS_EMPTY) {
                    ++failures;
                }
            }
        });
    }
    producer.join();
    for (auto& consumer : consumers)
        consumer.join();

    beeb_output_diagnostics final{};
    const bool success = failures.load() == 0 &&
                         beeb_get_output_diagnostics(machine, &final).code == BEEB_STATUS_OK &&
                         valid(final);
    return beeb_destroy(machine).code == BEEB_STATUS_OK && success;
}

} // namespace

int main() {
    beeb::MachineRuntime runtime;
    if (!runtime.loadOSROM(closedLoopROM()).isOK() || !runtime.reset().isOK()) return 1;

    std::atomic<unsigned> failures{0};
    std::jthread producer([&] {
        for (unsigned iteration = 0; iteration < 200; ++iteration) {
            const auto result = runtime.runFor(2'048);
            if (!result.status.isOK() || !result.value || *result.value < 2'048) ++failures;
        }
    });

    std::array<std::jthread, 4> consumers;
    for (auto& consumer : consumers) {
        consumer = std::jthread([&] {
            std::uint64_t previousCycles = 0;
            beeb::OutputCounters previousCounters;
            for (unsigned iteration = 0; iteration < 200; ++iteration) {
                const auto observed = runtime.outputDiagnostics();
                if (!observed.status.isOK() || !observed.value || !valid(*observed.value)) {
                    ++failures;
                    continue;
                }
                const auto& value = *observed.value;
                if (value.totalCycles < previousCycles ||
                    value.counters.framesProduced < previousCounters.framesProduced ||
                    value.counters.framesConsumed < previousCounters.framesConsumed ||
                    value.counters.framesDropped < previousCounters.framesDropped ||
                    value.counters.audioSamplesProduced < previousCounters.audioSamplesProduced ||
                    value.counters.audioSamplesConsumed < previousCounters.audioSamplesConsumed ||
                    value.counters.audioSamplesOverrun < previousCounters.audioSamplesOverrun ||
                    value.counters.audioSamplesUnderrun < previousCounters.audioSamplesUnderrun)
                    ++failures;
                previousCycles = value.totalCycles;
                previousCounters = value.counters;

                const auto drained = runtime.drainAudio(64);
                if (drained.status.code != beeb::OutputStatusCode::ok &&
                    drained.status.code != beeb::OutputStatusCode::underrun)
                    ++failures;
            }
        });
    }

    producer.join();
    for (auto& consumer : consumers)
        consumer.join();
    if (failures.load() != 0) return 2;

    const auto paused = runtime.outputDiagnostics();
    if (!paused.status.isOK() || !paused.value || !valid(*paused.value)) return 3;
    if (!runtime.start().isOK()) return 4;
    const auto running = runtime.outputDiagnostics();
    if (!running.status.isOK() || !running.value || !valid(*running.value) ||
        running.value->totalCycles < paused.value->totalCycles)
        return 5;
    if (!runtime.pause().isOK()) return 6;
    const auto repaused = runtime.outputDiagnostics();
    if (!repaused.status.isOK() || !repaused.value || !valid(*repaused.value) ||
        repaused.value->totalCycles < running.value->totalCycles)
        return 7;
    if (!runtime.shutdown().isOK()) return 8;

    const auto rejected = runtime.outputDiagnostics();
    if (rejected.status.code != beeb::RuntimeStatusCode::unavailable || rejected.value) return 9;
    return runCBoundaryRace() ? 0 : 10;
}
CPP

c2_tsan_supported() {
    # A successful link is not enough on hosts where the TSan runtime cannot
    # reserve its shadow address space, so support is established by execution.
    local probe_source="${c2_build_dir}/tsan-probe.cpp"
    local probe_binary="${c2_build_dir}/tsan-probe"
    printf '%s\n' \
        '#include <thread>' \
        'int main() { int value = 0; std::thread worker([&] { value = 1; }); worker.join(); return value == 1 ? 0 : 1; }' \
        >"${probe_source}"
    if ! "${c2_cxx}" -std=c++20 -fsanitize=thread -pthread \
        "${probe_source}" -o "${probe_binary}" >/dev/null 2>&1; then
        return 1
    fi
    sh -c 'TSAN_OPTIONS="halt_on_error=1:exitcode=66" "$1" >/dev/null 2>&1' \
        c2-tsan-probe "${probe_binary}" >/dev/null 2>&1
}

if [[ "${C2_ONLY_TSAN:-0}" != "1" ]]; then
    "${c2_cxx}" "${c2_common_flags[@]}" \
        "${c2_core_sources[@]}" "${source_path}" -o "${binary_path}"
    "${binary_path}"
fi

if c2_tsan_supported; then
    "${c2_cxx}" "${c2_common_flags[@]}" -fsanitize=thread -fno-omit-frame-pointer \
        "${c2_core_sources[@]}" "${source_path}" -o "${tsan_binary_path}"
    TSAN_OPTIONS="halt_on_error=1:exitcode=66" "${tsan_binary_path}"
elif [[ "${C2_REQUIRE_TSAN:-0}" == "1" ]]; then
    printf 'ERROR: C2 ThreadSanitizer is required but is not supported by %s on this host\n' \
        "${c2_cxx}" >&2
    exit 1
else
    printf 'N/A: C2 ThreadSanitizer is not supported by %s on this host\n' "${c2_cxx}"
fi
