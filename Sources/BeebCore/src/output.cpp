#include "beeb/output.hpp"

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

std::uint64_t CompletedFrameQueue::latestFrameNumber() const noexcept {
    return lastPublished_.value_or(0);
}

} // namespace beeb
