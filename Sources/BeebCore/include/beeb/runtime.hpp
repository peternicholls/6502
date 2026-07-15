#pragma once

#include "beeb/cpu6502.hpp"
#include "beeb/disc_image.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace beeb {

/// Lifecycle state owned and changed only by a MachineRuntime owner thread.
enum class RuntimeState {
    paused,
    running,
    faulted,
    shuttingDown,
};

/// Stable outcome categories shared by the C++ and public host boundaries.
enum class RuntimeStatusCode {
    ok,
    invalidArgument,
    invalidState,
    executionFailed,
    resourceExhausted,
    unavailable,
    reentrantCall,
    internalFailure,
};

/// Operation-owned status with no borrowed diagnostic storage.
struct RuntimeStatus {
    RuntimeStatusCode code = RuntimeStatusCode::ok;
    std::string message;
    std::uint64_t acceptanceSequence = 0;

    [[nodiscard]] bool isOK() const noexcept { return code == RuntimeStatusCode::ok; }
    explicit operator bool() const noexcept { return isOK(); }

    friend bool operator==(const RuntimeStatus&, const RuntimeStatus&) = default;
};

/// Status plus an output that exists only for a successful operation.
template <typename T>
struct RuntimeResult {
    RuntimeStatus status;
    std::optional<T> value;
};

/// Caller-owned copy of the most recently completed frame.
struct OwnedFrame {
    bool available = false;
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    std::uint64_t number = 0;
    std::vector<std::uint8_t> rgba;

    friend bool operator==(const OwnedFrame&, const OwnedFrame&) = default;
};

/// Identity of a completed-instruction and fully advanced-device boundary.
struct SafePoint {
    std::uint64_t cpuCycles = 0;
    std::uint64_t frameNumber = 0;
    RuntimeState state = RuntimeState::paused;
    std::uint64_t ledgerSequence = 0;

    friend bool operator==(const SafePoint&, const SafePoint&) = default;
};

/// Stable query result for a contained execution failure.
struct RuntimeFault {
    bool available = false;
    std::string message;
    SafePoint safePoint;

    friend bool operator==(const RuntimeFault&, const RuntimeFault&) = default;
};

/// Commands serialized by one runtime owner.
enum class RuntimeCommandKind {
    start,
    pause,
    reset,
    runCycles,
    runUntilFrame,
    loadOSROM,
    loadSidewaysROM,
    mountDisc,
    setKey,
    setBreak,
    runtimeState,
    safePoint,
    fault,
    cpuState,
    frame,
    renderAudio,
    shutdown,
};

/// Whether one diagnostic ledger entry represents a command or execution slice.
enum class LedgerEventKind {
    command,
    executionSlice,
};

/// Opt-in in-memory evidence for accepted order and emulated execution work.
struct LedgerEntry {
    std::uint64_t sequence = 0;
    std::uint64_t acceptanceSequence = 0;
    LedgerEventKind event = LedgerEventKind::command;
    RuntimeCommandKind command = RuntimeCommandKind::runtimeState;
    std::uint64_t requestedCycles = 0;
    std::uint64_t actualCycles = 0;
    std::uint64_t payloadDigest = 0;
    RuntimeStatusCode status = RuntimeStatusCode::ok;
    SafePoint safePoint;

    friend bool operator==(const LedgerEntry&, const LedgerEntry&) = default;
};

/// Construction options; full command ledgers are disabled for normal hosts.
struct MachineRuntimeOptions {
    bool enableLedger = false;
};

/// Owns one BBCMicro and serializes all supported host access on one thread.
class MachineRuntime final {
public:
    static constexpr std::size_t commandCapacity = 64;
    static constexpr std::uint64_t executionSliceCycles = 2'048;

    explicit MachineRuntime(MachineRuntimeOptions options = {});
    ~MachineRuntime();

    MachineRuntime(const MachineRuntime&) = delete;
    MachineRuntime& operator=(const MachineRuntime&) = delete;
    MachineRuntime(MachineRuntime&&) = delete;
    MachineRuntime& operator=(MachineRuntime&&) = delete;

    [[nodiscard]] RuntimeResult<RuntimeState> state();
    [[nodiscard]] RuntimeStatus start();
    [[nodiscard]] RuntimeStatus pause();
    [[nodiscard]] RuntimeStatus reset();
    [[nodiscard]] RuntimeResult<std::uint64_t> runFor(std::uint64_t cycles);
    [[nodiscard]] RuntimeResult<bool> runUntilFrame(std::uint64_t maximumCycles);
    [[nodiscard]] RuntimeStatus loadOSROM(std::span<const std::uint8_t> rom);
    [[nodiscard]] RuntimeStatus loadSidewaysROM(
        std::uint8_t bank, std::span<const std::uint8_t> rom);
    [[nodiscard]] RuntimeStatus mountDisc(
        unsigned drive, std::span<const std::uint8_t> bytes,
        DiscImage::Layout layout, bool writable = false);
    [[nodiscard]] RuntimeStatus setKey(
        std::uint8_t column, std::uint8_t row, bool pressed);
    [[nodiscard]] RuntimeStatus setBreak(bool pressed);
    [[nodiscard]] RuntimeResult<CPUState> cpuState();
    [[nodiscard]] RuntimeResult<OwnedFrame> frame();
    [[nodiscard]] RuntimeResult<std::vector<float>> renderAudio(
        std::size_t frames, double sampleRate);
    [[nodiscard]] RuntimeResult<SafePoint> safePoint();
    [[nodiscard]] RuntimeResult<RuntimeFault> fault();

    /// Stops acceptance, drains accepted commands, joins the owner, and is idempotent.
    [[nodiscard]] RuntimeStatus shutdown() noexcept;

    /// Returns opt-in diagnostic history; normal runtimes return an empty vector.
    [[nodiscard]] std::vector<LedgerEntry> ledger() const;

    /// Returns the number of commands accepted by this runtime for test diagnostics.
    /// This value never reads machine state and is not a persisted replay format.
    [[nodiscard]] std::uint64_t acceptedCommandCount() const noexcept;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace beeb
