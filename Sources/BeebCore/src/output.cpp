#include "beeb/output.hpp"

// C0-DOC-RATIONALE: docs/code/bounded-output.md owns the queue mutation,
// conservation equations, pressure policy, and owned-result rationale.

#include <algorithm>
#include <limits>
#include <type_traits>
#include <utility>

namespace beeb {
namespace {

static_assert(std::is_nothrow_move_constructible_v<CompletedFrame>);
static_assert(std::is_nothrow_move_assignable_v<CompletedFrame>);

OutputStatus outputStatus(OutputStatusCode code) noexcept {
    OutputStatus result;
    result.code = code;
    return result;
}

bool validFrame(const CompletedFrame& frame) noexcept {
    if (frame.width == 0 || frame.height == 0 || frame.format != OutputPixelFormat::rgba8)
        return false;
    const auto width = static_cast<std::size_t>(frame.width);
    const auto height = static_cast<std::size_t>(frame.height);
    if (width > std::numeric_limits<std::size_t>::max() / height) return false;
    const auto pixels = width * height;
    if (pixels > std::numeric_limits<std::size_t>::max() / 4) return false;
    return frame.rgba.size() == pixels * 4;
}

} // namespace

OutputStatus CompletedFrameQueue::publish(CompletedFrame frame) noexcept {
    if (!validFrame(frame) || (lastPublished_ && frame.number <= *lastPublished_))
        return outputStatus(OutputStatusCode::invalidArgument);

    const bool overflow = size_ == completedFrameCapacity;
    if (overflow) {
        slots_[head_] = std::move(frame);
        head_ = (head_ + 1) % completedFrameCapacity;
        ++counters_.framesDropped;
    } else {
        const auto tail = (head_ + size_) % completedFrameCapacity;
        slots_[tail].emplace(std::move(frame));
        ++size_;
    }
    const auto newest = (head_ + size_ - 1) % completedFrameCapacity;
    lastPublished_ = slots_[newest]->number;
    ++counters_.framesProduced;
    return outputStatus(overflow ? OutputStatusCode::overrun : OutputStatusCode::ok);
}

FrameDequeueResult CompletedFrameQueue::dequeue() noexcept {
    FrameDequeueResult result;
    if (size_ == 0) {
        result.status.code = OutputStatusCode::empty;
        return result;
    }

    result.frame.emplace(std::move(*slots_[head_]));
    slots_[head_].reset();
    head_ = (head_ + 1) % completedFrameCapacity;
    --size_;
    ++counters_.framesConsumed;
    return result;
}

void CompletedFrameQueue::discardRetained() noexcept {
    counters_.framesDropped += size_;
    for (auto& slot : slots_)
        slot.reset();
    head_ = 0;
    size_ = 0;
}

std::uint64_t CompletedFrameQueue::latestFrameNumber() const noexcept {
    return lastPublished_.value_or(0);
}

OutputStatus AudioSampleQueue::publish(std::span<const float> samples) noexcept {
    bool overflow = false;
    for (const auto sample : samples) {
        if (size_ == 0) oldestSequence_ = nextSequence_;
        if (size_ == audioSampleCapacity) {
            samples_[head_] = sample;
            head_ = (head_ + 1) % audioSampleCapacity;
            ++oldestSequence_;
            ++counters_.audioSamplesOverrun;
            overflow = true;
        } else {
            const auto tail = (head_ + size_) % audioSampleCapacity;
            samples_[tail] = sample;
            ++size_;
        }
        ++nextSequence_;
        ++counters_.audioSamplesProduced;
    }
    return outputStatus(overflow ? OutputStatusCode::overrun : OutputStatusCode::ok);
}

AudioDrainResult AudioSampleQueue::drain(std::size_t maximumSamples) noexcept {
    AudioDrainResult result;
    result.chunk.requested = maximumSamples;
    result.chunk.firstSample = oldestSequence_;
    const auto copied = std::min(maximumSamples, size_);
    try {
        result.chunk.samples.resize(copied);
    } catch (...) {
        result.status.code = OutputStatusCode::resourceExhausted;
        result.demand = demand();
        result.counters = counters_;
        return result;
    }
    for (std::size_t index = 0; index < copied; ++index)
        result.chunk.samples[index] = samples_[(head_ + index) % audioSampleCapacity];

    head_ = (head_ + copied) % audioSampleCapacity;
    size_ -= copied;
    oldestSequence_ += copied;
    counters_.audioSamplesConsumed += copied;
    result.chunk.shortfall = maximumSamples - copied;
    counters_.audioSamplesUnderrun += result.chunk.shortfall;
    result.status.code =
        result.chunk.shortfall == 0 ? OutputStatusCode::ok : OutputStatusCode::underrun;
    result.demand = demand();
    result.counters = counters_;
    return result;
}

void AudioSampleQueue::discardRetained() noexcept {
    counters_.audioSamplesOverrun += size_;
    head_ = 0;
    size_ = 0;
    oldestSequence_ = nextSequence_;
}

std::size_t AudioSampleQueue::demand() const noexcept {
    return size_ < audioTargetDepth ? audioTargetDepth - size_ : 0;
}

OutputDiagnostics captureOutputDiagnostics(std::uint64_t totalCycles,
                                           const CompletedFrameQueue& frames,
                                           const AudioSampleQueue& audio,
                                           OutputStatusCode lastStatus) noexcept {
    OutputDiagnostics result;
    result.totalCycles = totalCycles;
    result.latestFrameNumber = frames.latestFrameNumber();
    result.frameDepth = frames.depth();
    result.audioDepth = audio.depth();
    result.audioDemand = audio.demand();

    const auto frameCounters = frames.counters();
    const auto audioCounters = audio.counters();
    result.counters.framesProduced = frameCounters.framesProduced;
    result.counters.framesConsumed = frameCounters.framesConsumed;
    result.counters.framesDropped = frameCounters.framesDropped;
    result.counters.audioSamplesProduced = audioCounters.audioSamplesProduced;
    result.counters.audioSamplesConsumed = audioCounters.audioSamplesConsumed;
    result.counters.audioSamplesOverrun = audioCounters.audioSamplesOverrun;
    result.counters.audioSamplesUnderrun = audioCounters.audioSamplesUnderrun;
    result.lastStatus = lastStatus;
    return result;
}

} // namespace beeb
