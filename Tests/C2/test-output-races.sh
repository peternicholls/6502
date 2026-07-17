#!/usr/bin/env bash

set -euo pipefail
source "$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/testlib.sh"

c2_prepare_paths
source_path="${c2_fixture_dir}/output-races.cpp"
binary_path="${c2_build_dir}/output-races"
cat >"${source_path}" <<'CPP'
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
    return rejected.status.code == beeb::RuntimeStatusCode::unavailable && !rejected.value ? 0 : 9;
}
CPP

"${c2_cxx}" "${c2_common_flags[@]}" \
    "${c2_core_sources[@]}" "${source_path}" -o "${binary_path}"
"${binary_path}"
