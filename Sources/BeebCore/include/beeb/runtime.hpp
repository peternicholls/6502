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
    RuntimeStatusCode code = RuntimeStatusCode::ok; ///< Stable category.
    std::string message;                  ///< Operation-owned diagnostic; empty for success.
    std::uint64_t acceptanceSequence = 0; ///< FIFO identity, or zero if never accepted.

    /// Reports whether this operation completed successfully.
    /// @return `true` only for `RuntimeStatusCode::ok`.
    [[nodiscard]] bool isOK() const noexcept { return code == RuntimeStatusCode::ok; }
    /// Provides explicit success testing without discarding the status value.
    /// @return The same result as isOK().
    explicit operator bool() const noexcept { return isOK(); }

    /// Compares category, diagnostic, and acceptance identity.
    friend bool operator==(const RuntimeStatus&, const RuntimeStatus&) = default;
};

/// Status plus an output that exists only for a successful operation.
template <typename T> struct RuntimeResult {
    RuntimeStatus status;   ///< Operation-owned outcome.
    std::optional<T> value; ///< Present only when status is successful.
};

/// Caller-owned copy of the most recently completed frame.
struct OwnedFrame {
    bool available = false;         ///< Whether one complete frame exists.
    std::uint32_t width = 0;        ///< Pixel width when available.
    std::uint32_t height = 0;       ///< Pixel height when available.
    std::uint64_t number = 0;       ///< CRTC frame identity.
    std::vector<std::uint8_t> rgba; ///< Packed 8-bit RGBA bytes owned by this value.

    /// Compares availability, metadata, and all owned RGBA bytes.
    friend bool operator==(const OwnedFrame&, const OwnedFrame&) = default;
};

/// Identity of a completed-instruction and fully advanced-device boundary.
struct SafePoint {
    std::uint64_t cpuCycles = 0;               ///< Completed CPU cycles.
    std::uint64_t frameNumber = 0;             ///< Latest completed frame identity.
    RuntimeState state = RuntimeState::paused; ///< Lifecycle state at this boundary.
    std::uint64_t ledgerSequence = 0;          ///< Latest total command/slice identity.

    /// Compares the complete externally observable safe-point identity.
    friend bool operator==(const SafePoint&, const SafePoint&) = default;
};

/// Stable query result for a contained execution failure.
struct RuntimeFault {
    bool available = false; ///< Whether the runtime is faulted.
    std::string message;    ///< Stable owned execution diagnostic.
    SafePoint safePoint;    ///< Last complete boundary retained with the fault.

    /// Compares availability, diagnostic, and contained safe point.
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
    std::uint64_t sequence = 0;                       ///< Total owner execution order.
    std::uint64_t acceptanceSequence = 0;             ///< FIFO identity; zero for internal slices.
    LedgerEventKind event = LedgerEventKind::command; ///< Command or execution slice.
    RuntimeCommandKind command = RuntimeCommandKind::runtimeState; ///< Command identity.
    std::uint64_t requestedCycles = 0;                ///< Emulated budget where applicable.
    std::uint64_t actualCycles = 0;                   ///< Whole-instruction work completed.
    std::uint64_t payloadDigest = 0;                  ///< Deterministic copied-input signature.
    std::uint64_t resultDigest = 0;                   ///< Deterministic owned-result signature.
    RuntimeStatusCode status = RuntimeStatusCode::ok; ///< Operation category.
    SafePoint safePoint;                              ///< Boundary after this event.

    /// Compares every recorded input, result, order, and safe-point field.
    friend bool operator==(const LedgerEntry&, const LedgerEntry&) = default;
};

/// Construction options; full command ledgers are disabled for normal hosts.
struct MachineRuntimeOptions {
    bool enableLedger = false; ///< Retain full in-memory test diagnostics when true.
};

/// Owns one BBCMicro and serializes all supported host access on one thread.
///
/// Calls are synchronous to owner-thread completion. Inputs are copied before
/// acceptance, outputs own their storage, and no machine or device reference
/// escapes. `docs/code/runtime-ownership.md` owns the lifecycle and ordering model.
/// Documentation rationale: that guide also owns safe-point and replay constraints.
class MachineRuntime final {
  public:
    /// Maximum accepted commands that may remain incomplete.
    static constexpr std::size_t commandCapacity = 64;
    /// Minimum whole-instruction budget selected during sustained execution.
    static constexpr std::uint64_t executionSliceCycles = 2'048;

    /// Creates a paused runtime and waits until its owner and machine are ready.
    /// @param options Opt-in diagnostic behavior; normal hosts use the default.
    /// @throws std::bad_alloc when owner or machine construction cannot allocate.
    explicit MachineRuntime(MachineRuntimeOptions options = {});
    /// Drains accepted work and joins the owner before releasing the machine.
    ~MachineRuntime();

    /// Runtime ownership is unique and cannot be copied.
    MachineRuntime(const MachineRuntime&) = delete;
    /// Runtime ownership cannot be reassigned by copying.
    MachineRuntime& operator=(const MachineRuntime&) = delete;
    /// A running owner cannot be moved to another object identity.
    MachineRuntime(MachineRuntime&&) = delete;
    /// Runtime ownership cannot be reassigned by moving.
    MachineRuntime& operator=(MachineRuntime&&) = delete;

