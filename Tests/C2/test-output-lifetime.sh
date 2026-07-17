#!/usr/bin/env bash

set -euo pipefail
source "$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/testlib.sh"

c2_prepare_paths
source_path="${c2_fixture_dir}/output-lifetime.cpp"
binary_path="${c2_build_dir}/output-lifetime"
cat >"${source_path}" <<'CPP'
#include "beeb/runtime.hpp"

#include <chrono>
#include <cstdint>
#include <future>
#include <thread>
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

bool produceFrame(beeb::MachineRuntime& runtime) {
    const auto result = runtime.runUntilFrame(200'000);
    return result.status.isOK() && result.value.has_value() && *result.value;
}

} // namespace

int main() {
    beeb::MachineRuntime runtime;
    if (!runtime.loadOSROM(closedLoopROM()).isOK()) return 1;
    if (!produceFrame(runtime)) return 2;

    auto retained = runtime.dequeueFrame();
    if (retained.status.code != beeb::OutputStatusCode::ok || !retained.frame) return 3;
    const auto retainedNumber = retained.frame->number;
    const auto retainedPixels = retained.frame->rgba;

    auto producer = std::async(std::launch::async, [&runtime] {
        for (unsigned iteration = 0; iteration < 8; ++iteration) {
            if (!produceFrame(runtime)) return false;
        }
        return true;
    });

    std::size_t observed = 0;
    while (producer.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready) {
        auto result = runtime.dequeueFrame();
        if (result.status.code == beeb::OutputStatusCode::ok) {
            if (!result.frame || result.frame->number <= retainedNumber) return 4;
            ++observed;
        } else if (result.status.code != beeb::OutputStatusCode::empty) {
            return 5;
        }
        std::this_thread::yield();
    }
    if (!producer.get()) return 6;

    for (;;) {
        auto result = runtime.dequeueFrame();
        if (result.status.code == beeb::OutputStatusCode::empty) break;
        if (result.status.code != beeb::OutputStatusCode::ok || !result.frame) return 7;
        if (result.frame->number <= retainedNumber) return 8;
        ++observed;
    }

    if (observed == 0) return 9;
    if (retained.frame->number != retainedNumber || retained.frame->rgba != retainedPixels) return 10;
    return runtime.shutdown().isOK() ? 0 : 11;
}
CPP

"${c2_cxx}" "${c2_common_flags[@]}" \
    "${c2_core_sources[@]}" "${source_path}" -o "${binary_path}"
"${binary_path}"
