#!/usr/bin/env bash

set -euo pipefail
source "$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/testlib.sh"

c2_prepare_paths
source_path="${c2_fixture_dir}/output-replay.cpp"
binary_path="${c2_build_dir}/output-replay"
cat >"${source_path}" <<'CPP'
#include "beeb/runtime.hpp"

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

} // namespace

int main() {
    const auto first = capture();
    const auto second = capture();
    if (first.samples.size() != beeb::audioSampleCapacity) return 1;
    if (first.format != beeb::OutputSampleFormat::monoFloat32) return 2;
    if (first.firstSample == 0 || first.firstSample != second.firstSample) return 3;
    return first.samples == second.samples ? 0 : 4;
}
CPP

"${c2_cxx}" "${c2_common_flags[@]}" \
    "${c2_core_sources[@]}" "${source_path}" -o "${binary_path}"
"${binary_path}"
