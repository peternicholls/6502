#!/usr/bin/env bash

set -euo pipefail
source "$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/testlib.sh"

c2_prepare_paths
probe="${c2_fixture_dir}/public-boundaries.cpp"
cat >"${probe}" <<'CPP'
#include "beeb/output.hpp"

#include <cstdint>
#include <optional>
#include <type_traits>
#include <vector>

static_assert(beeb::completedFrameCapacity == 3);
static_assert(beeb::audioSampleCapacity == 4'096);
static_assert(beeb::audioTargetDepth == 2'048);
static_assert(beeb::audioSampleRate == 48'000);
static_assert(beeb::audioChannelCount == 1);
static_assert(beeb::OutputPixelFormat::rgba8 != beeb::OutputPixelFormat{});
static_assert(beeb::OutputSampleFormat::monoFloat32 != beeb::OutputSampleFormat{});

static_assert(std::is_same_v<decltype(beeb::CompletedFrame::number), std::uint64_t>);
static_assert(std::is_same_v<decltype(beeb::CompletedFrame::rgba),
                             std::vector<std::uint8_t>>);
static_assert(std::is_same_v<decltype(beeb::AudioChunk::samples), std::vector<float>>);
static_assert(std::is_same_v<decltype(beeb::FrameDequeueResult::frame),
                             std::optional<beeb::CompletedFrame>>);
static_assert(std::is_same_v<decltype(beeb::OutputDiagnostics::counters),
                             beeb::OutputCounters>);

constexpr bool hasRequiredStatuses() {
    using Code = beeb::OutputStatusCode;
    return Code::ok != Code::empty && Code::empty != Code::underrun &&
           Code::capacityExceeded != Code::invalidArgument &&
           Code::invalidState != Code::resourceExhausted &&
           Code::unavailable != Code::productionFailed &&
           Code::productionFailed != Code::internalFailure;
}

static_assert(hasRequiredStatuses());

int main() {
    beeb::OutputCounters counters{};
    counters.framesProduced = 1;
    counters.framesConsumed = 1;
    counters.framesDropped = 1;
    counters.audioSamplesProduced = 1;
    counters.audioSamplesConsumed = 1;
    counters.audioSamplesOverrun = 1;
    counters.audioSamplesUnderrun = 1;
    return counters.framesProduced == 1 ? 0 : 1;
}
CPP

"${c2_cxx}" "${c2_common_flags[@]}" -fsyntax-only "${probe}"
