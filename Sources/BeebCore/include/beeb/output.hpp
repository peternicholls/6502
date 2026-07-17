#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace beeb {

/// Maximum number of complete frames retained for a lagging consumer.
inline constexpr std::size_t completedFrameCapacity = 3;
/// Maximum number of mono samples retained by the continuous audio FIFO.
inline constexpr std::size_t audioSampleCapacity = 4'096;
/// Preferred audio fill level used to calculate recoverable demand.
inline constexpr std::size_t audioTargetDepth = 2'048;
/// Fixed sample rate of continuous C2 audio output in hertz.
inline constexpr std::uint32_t audioSampleRate = 48'000;
/// Fixed channel count of continuous C2 audio output.
inline constexpr std::uint32_t audioChannelCount = 1;

/// Pixel layout carried by a completed owned frame.
enum class OutputPixelFormat {
    unknown,
    rgba8, ///< Packed red, green, blue, and alpha bytes in display order.
};

/// Sample layout carried by an owned audio drain.
enum class OutputSampleFormat {
    unknown,
    monoFloat32, ///< One normalized 32-bit floating-point channel at 48 kHz.
};

/// Stable categories for bounded-output operations and recoverable pressure.
enum class OutputStatusCode {
    ok,
    empty,
    underrun,
    overrun,
    capacityExceeded,
    invalidArgument,
    invalidState,
    resourceExhausted,
    unavailable,
    productionFailed,
    internalFailure,
};

/// Operation-owned bounded-output status with no shared diagnostic storage.
struct OutputStatus {
    OutputStatusCode code = OutputStatusCode::ok; ///< Stable result category.
    std::string message;                          ///< Owned detail; empty on success.

    /// Reports whether the operation completed without pressure or failure.
    /// @return `true` only for `OutputStatusCode::ok`.
    [[nodiscard]] bool isOK() const noexcept { return code == OutputStatusCode::ok; }

    /// Compares the complete operation-owned outcome.
    friend bool operator==(const OutputStatus&, const OutputStatus&) = default;
};

/// Complete immutable-in-transit RGBA observation owned by its result value.
///
/// Bytes are packed row-major red, green, blue, alpha with one byte per
/// component and exactly `width * height * 4` bytes. The runtime-lifetime frame
/// number identifies safe-point publication and remains strictly increasing
/// across device reset. Moving or copying this value never exposes producer or
/// queue storage.
struct CompletedFrame {
    std::uint64_t number = 0;                            ///< Monotonic emulated frame identity.
    std::uint32_t width = 0;                             ///< Positive pixel width.
    std::uint32_t height = 0;                            ///< Positive pixel height.
    OutputPixelFormat format = OutputPixelFormat::rgba8; ///< Packed byte layout.
    std::vector<std::uint8_t> rgba; ///< Owned pixels; never aliases producer storage.

    /// Compares metadata and every owned pixel byte.
    friend bool operator==(const CompletedFrame&, const CompletedFrame&) = default;
};

/// Owned FIFO audio result and exact demand observed by one drain operation.
///
/// Samples are deterministic mono IEEE Float32 values at exactly 48,000 Hz.
/// `shortfall == requested - samples.size()` and may be nonzero only with a
/// structured underrun. Storage belongs exclusively to this value.
struct AudioChunk {
    std::uint64_t firstSample = 0; ///< Emulated sequence of the first returned sample.
    std::size_t requested = 0;     ///< Maximum samples requested by the consumer.
    std::size_t shortfall = 0;     ///< Requested samples not available for this drain.
    OutputSampleFormat format = OutputSampleFormat::monoFloat32; ///< Fixed C2 layout.
    std::vector<float> samples; ///< Owned FIFO samples; never aliases queue storage.

    /// Compares sequence, request accounting, format, and owned samples.
    friend bool operator==(const AudioChunk&, const AudioChunk&) = default;
};

/// Monotonic exact accounting for bounded frame and audio flow.
struct OutputCounters {
    std::uint64_t framesProduced = 0;       ///< Complete frames offered to the FIFO.
    std::uint64_t framesConsumed = 0;       ///< Complete frames transferred to consumers.
    std::uint64_t framesDropped = 0;        ///< Oldest frames discarded on overflow.
    std::uint64_t audioSamplesProduced = 0; ///< Samples offered to the audio FIFO.
    std::uint64_t audioSamplesConsumed = 0; ///< Samples transferred to consumers.
    std::uint64_t audioSamplesOverrun = 0;  ///< Oldest samples discarded on overflow.
    std::uint64_t audioSamplesUnderrun = 0; ///< Exact requested sample shortfall.

    /// Compares every monotonic accounting value.
    friend bool operator==(const OutputCounters&, const OutputCounters&) = default;
};

/// Consistent owned observation of emulated progress and output pressure.
struct OutputDiagnostics {
    std::uint64_t totalCycles = 0;       ///< Completed emulated CPU cycles.
    std::uint64_t latestFrameNumber = 0; ///< Latest complete emulated frame identity.
    std::size_t frameDepth = 0;          ///< Complete frames currently retained.
    std::size_t frameCapacity = completedFrameCapacity; ///< Fixed frame limit.
    std::size_t audioDepth = 0;                         ///< Samples currently retained.
    std::size_t audioCapacity = audioSampleCapacity;    ///< Fixed audio limit.
    std::size_t audioDemand = audioTargetDepth;         ///< Samples needed to reach target.
    OutputCounters counters; ///< Exact produced, consumed, dropped, and shortfall totals.
    OutputStatusCode lastStatus = OutputStatusCode::ok; ///< Latest recoverable condition.

