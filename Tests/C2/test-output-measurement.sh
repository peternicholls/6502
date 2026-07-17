#!/usr/bin/env bash

set -euo pipefail
source "$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/testlib.sh"

c2_prepare_paths
source_path="${c2_fixture_dir}/output-measurement.cpp"
binary_path="${c2_build_dir}/output-measurement"
record_path="${c2_measurement_dir}/latest.txt"

cat >"${source_path}" <<'CPP'
#include "beeb_c.h"
#include "beeb/output.hpp"
#include "beeb/runtime.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>
#include <sys/resource.h>
#include <vector>

namespace {

constexpr std::uint64_t cyclesPerSecond = 2'000'000;
constexpr std::uint64_t warmupSeconds = 10;
constexpr std::uint64_t measurementSeconds = 60;
constexpr std::size_t stressItems = 10'000;
constexpr std::uint64_t rssToleranceBytes = 16ULL * 1024ULL * 1024ULL;

int fail(int code, const std::string& message) {
    std::cerr << "C2 measurement failure " << code << ": " << message << '\n';
    return code;
}

std::uint64_t peakRSSBytes() {
    rusage usage{};
    if (getrusage(RUSAGE_SELF, &usage) != 0) return 0;
#if defined(__APPLE__)
    return static_cast<std::uint64_t>(usage.ru_maxrss);
#else
    return static_cast<std::uint64_t>(usage.ru_maxrss) * 1024ULL;
#endif
}

std::vector<std::uint8_t> measurementROM() {
    std::vector<std::uint8_t> rom(0x4000, 0xEA);
    std::size_t cursor = 0;
    const auto emit = [&](std::uint8_t byte) { rom[cursor++] = byte; };
    const auto store = [&](std::uint16_t address) {
        emit(0x8D);
        emit(static_cast<std::uint8_t>(address));
        emit(static_cast<std::uint8_t>(address >> 8));
    };
    const auto load = [&](std::uint8_t value) {
        emit(0xA9);
        emit(value);
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
    rom[0x3FFC] = 0x00;
    rom[0x3FFD] = 0xC0;
    return rom;
}

bool valid(const beeb::OutputDiagnostics& value) {
    return value.frameCapacity == beeb::completedFrameCapacity &&
           value.audioCapacity == beeb::audioSampleCapacity &&
           value.frameDepth <= value.frameCapacity && value.audioDepth <= value.audioCapacity &&
           value.counters.framesProduced == value.counters.framesConsumed +
                                                value.counters.framesDropped + value.frameDepth &&
           value.counters.audioSamplesProduced ==
               value.counters.audioSamplesConsumed + value.counters.audioSamplesOverrun +
                   value.audioDepth;
}

} // namespace

int main(int argc, char** argv) {
    if (argc != 2) return fail(1, "expected one evidence-record path");

    beeb::CompletedFrameQueue ownershipFrames;
    beeb::CompletedFrame firstOwnedFrame;
    for (std::size_t item = 1; item <= stressItems; ++item) {
        beeb::CompletedFrame frame;
        frame.number = item;
        frame.width = 1;
        frame.height = 1;
        frame.rgba = {static_cast<std::uint8_t>(item), 1, 2, 255};
        if (ownershipFrames.publish(std::move(frame)).code != beeb::OutputStatusCode::ok)
            return fail(2, "ownership frame publication failed");
        const auto drained = ownershipFrames.dequeue();
        if (drained.status.code != beeb::OutputStatusCode::ok || !drained.frame ||
            drained.frame->number != item || drained.frame->width != 1 ||
            drained.frame->height != 1 || drained.frame->rgba.size() != 4)
            return fail(3, "ownership frame drain was incomplete");
        if (item == 1) firstOwnedFrame = *drained.frame;
    }
    const auto ownershipCounters = ownershipFrames.counters();
    if (ownershipCounters.framesProduced != stressItems ||
        ownershipCounters.framesConsumed != stressItems ||
        ownershipCounters.framesDropped != 0 || ownershipFrames.depth() != 0)
        return fail(4, "10,000-frame ownership accounting did not balance");
    if (firstOwnedFrame.number != 1 ||
        firstOwnedFrame.rgba != std::vector<std::uint8_t>({1, 1, 2, 255}))
        return fail(5, "retained ownership frame changed");

    beeb::CompletedFrameQueue frameStress;
    beeb::AudioSampleQueue audioStress;
    std::size_t stressMaxFrameDepth = 0;
    std::size_t stressMaxAudioDepth = 0;
    for (std::size_t item = 1; item <= stressItems; ++item) {
        beeb::CompletedFrame frame;
        frame.number = item;
        frame.width = 1;
        frame.height = 1;
        frame.rgba = {static_cast<std::uint8_t>(item), 0, 0, 255};
        const auto frameStatus = frameStress.publish(std::move(frame));
        if (frameStatus.code != beeb::OutputStatusCode::ok &&
            frameStatus.code != beeb::OutputStatusCode::overrun)
            return fail(6, "frame stress publication failed");

        const std::array sample{static_cast<float>(item % 256)};
        const auto audioStatus = audioStress.publish(sample);
        if (audioStatus.code != beeb::OutputStatusCode::ok &&
            audioStatus.code != beeb::OutputStatusCode::overrun)
            return fail(7, "audio stress publication failed");

        stressMaxFrameDepth = std::max(stressMaxFrameDepth, frameStress.depth());
        stressMaxAudioDepth = std::max(stressMaxAudioDepth, audioStress.depth());
        if (item % 4 == 0) {
            const auto drained = frameStress.dequeue();
            if (drained.status.code != beeb::OutputStatusCode::ok || !drained.frame)
                return fail(8, "frame stress drain failed");
        }
        if (item % 5 == 0) {
            const auto drained = audioStress.drain(1);
            if (drained.status.code != beeb::OutputStatusCode::ok ||
                drained.chunk.samples.size() != 1)
                return fail(9, "audio stress drain failed");
        }
    }
    while (frameStress.depth() != 0) {
        const auto drained = frameStress.dequeue();
        if (drained.status.code != beeb::OutputStatusCode::ok || !drained.frame)
            return fail(10, "final frame stress drain failed");
    }
    const auto finalAudioStress = audioStress.drain(audioStress.depth());
    if (finalAudioStress.status.code != beeb::OutputStatusCode::ok)
        return fail(11, "final audio stress drain failed");
    const auto frameStressCounters = frameStress.counters();
    const auto audioStressCounters = audioStress.counters();
    if (frameStressCounters.framesProduced != stressItems ||
        frameStressCounters.framesProduced != frameStressCounters.framesConsumed +
                                                   frameStressCounters.framesDropped)
        return fail(12, "frame stress accounting did not balance");
    if (audioStressCounters.audioSamplesProduced != stressItems ||
        audioStressCounters.audioSamplesProduced != audioStressCounters.audioSamplesConsumed +
                                                         audioStressCounters.audioSamplesOverrun)
        return fail(13, "audio stress accounting did not balance");
    if (stressMaxFrameDepth != beeb::completedFrameCapacity ||
        stressMaxAudioDepth != beeb::audioSampleCapacity)
        return fail(14, "stress fixture did not reach both fixed capacities");

    beeb::MachineRuntime runtime;
    if (!runtime.loadOSROM(measurementROM()).isOK() || !runtime.reset().isOK())
        return fail(15, "measurement runtime setup failed");

    constexpr std::uint64_t chunkCycles = cyclesPerSecond;
    for (std::uint64_t second = 0; second < warmupSeconds; ++second) {
        const auto run = runtime.runFor(chunkCycles);
        if (!run.status.isOK() || !run.value || *run.value < chunkCycles)
            return fail(16, "warm-up execution failed");
        const auto frame = runtime.dequeueFrame();
        if (frame.status.code != beeb::OutputStatusCode::ok || !frame.frame)
            return fail(17, "warm-up frame drain failed");
        const auto audio = runtime.drainAudio(beeb::audioSampleCapacity);
        if (audio.status.code != beeb::OutputStatusCode::ok ||
            audio.chunk.samples.size() != beeb::audioSampleCapacity)
            return fail(18, "warm-up audio drain failed");
    }

    const auto retainedResult = runtime.dequeueFrame();
    if (retainedResult.status.code != beeb::OutputStatusCode::ok || !retainedResult.frame)
        return fail(19, "no retained frame after warm-up");
    const auto retainedFrame = *retainedResult.frame;
    const auto startResult = runtime.outputDiagnostics();
    if (!startResult.status.isOK() || !startResult.value || !valid(*startResult.value))
        return fail(20, "invalid measurement start diagnostics");
    const auto start = *startResult.value;
    const auto rssAfterWarmup = peakRSSBytes();
    if (rssAfterWarmup == 0) return fail(21, "could not read process RSS");

    std::size_t maximumFrameDepth = start.frameDepth;
    std::size_t maximumAudioDepth = start.audioDepth;
    for (std::uint64_t second = 0; second < measurementSeconds; ++second) {
        const auto run = runtime.runFor(chunkCycles);
        if (!run.status.isOK() || !run.value || *run.value < chunkCycles)
            return fail(22, "measurement execution failed");
        const auto observed = runtime.outputDiagnostics();
        if (!observed.status.isOK() || !observed.value || !valid(*observed.value))
            return fail(23, "measurement accounting did not balance");
        maximumFrameDepth = std::max(maximumFrameDepth, observed.value->frameDepth);
        maximumAudioDepth = std::max(maximumAudioDepth, observed.value->audioDepth);

        const auto frame = runtime.dequeueFrame();
        if (frame.status.code != beeb::OutputStatusCode::ok || !frame.frame)
            return fail(24, "measurement frame drain failed");
        const auto audio = runtime.drainAudio(beeb::audioSampleCapacity);
        if (audio.status.code != beeb::OutputStatusCode::ok ||
            audio.chunk.samples.size() != beeb::audioSampleCapacity)
            return fail(25, "measurement audio drain failed");
    }

    const auto endResult = runtime.outputDiagnostics();
    if (!endResult.status.isOK() || !endResult.value || !valid(*endResult.value))
        return fail(26, "invalid measurement end diagnostics");
    const auto end = *endResult.value;
    const auto measuredCycles = end.totalCycles - start.totalCycles;
    if (measuredCycles < measurementSeconds * cyclesPerSecond)
        return fail(27, "measured emulated duration was too short");
    if (maximumFrameDepth > beeb::completedFrameCapacity ||
        maximumAudioDepth > beeb::audioSampleCapacity)
        return fail(28, "runtime depth exceeded a fixed capacity");
    if (*retainedResult.frame != retainedFrame)
        return fail(29, "retained frame changed during sustained production");

    const auto rssAfterMeasurement = peakRSSBytes();
    if (rssAfterMeasurement == 0) return fail(30, "could not read final process RSS");
    const auto rssGrowth = rssAfterMeasurement > rssAfterWarmup
                               ? rssAfterMeasurement - rssAfterWarmup
                               : 0;
    if (rssGrowth > rssToleranceBytes)
        return fail(31, "RSS growth exceeded 16 MiB after warm-up");

    beeb_output_diagnostics rateBefore{};
    beeb_output_diagnostics rateAfter{};
    rateBefore.total_cycles = 2'000'000;
    rateAfter.total_cycles = 8'000'000;
    constexpr double hostSeconds = 1.5;
    constexpr double expectedRate = 2.0;
    double observedRate = 0.0;
    const auto observationBefore = runtime.outputDiagnostics();
    const auto rateStatus =
        beeb_calculate_emulation_rate(&rateBefore, &rateAfter, hostSeconds, &observedRate);
    const auto observationAfter = runtime.outputDiagnostics();
    if (rateStatus.code != BEEB_STATUS_OK || !observationBefore.status.isOK() ||
        !observationAfter.status.isOK() || !observationBefore.value ||
        !observationAfter.value || *observationBefore.value != *observationAfter.value)
        return fail(32, "rate observation changed runtime state");
    const auto rateError = std::abs(observedRate - expectedRate) / expectedRate;
    if (rateError > 0.001) return fail(33, "rate error exceeded 0.1 percent");

    std::ofstream record(argv[1]);
    if (!record) return fail(34, "could not open evidence record");
    record << "schema=beeb-c2-measurement-v1\n"
           << "warmup_seconds=" << warmupSeconds << '\n'
           << "measurement_seconds=" << measurementSeconds << '\n'
           << "measurement_cycles=" << measuredCycles << '\n'
           << "queue_stress_items=" << stressItems << '\n'
           << "ownership_frames_produced=" << ownershipCounters.framesProduced << '\n'
           << "ownership_frames_consumed=" << ownershipCounters.framesConsumed << '\n'
           << "ownership_frames_dropped=" << ownershipCounters.framesDropped << '\n'
           << "ownership_retained_frame_unchanged=true\n"
           << "stress_max_frame_depth=" << stressMaxFrameDepth << '\n'
           << "stress_max_audio_depth=" << stressMaxAudioDepth << '\n'
           << "stress_frames_produced=" << frameStressCounters.framesProduced << '\n'
           << "stress_frames_consumed=" << frameStressCounters.framesConsumed << '\n'
           << "stress_frames_dropped=" << frameStressCounters.framesDropped << '\n'
           << "stress_audio_produced=" << audioStressCounters.audioSamplesProduced << '\n'
           << "stress_audio_consumed=" << audioStressCounters.audioSamplesConsumed << '\n'
           << "stress_audio_overrun=" << audioStressCounters.audioSamplesOverrun << '\n'
           << "maximum_frame_depth=" << maximumFrameDepth << '\n'
           << "maximum_audio_depth=" << maximumAudioDepth << '\n'
           << "frames_produced=" << end.counters.framesProduced << '\n'
           << "frames_consumed=" << end.counters.framesConsumed << '\n'
           << "frames_dropped=" << end.counters.framesDropped << '\n'
           << "frames_retained=" << end.frameDepth << '\n'
           << "audio_samples_produced=" << end.counters.audioSamplesProduced << '\n'
           << "audio_samples_consumed=" << end.counters.audioSamplesConsumed << '\n'
           << "audio_samples_overrun=" << end.counters.audioSamplesOverrun << '\n'
           << "audio_samples_retained=" << end.audioDepth << '\n'
           << "rss_after_warmup_bytes=" << rssAfterWarmup << '\n'
           << "rss_after_measurement_bytes=" << rssAfterMeasurement << '\n'
           << "rss_growth_bytes=" << rssGrowth << '\n'
           << "rss_tolerance_bytes=" << rssToleranceBytes << '\n'
           << "expected_rate=" << expectedRate << '\n'
           << "observed_rate=" << observedRate << '\n'
           << "relative_rate_error=" << rateError << '\n'
           << "retained_frame_unchanged=true\n"
           << "valid=true\n";
    if (!record) return fail(35, "could not write evidence record");
    return runtime.shutdown().isOK() ? 0 : fail(36, "runtime shutdown failed");
}
CPP

"${c2_cxx}" "${c2_common_flags[@]}" -O2 \
    "${c2_core_sources[@]}" "${source_path}" -o "${binary_path}"
"${binary_path}" "${record_path}"

rg -q '^schema=beeb-c2-measurement-v1$' "${record_path}"
rg -q '^warmup_seconds=10$' "${record_path}"
rg -q '^measurement_seconds=60$' "${record_path}"
rg -q '^queue_stress_items=10000$' "${record_path}"
rg -q '^ownership_frames_produced=10000$' "${record_path}"
rg -q '^ownership_frames_consumed=10000$' "${record_path}"
rg -q '^ownership_frames_dropped=0$' "${record_path}"
rg -q '^ownership_retained_frame_unchanged=true$' "${record_path}"
rg -q '^stress_max_frame_depth=3$' "${record_path}"
rg -q '^stress_max_audio_depth=4096$' "${record_path}"
rg -q '^retained_frame_unchanged=true$' "${record_path}"
rg -q '^valid=true$' "${record_path}"
awk -F= '$1 == "rss_growth_bytes" { growth = $2 }
         $1 == "rss_tolerance_bytes" { tolerance = $2 }
         END { exit !(growth <= tolerance) }' "${record_path}"
awk -F= '$1 == "relative_rate_error" { exit !($2 <= 0.001) }' "${record_path}"
cat "${record_path}"