    /// Queries lifecycle state in FIFO order at one safe point.
    /// @return Operation status and the state on success.
    [[nodiscard]] RuntimeResult<RuntimeState> state();
    /// Requests sustained execution; starting while running is idempotent.
    /// @return Operation-owned success or lifecycle failure.
    [[nodiscard]] RuntimeStatus start();
    /// Pauses sustained execution at the next owner safe point.
    /// @return Operation-owned success; pausing while paused is idempotent.
    [[nodiscard]] RuntimeStatus pause();
    /// Resets the machine, clears a runtime fault, and finishes paused.
    /// @return Operation-owned reset result.
    [[nodiscard]] RuntimeStatus reset();
    /// Executes a whole-instruction minimum budget while paused.
    /// @param cycles Minimum emulated CPU cycles; zero performs no work.
    /// @return Actual cycles on success, which may exceed the request by one instruction.
    [[nodiscard]] RuntimeResult<std::uint64_t> runFor(std::uint64_t cycles);
    /// Executes while paused until a frame completes or the budget is reached.
    /// @param maximumCycles Maximum emulated CPU-cycle budget.
    /// @return Whether a frame completed, or an operation failure.
    [[nodiscard]] RuntimeResult<bool> runUntilFrame(std::uint64_t maximumCycles);
    /// Copies and installs an exact 16 KiB OS ROM on the owner.
    /// @param rom Source bytes copied before the command can outlive the call.
    /// @return Validation, resource, lifecycle, or success status.
    [[nodiscard]] RuntimeStatus loadOSROM(std::span<const std::uint8_t> rom);
    /// Copies and installs one sideways-ROM bank on the owner.
    /// @param bank Bank in the inclusive range 0...15.
    /// @param rom Source bytes up to 16 KiB, copied before acceptance.
    /// @return Validation, resource, lifecycle, or success status.
    [[nodiscard]] RuntimeStatus loadSidewaysROM(std::uint8_t bank,
                                                std::span<const std::uint8_t> rom);
    /// Copies and mounts one validated DFS image on the owner.
    /// @param drive Drive 0 or 1.
    /// @param bytes Complete image bytes copied before acceptance.
    /// @param layout SSD or DSD byte ordering.
    /// @param writable Whether the runtime-owned copy permits writes.
    /// @return Validation, resource, lifecycle, or success status.
    [[nodiscard]] RuntimeStatus mountDisc(unsigned drive, std::span<const std::uint8_t> bytes,
                                          DiscImage::Layout layout, bool writable = false);
    /// Changes one keyboard-matrix bit in FIFO order.
    /// @param column Matrix column in the inclusive range 0...15.
    /// @param row Matrix row in the inclusive range 0...15.
    /// @param pressed Whether the key is pressed.
    /// @return Validation, lifecycle, or success status.
    [[nodiscard]] RuntimeStatus setKey(std::uint8_t column, std::uint8_t row, bool pressed);
    /// Changes BREAK state without inventing a start or pause transition.
    /// @param pressed Whether BREAK is held.
    /// @return Lifecycle or success status.
    [[nodiscard]] RuntimeStatus setBreak(bool pressed);
    /// Copies CPU registers and cycles from one owner safe point.
    /// @return An owned CPU state on success.
    [[nodiscard]] RuntimeResult<CPUState> cpuState();
    /// Copies the latest frame and its RGBA storage on the owner.
    /// @return An owned frame; `available` is false before the first frame.
    [[nodiscard]] RuntimeResult<OwnedFrame> frame();
    /// Renders owned mono samples on the owner because sampling mutates sound phase.
    /// @param frames Number of samples to return.
    /// @param sampleRate Finite positive sample rate in hertz.
    /// @return Owned samples or validation/lifecycle/resource failure.
    [[nodiscard]] RuntimeResult<std::vector<float>> renderAudio(std::size_t frames,
                                                                double sampleRate);
    /// Queries the current completed-instruction/device-tick identity in FIFO order.
    /// @return Safe-point identity on success.
    [[nodiscard]] RuntimeResult<SafePoint> safePoint();
    /// Queries the stable contained execution failure, if any.
    /// @return Owned fault detail; `available` is false outside the faulted state.
    [[nodiscard]] RuntimeResult<RuntimeFault> fault();

    /// Stops acceptance, drains accepted commands, joins the owner, and is idempotent.
    /// @return Success after join, or re-entrant/internal failure.
    [[nodiscard]] RuntimeStatus shutdown() noexcept;

    /// Returns opt-in diagnostic history; normal runtimes return an empty vector.
    /// @return An owned snapshot that never exposes machine storage.
    [[nodiscard]] std::vector<LedgerEntry> ledger() const;

    /// Returns the number of commands accepted by this runtime for test diagnostics.
    /// This value never reads machine state and is not a persisted replay format.
    /// @return Monotonic accepted-command count, including an accepted shutdown marker.
    [[nodiscard]] std::uint64_t acceptedCommandCount() const noexcept;

  private:
    /// Hides the machine, synchronization, queue, owner, and diagnostic ledger.
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace beeb