    /// Compares the complete point-in-time diagnostic observation.
    friend bool operator==(const OutputDiagnostics&, const OutputDiagnostics&) = default;
};

/// Owned result of one oldest-first completed-frame dequeue.
struct FrameDequeueResult {
    OutputStatus status;                 ///< Empty, lifecycle, failure, or success outcome.
    std::optional<CompletedFrame> frame; ///< Present only when one frame transfers.
};

/// Owner-thread-only capacity-three FIFO for complete immutable-in-transit frames.
///
/// Publishing and dequeueing are serialized by `MachineRuntime`; this type has
/// no internal lock and exposes no callback or borrowed storage. Moving a frame
/// out leaves every retained consumer value independent of later publication.
/// Dequeue is oldest-first. Publishing at capacity replaces exactly the oldest
/// unconsumed frame, reports overrun, and increments `framesDropped`, preserving
/// `framesProduced == framesConsumed + framesDropped + depth()`.
class CompletedFrameQueue final {
  public:
    /// Validates and publishes a strictly newer complete frame.
    /// @param frame Owned RGBA value moved into bounded storage.
    /// @return Success, overrun when the oldest frame was dropped, or invalid input.
    [[nodiscard]] OutputStatus publish(CompletedFrame frame) noexcept;
    /// Transfers the oldest retained frame.
    /// @return An owned frame, or a structured empty result.
    [[nodiscard]] FrameDequeueResult dequeue() noexcept;
    /// Reports retained frame count.
    /// @return A value in the inclusive range zero to `completedFrameCapacity`.
    [[nodiscard]] std::size_t depth() const noexcept { return size_; }
    /// Copies exact frame-flow counters; audio fields remain unchanged.
    /// @return Monotonic produced, consumed, and dropped values.
    [[nodiscard]] OutputCounters counters() const noexcept { return counters_; }
    /// Reports the latest successfully published output identity.
    /// @return Latest identity, or zero before the first frame.
    [[nodiscard]] std::uint64_t latestFrameNumber() const noexcept;

  private:
    std::array<std::optional<CompletedFrame>, completedFrameCapacity> slots_;
    std::size_t head_ = 0;
    std::size_t size_ = 0;
    std::optional<std::uint64_t> lastPublished_;
    OutputCounters counters_;
};

/// Owned result of one FIFO audio drain, including a possible partial chunk.
struct AudioDrainResult {
    OutputStatus status; ///< Success, underrun, lifecycle, or failure outcome.
    AudioChunk chunk;    ///< Owned samples and exact request/shortfall accounting.
    std::size_t demand = audioTargetDepth; ///< Post-drain samples needed to reach target.
    OutputCounters counters;               ///< Exact pressure totals observed with this drain.
};

/// Owner-thread-only fixed ring for continuous mono Float32 output.
///
/// Publication and drain are serialized by `MachineRuntime`. Storage is fixed
/// at 4,096 samples and demand targets 2,048 retained samples. Overflow replaces
/// exactly the oldest samples and increments their exact count. Drains allocate
/// an owned result before mutating the ring so allocation failure consumes
/// nothing. At all times, produced equals consumed plus overrun plus depth.
class AudioSampleQueue final {
  public:
    /// Publishes ordered samples, dropping oldest retained values at capacity.
    /// @param samples Deterministic mono Float32 values in production order.
    /// @return Success, or overrun when one or more oldest samples were dropped.
    [[nodiscard]] OutputStatus publish(std::span<const float> samples) noexcept;
    /// Copies and consumes up to the requested number of FIFO samples.
    /// @param maximumSamples Maximum samples to return in owned storage.
    /// @return Owned samples, exact shortfall, post-drain demand, and counters.
    [[nodiscard]] AudioDrainResult drain(std::size_t maximumSamples) noexcept;
    /// Reports the fixed storage limit.
    /// @return `audioSampleCapacity`.
    [[nodiscard]] constexpr std::size_t capacity() const noexcept { return audioSampleCapacity; }
    /// Reports retained samples.
    /// @return A value from zero through `audioSampleCapacity`.
    [[nodiscard]] std::size_t depth() const noexcept { return size_; }
    /// Reports samples needed to reach the 2,048-sample target.
    /// @return `max(0, audioTargetDepth - depth())`.
    [[nodiscard]] std::size_t demand() const noexcept;
    /// Copies exact audio-flow counters; frame fields remain unchanged.
    /// @return Monotonic produced, consumed, overrun, and underrun totals.
    [[nodiscard]] OutputCounters counters() const noexcept { return counters_; }

  private:
    std::array<float, audioSampleCapacity> samples_{};
    std::size_t head_ = 0;
    std::size_t size_ = 0;
    std::uint64_t oldestSequence_ = 1;
    std::uint64_t nextSequence_ = 1;
    OutputCounters counters_;
};

} // namespace beeb
