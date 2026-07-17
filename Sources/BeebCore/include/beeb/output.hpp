#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
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

/// Owned result of one FIFO audio drain, including a possible partial chunk.
struct AudioDrainResult {
    OutputStatus status; ///< Success, underrun, lifecycle, or failure outcome.
    AudioChunk chunk;    ///< Owned samples and exact request/shortfall accounting.
};

} // namespace beeb
