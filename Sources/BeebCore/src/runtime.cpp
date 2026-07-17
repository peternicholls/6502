#include "beeb/runtime.hpp"

#include "beeb/machine.hpp"

#include <atomic>
#include <bit>
#include <cmath>
#include <condition_variable>
#include <deque>
#include <exception>
#include <future>
#include <mutex>
#include <new>
#include <thread>
#include <type_traits>
#include <utility>
#include <variant>

namespace beeb {
namespace {

// Documentation rationale: docs/code/runtime-ownership.md owns the queue,
// acceptance/completion, safe-point, execution-arbitration, and shutdown invariants.

RuntimeStatus status(RuntimeStatusCode code, std::string message = {},
                     std::uint64_t acceptanceSequence = 0) {
    return {code, std::move(message), acceptanceSequence};
}

RuntimeStatus allocationFailure(std::uint64_t acceptanceSequence = 0) noexcept {
    RuntimeStatus result;
    result.code = RuntimeStatusCode::resourceExhausted;
    result.acceptanceSequence = acceptanceSequence;
    return result;
}

std::uint64_t hashBytes(std::span<const std::uint8_t> bytes) noexcept {
    std::uint64_t hash = 14'695'981'039'346'656'037ULL;
    for (const auto byte : bytes) {
        hash ^= byte;
        hash *= 1'099'511'628'211ULL;
    }
    return hash;
}

std::uint64_t mix(std::uint64_t hash, std::uint64_t value) noexcept {
    hash ^= value + 0x9E3779B97F4A7C15ULL + (hash << 6) + (hash >> 2);
    return hash;
}

/// Copied sideways-ROM transaction retained until its owner command completes.
struct SidewaysPayload {
    std::uint8_t bank = 0;
    std::vector<std::uint8_t> bytes;
};

/// Copied disc transaction whose bytes cannot alias caller-owned storage.
struct DiscPayload {
    unsigned drive = 0;
    std::vector<std::uint8_t> bytes;
    DiscImage::Layout layout = DiscImage::Layout::SSD;
    bool writable = false;
};

/// Value-only keyboard mutation applied atomically by the owner.
struct KeyPayload {
    std::uint8_t column = 0;
    std::uint8_t row = 0;
    bool pressed = false;
};

/// Audio request metadata; the owner allocates and returns the sample storage.
struct AudioPayload {
    std::size_t frames = 0;
    double sampleRate = 0;
};

/// Closed payload vocabulary kept in the same queue node as its completion.
using CommandPayload = std::variant<std::monostate, std::uint64_t, std::vector<std::uint8_t>,
                                    SidewaysPayload, DiscPayload, KeyPayload, bool, AudioPayload>;

/// Closed owned-result vocabulary returned through one caller-specific promise.
using CompletionValue = std::variant<std::monostate, RuntimeState, std::uint64_t, bool, CPUState,
                                     OwnedFrame, std::vector<float>, FrameDequeueResult,
                                     AudioDrainResult, OutputDiagnostics, SafePoint, RuntimeFault>;

std::uint64_t hashString(const std::string& value) noexcept {
    return hashBytes(std::span(reinterpret_cast<const std::uint8_t*>(value.data()), value.size()));
}

std::uint64_t completionDigest(const CompletionValue& value) noexcept {
    return std::visit(
        [](const auto& result) -> std::uint64_t {
            using Result = std::decay_t<decltype(result)>;
            if constexpr (std::is_same_v<Result, std::monostate>) {
                return 0;
            } else if constexpr (std::is_same_v<Result, RuntimeState>) {
                return static_cast<std::uint64_t>(result) + 1;
            } else if constexpr (std::is_same_v<Result, std::uint64_t>) {
                return mix(1, result);
            } else if constexpr (std::is_same_v<Result, bool>) {
                return result ? 2 : 1;
            } else if constexpr (std::is_same_v<Result, CPUState>) {
                auto digest = mix(result.a, result.x);
                digest = mix(digest, result.y);
                digest = mix(digest, result.sp);
                digest = mix(digest, result.p);
                digest = mix(digest, result.pc);
                return mix(digest, result.cycles);
            } else if constexpr (std::is_same_v<Result, OwnedFrame>) {
                auto digest = mix(result.available ? 1 : 0, result.width);
                digest = mix(digest, result.height);
                digest = mix(digest, result.number);
                return mix(digest, hashBytes(result.rgba));
            } else if constexpr (std::is_same_v<Result, std::vector<float>>) {
                std::uint64_t digest = result.size();
                for (const auto sample : result) {
                    digest = mix(digest, std::bit_cast<std::uint32_t>(sample));
                }
                return digest;
            } else if constexpr (std::is_same_v<Result, FrameDequeueResult>) {
                auto digest = static_cast<std::uint64_t>(result.status.code);
                if (!result.frame) return digest;
                digest = mix(digest, result.frame->number);
                digest = mix(digest, result.frame->width);
                digest = mix(digest, result.frame->height);
                return mix(digest, hashBytes(result.frame->rgba));
            } else if constexpr (std::is_same_v<Result, AudioDrainResult>) {
                auto digest =
                    mix(static_cast<std::uint64_t>(result.status.code), result.chunk.firstSample);
                digest = mix(digest, result.chunk.requested);
                digest = mix(digest, result.chunk.shortfall);
                digest = mix(digest, result.demand);
                for (const auto sample : result.chunk.samples)
                    digest = mix(digest, std::bit_cast<std::uint32_t>(sample));
                return digest;
            } else if constexpr (std::is_same_v<Result, OutputDiagnostics>) {
                auto digest = mix(result.totalCycles, result.latestFrameNumber);
                digest = mix(digest, result.frameDepth);
                digest = mix(digest, result.frameCapacity);
                digest = mix(digest, result.audioDepth);
                digest = mix(digest, result.audioCapacity);
                digest = mix(digest, result.audioDemand);
                digest = mix(digest, result.counters.framesProduced);
                digest = mix(digest, result.counters.framesConsumed);
                digest = mix(digest, result.counters.framesDropped);
                digest = mix(digest, result.counters.audioSamplesProduced);
                digest = mix(digest, result.counters.audioSamplesConsumed);
                digest = mix(digest, result.counters.audioSamplesOverrun);
                digest = mix(digest, result.counters.audioSamplesUnderrun);
                return mix(digest, static_cast<std::uint64_t>(result.lastStatus));
            } else if constexpr (std::is_same_v<Result, SafePoint>) {
                auto digest = mix(result.cpuCycles, result.frameNumber);
                digest = mix(digest, static_cast<std::uint64_t>(result.state));
                return mix(digest, result.ledgerSequence);
            } else if constexpr (std::is_same_v<Result, RuntimeFault>) {
                auto digest = mix(result.available ? 1 : 0, hashString(result.message));
                digest = mix(digest, result.safePoint.cpuCycles);
                digest = mix(digest, result.safePoint.frameNumber);
                digest = mix(digest, static_cast<std::uint64_t>(result.safePoint.state));
                return mix(digest, result.safePoint.ledgerSequence);
            }
        },
        value);
}

/// One operation-scoped status and optional value, never shared between callers.
struct Completion {
    RuntimeStatus status;
    CompletionValue value;
    std::uint64_t retainedCycles = 0;
};

/// One accepted FIFO unit; payload, identity, digest, and promise move together.
struct Request {
    RuntimeCommandKind kind = RuntimeCommandKind::runtimeState;
    CommandPayload payload;
    std::uint64_t acceptanceSequence = 0;
    std::uint64_t payloadDigest = 0;
    std::promise<Completion> completion;
};

template <typename T> RuntimeResult<T> resultFromCompletion(Completion completion) {
    RuntimeResult<T> result;
    result.status = std::move(completion.status);
    if (!result.status.isOK()) return result;
    if (auto* value = std::get_if<T>(&completion.value)) {
        result.value = std::move(*value);
        return result;
    }
    result.status = status(RuntimeStatusCode::internalFailure,
                           "runtime command completed without its declared result",
                           result.status.acceptanceSequence);
    return result;
}

OutputStatusCode outputCode(RuntimeStatusCode code) noexcept {
    switch (code) {
    case RuntimeStatusCode::ok:
        return OutputStatusCode::ok;
    case RuntimeStatusCode::invalidArgument:
        return OutputStatusCode::invalidArgument;
    case RuntimeStatusCode::invalidState:
        return OutputStatusCode::invalidState;
    case RuntimeStatusCode::resourceExhausted:
        return OutputStatusCode::resourceExhausted;
    case RuntimeStatusCode::unavailable:
        return OutputStatusCode::unavailable;
    case RuntimeStatusCode::executionFailed:
        return OutputStatusCode::productionFailed;
    case RuntimeStatusCode::reentrantCall:
    case RuntimeStatusCode::internalFailure:
        return OutputStatusCode::internalFailure;
    }
    return OutputStatusCode::internalFailure;
}

FrameDequeueResult frameResultFromCompletion(Completion completion) {
    if (!completion.status.isOK()) {
        return {{outputCode(completion.status.code), std::move(completion.status.message)}, {}};
    }
    if (auto* value = std::get_if<FrameDequeueResult>(&completion.value)) return std::move(*value);
    return {{OutputStatusCode::internalFailure,
             "runtime command completed without its declared frame result"},
            {}};
}

AudioDrainResult audioResultFromCompletion(Completion completion) {
    if (!completion.status.isOK()) {
        AudioDrainResult result;
        result.status = {outputCode(completion.status.code), std::move(completion.status.message)};
        return result;
    }
    if (auto* value = std::get_if<AudioDrainResult>(&completion.value)) return std::move(*value);
    AudioDrainResult result;
    result.status = {OutputStatusCode::internalFailure,
                     "runtime command completed without its declared audio result"};
    return result;
}

} // namespace

/// Private runtime owner and the only collaborator permitted to dereference BBCMicro.
///
/// `mutex_` protects acceptance, capacity, queue, startup, and shutdown state.
/// Runtime lifecycle and `machine_` are otherwise owner-thread-only. The
/// diagnostic ledger has a separate lock so tests can copy it without entering
/// the command FIFO or granting access to machine state.
class MachineRuntime::Impl final {
  public:
    /// Starts the owner and blocks until BBCMicro construction succeeds or fails.
    /// @param options Controls opt-in in-memory ledger retention.
    explicit Impl(MachineRuntimeOptions options)
        : allocationFailurePoint_(options.failAllocationAt),
          testReentrantSubmission_(options.testReentrantSubmission),
          ledgerEnabled_(options.enableLedger), owner_([this] { ownerLoop(); }) {
        std::unique_lock lock(mutex_);
        stateChanged_.wait(lock, [this] { return ready_; });
        if (startupError_) std::rethrow_exception(startupError_);
    }

    ~Impl() = default;

    /// Applies capacity back-pressure, atomically accepts one node, and waits for it.
    /// @param kind Command identity interpreted only by the owner.
    /// @param payload Copied/value payload moved into the queue node.
    /// @param payloadDigest Deterministic signature of the copied input.
    /// @return Caller-specific status and optional owned result.
    Completion submit(RuntimeCommandKind kind, CommandPayload payload = {},
                      std::uint64_t payloadDigest = 0) {
        {
            std::lock_guard lock(mutex_);
            if (std::this_thread::get_id() == ownerId_) {
                return {status(RuntimeStatusCode::reentrantCall,
                               "runtime commands cannot be submitted by the owner thread"),
                        {}};
            }
        }

        if (shouldFail(RuntimeAllocationFailurePoint::request)) return {allocationFailure(), {}};

        std::shared_ptr<Request> request;
        std::future<Completion> future;
        try {
            request = std::make_shared<Request>();
            request->kind = kind;
            request->payload = std::move(payload);
            request->payloadDigest = payloadDigest;
            future = request->completion.get_future();
        } catch (const std::bad_alloc&) {
            return {allocationFailure(), {}};
        } catch (const std::exception& error) {
            return {status(RuntimeStatusCode::internalFailure, error.what()), {}};
        }

        {
            std::unique_lock lock(mutex_);
            capacityChanged_.wait(lock, [this] {
                return !accepting_ || incomplete_ < MachineRuntime::commandCapacity;
            });
            if (!accepting_) {
                return {status(RuntimeStatusCode::unavailable, "runtime is shutting down"), {}};
            }
            if (shouldFail(RuntimeAllocationFailurePoint::queue)) return {allocationFailure(), {}};
            try {
                queue_.push_back(request);
            } catch (const std::bad_alloc&) {
                return {allocationFailure(), {}};
            } catch (const std::exception& error) {
                return {status(RuntimeStatusCode::internalFailure, error.what()), {}};
            }
            request->acceptanceSequence = nextAcceptanceSequence_++;
            ++incomplete_;
        }
        workAvailable_.notify_one();

        try {
            return future.get();
        } catch (const std::exception& error) {
            return {status(RuntimeStatusCode::internalFailure, error.what()), {}};
        }
    }

    /// Coordinates one drain marker and one join across concurrent shutdown callers.
    /// @return Success after owner exit, or re-entrant/internal failure.
    RuntimeStatus shutdown() noexcept {
        std::uint64_t acceptanceSequence = 0;
        try {
            if (shouldFail(RuntimeAllocationFailurePoint::shutdownResult)) throw std::bad_alloc{};
            std::unique_lock lock(mutex_);
            if (std::this_thread::get_id() == ownerId_) {
                return status(RuntimeStatusCode::reentrantCall, "runtime owner cannot join itself");
            }
            if (shutdownComplete_) return status(RuntimeStatusCode::ok);
            if (shutdownStarted_) {
                stateChanged_.wait(lock, [this] { return shutdownComplete_; });
                return status(RuntimeStatusCode::ok);
            }

            shutdownStarted_ = true;
            accepting_ = false;
            capacityChanged_.notify_all();
            workAvailable_.notify_one();

            capacityChanged_.wait(lock,
                                  [this] { return incomplete_ < MachineRuntime::commandCapacity; });
            acceptanceSequence = nextAcceptanceSequence_++;
            shutdownAcceptanceSequence_ = acceptanceSequence;
            ++incomplete_;
            shutdownMarkerReady_ = true;
            workAvailable_.notify_one();

            stateChanged_.wait(lock, [this] { return ownerExited_; });
            lock.unlock();
            if (owner_.joinable()) owner_.join();
            lock.lock();
            shutdownComplete_ = true;
            lock.unlock();
            stateChanged_.notify_all();
            return status(RuntimeStatusCode::ok, {}, acceptanceSequence);
        } catch (const std::bad_alloc&) {
            return allocationFailure(acceptanceSequence);
        } catch (...) {
            RuntimeStatus result;
            result.code = RuntimeStatusCode::internalFailure;
            result.acceptanceSequence = acceptanceSequence;
            return result;
        }
    }

    /// Copies the opt-in ledger without accessing BBCMicro.
    /// @return Owned entries, or an empty vector when capture is disabled.
    std::vector<LedgerEntry> ledger() const {
        if (!ledgerEnabled_) return {};
        std::lock_guard lock(ledgerMutex_);
        return ledger_;
    }

    /// Reads acceptance progress under the queue mutex for latency tests.
    /// @return Count of commands that received an acceptance identity.
    std::uint64_t acceptedCommandCount() const noexcept {
        try {
            std::lock_guard lock(mutex_);
            return nextAcceptanceSequence_ - 1;
        } catch (...) {
            return 0;
        }
    }

  private:
    mutable std::mutex mutex_;
    std::condition_variable workAvailable_;
    std::condition_variable capacityChanged_;
    std::condition_variable stateChanged_;
    std::deque<std::shared_ptr<Request>> queue_;
    std::size_t incomplete_ = 0;
    std::uint64_t nextAcceptanceSequence_ = 1;
    std::uint64_t shutdownAcceptanceSequence_ = 0;
    bool accepting_ = true;
    bool shutdownStarted_ = false;
    bool shutdownMarkerReady_ = false;
    bool shutdownComplete_ = false;
    bool ownerExited_ = false;
    bool ready_ = false;
    std::thread::id ownerId_;
    std::exception_ptr startupError_;

    RuntimeAllocationFailurePoint allocationFailurePoint_ = RuntimeAllocationFailurePoint::none;
    std::atomic<bool> allocationFailureConsumed_{false};
    bool testReentrantSubmission_ = false;
    bool testReentrantSubmissionConsumed_ = false;

    RuntimeState runtimeState_ = RuntimeState::paused;
    std::string faultMessage_;
    std::unique_ptr<BBCMicro> machine_;
    CompletedFrameQueue frameOutput_;
    AudioSampleQueue audioOutput_;
    std::uint64_t nextOutputFrameNumber_ = 1;
    std::uint64_t audioCycleRemainder_ = 0;
    OutputStatusCode lastOutputStatus_ = OutputStatusCode::ok;

    bool ledgerEnabled_ = false;
    mutable std::mutex ledgerMutex_;
    std::vector<LedgerEntry> ledger_;
    std::uint64_t nextLedgerSequence_ = 1;
    bool ledgerAllocationFailed_ = false;

    std::jthread owner_;

    bool shouldFail(RuntimeAllocationFailurePoint point) noexcept {
        if (allocationFailurePoint_ != point) return false;
        bool expected = false;
        return allocationFailureConsumed_.compare_exchange_strong(expected, true);
    }

    void ownerLoop() noexcept {
        {
            std::lock_guard lock(mutex_);
            ownerId_ = std::this_thread::get_id();
        }
        try {
            machine_ = std::make_unique<BBCMicro>();
        } catch (...) {
            std::lock_guard lock(mutex_);
            startupError_ = std::current_exception();
            accepting_ = false;
            ready_ = true;
            ownerExited_ = true;
            stateChanged_.notify_all();
            capacityChanged_.notify_all();
            return;
        }

        {
            std::lock_guard lock(mutex_);
            ready_ = true;
        }
        stateChanged_.notify_all();

        for (;;) {
            std::shared_ptr<Request> request;
            bool executeSlice = false;
            bool processShutdown = false;
            {
                std::unique_lock lock(mutex_);
                workAvailable_.wait(lock, [this] {
                    return !queue_.empty() || shutdownMarkerReady_ ||
                           (runtimeState_ == RuntimeState::running && !shutdownStarted_);
                });
                if (!queue_.empty()) {
                    request = std::move(queue_.front());
                    queue_.pop_front();
                } else if (shutdownMarkerReady_) {
                    processShutdown = true;
                } else {
                    executeSlice = true;
                }
            }

            if (request) {
                completeRequest(*request);
                continue;
            }
            if (executeSlice) {
                executeRunningSlice();
                continue;
            }
            if (processShutdown) {
                runtimeState_ = RuntimeState::shuttingDown;
                const auto sequence = nextLedgerSequence_++;
                const auto safePoint = currentSafePoint(sequence);
                appendLedger({sequence, shutdownAcceptanceSequence_, LedgerEventKind::command,
                              RuntimeCommandKind::shutdown, 0, 0, 0, 0, RuntimeStatusCode::ok,
                              safePoint});
                {
                    std::lock_guard lock(mutex_);
                    shutdownMarkerReady_ = false;
                    --incomplete_;
                    ownerExited_ = true;
                }
                capacityChanged_.notify_all();
                stateChanged_.notify_all();
                return;
            }
        }
    }

    void completeRequest(Request& request) noexcept {
        Completion completion;
        try {
            if (shouldFail(RuntimeAllocationFailurePoint::ledger))
                completion = {allocationFailure(request.acceptanceSequence), {}};
            else
                completion = processCommand(request);
        } catch (const std::bad_alloc&) {
            completion.status.code = RuntimeStatusCode::resourceExhausted;
            completion.status.acceptanceSequence = request.acceptanceSequence;
            completion.status.message.clear();
        } catch (const std::exception& error) {
            completion.status.code = RuntimeStatusCode::internalFailure;
            completion.status.acceptanceSequence = request.acceptanceSequence;
            try {
                completion.status.message = error.what();
            } catch (...) {
                completion.status.message.clear();
            }
        } catch (...) {
            completion.status.code = RuntimeStatusCode::internalFailure;
            completion.status.acceptanceSequence = request.acceptanceSequence;
            completion.status.message.clear();
        }

        const auto sequence = nextLedgerSequence_++;
        const auto safePoint = currentSafePoint(sequence);
        if (request.kind == RuntimeCommandKind::safePoint && completion.status.isOK()) {
            completion.value = safePoint;
        }
        if (request.kind == RuntimeCommandKind::fault && completion.status.isOK()) {
            try {
                if (shouldFail(RuntimeAllocationFailurePoint::faultResult)) throw std::bad_alloc{};
                completion.value =
                    RuntimeFault{runtimeState_ == RuntimeState::faulted, faultMessage_, safePoint};
            } catch (const std::bad_alloc&) {
                completion = {allocationFailure(request.acceptanceSequence), {}};
            } catch (...) {
                completion.status.code = RuntimeStatusCode::internalFailure;
                completion.status.acceptanceSequence = request.acceptanceSequence;
                completion.status.message.clear();
                completion.value = {};
            }
        }
        const auto resultDigest =
            request.kind == RuntimeCommandKind::safePoint && completion.status.isOK()
                ? BBCMicroTestAccess::digest(*machine_)
                : completionDigest(completion.value);
        appendLedger({sequence, request.acceptanceSequence, LedgerEventKind::command, request.kind,
                      requestedCycles(request), actualCycles(completion), request.payloadDigest,
                      resultDigest, completion.status.code, safePoint});

        try {
            request.completion.set_value(std::move(completion));
        } catch (...) {
            // The caller abandoned its future; owner progress must continue.
        }
        {
            std::lock_guard lock(mutex_);
            --incomplete_;
        }
        capacityChanged_.notify_all();
    }

    Completion processCommand(const Request& request) {
        const auto accepted = request.acceptanceSequence;
        const auto ok = [accepted](CompletionValue value = {}) {
            return Completion{status(RuntimeStatusCode::ok, {}, accepted), std::move(value)};
        };
        const auto invalidState = [accepted](const char* message) {
            return Completion{status(RuntimeStatusCode::invalidState, message, accepted), {}};
        };

        if (testReentrantSubmission_ && !testReentrantSubmissionConsumed_ &&
            request.kind == RuntimeCommandKind::runtimeState) {
            testReentrantSubmissionConsumed_ = true;
            auto nested = submit(RuntimeCommandKind::runtimeState);
            nested.status.acceptanceSequence = accepted;
            return nested;
        }

        switch (request.kind) {
        case RuntimeCommandKind::start:
            if (runtimeState_ == RuntimeState::faulted) {
                return invalidState("cannot start a faulted runtime; reset first");
            }
            runtimeState_ = RuntimeState::running;
            return ok();
        case RuntimeCommandKind::pause:
            if (runtimeState_ == RuntimeState::faulted) {
                return invalidState("cannot pause a faulted runtime; reset first");
            }
            runtimeState_ = RuntimeState::paused;
            return ok();
        case RuntimeCommandKind::reset:
            machine_->reset();
            runtimeState_ = RuntimeState::paused;
            faultMessage_.clear();
            return ok();
        case RuntimeCommandKind::runCycles:
            if (runtimeState_ != RuntimeState::paused) {
                return invalidState("bounded execution requires a paused runtime");
            }
            return executeBounded(std::get<std::uint64_t>(request.payload), accepted);
        case RuntimeCommandKind::runUntilFrame:
            if (runtimeState_ != RuntimeState::paused) {
                return invalidState("run-to-frame requires a paused runtime");
            }
            return executeUntilFrame(std::get<std::uint64_t>(request.payload), accepted);
        case RuntimeCommandKind::loadOSROM: {
            if (runtimeState_ == RuntimeState::faulted) {
                return invalidState("cannot load media while runtime is faulted");
            }
            const auto& bytes = std::get<std::vector<std::uint8_t>>(request.payload);
            if (!machine_->loadOSROM(bytes)) {
                return {status(RuntimeStatusCode::invalidArgument,
                               "OS ROM must be exactly 16384 bytes", accepted),
                        {}};
            }
            return ok();
        }
        case RuntimeCommandKind::loadSidewaysROM: {
            if (runtimeState_ == RuntimeState::faulted) {
                return invalidState("cannot load media while runtime is faulted");
            }
            const auto& payload = std::get<SidewaysPayload>(request.payload);
            if (!machine_->loadSidewaysROM(payload.bank, payload.bytes)) {
                return {status(RuntimeStatusCode::invalidArgument,
                               "invalid sideways ROM bank or size", accepted),
                        {}};
            }
            return ok();
        }
        case RuntimeCommandKind::mountDisc: {
            if (runtimeState_ == RuntimeState::faulted) {
                return invalidState("cannot mount media while runtime is faulted");
            }
            const auto& payload = std::get<DiscPayload>(request.payload);
            if (!machine_->mountDisc(payload.drive, payload.bytes, payload.layout,
                                     payload.writable)) {
                return {status(RuntimeStatusCode::invalidArgument, "invalid drive or disc image",
                               accepted),
                        {}};
            }
            return ok();
        }
        case RuntimeCommandKind::setKey: {
            if (runtimeState_ == RuntimeState::faulted) {
                return invalidState("cannot change input while runtime is faulted");
            }
            const auto payload = std::get<KeyPayload>(request.payload);
            if (payload.column >= 16 || payload.row >= 16) {
                return {status(RuntimeStatusCode::invalidArgument,
                               "keyboard coordinates must be in the range 0...15", accepted),
                        {}};
            }
            machine_->setKey(payload.column, payload.row, payload.pressed);
            return ok();
        }
        case RuntimeCommandKind::setBreak:
            if (runtimeState_ == RuntimeState::faulted) {
                return invalidState("cannot change input while runtime is faulted");
            }
            machine_->setBreak(std::get<bool>(request.payload));
            return ok();
        case RuntimeCommandKind::runtimeState:
            return ok(runtimeState_);
        case RuntimeCommandKind::safePoint:
            return ok();
        case RuntimeCommandKind::fault:
            return ok();
        case RuntimeCommandKind::cpuState:
            return ok(machine_->cpu().state());
        case RuntimeCommandKind::frame: {
            if (shouldFail(RuntimeAllocationFailurePoint::frame))
                return {allocationFailure(accepted), {}};
            const auto& frame = machine_->frame();
            OwnedFrame copy;
            copy.available = !frame.rgba.empty();
            copy.width = frame.width;
            copy.height = frame.height;
            copy.number = frame.number;
            copy.rgba = frame.rgba;
            return ok(std::move(copy));
        }
        case RuntimeCommandKind::renderAudio: {
            if (runtimeState_ == RuntimeState::faulted) {
                return invalidState("cannot render audio while runtime is faulted");
            }
            const auto payload = std::get<AudioPayload>(request.payload);
            if (!std::isfinite(payload.sampleRate) || payload.sampleRate <= 0) {
                return {status(RuntimeStatusCode::invalidArgument,
                               "audio sample rate must be finite and positive", accepted),
                        {}};
            }
            if (payload.frames >= std::vector<float>{}.max_size()) {
                return {status(RuntimeStatusCode::invalidArgument,
                               "audio frame count exceeds container capacity", accepted),
                        {}};
            }
            if (shouldFail(RuntimeAllocationFailurePoint::audio))
                return {allocationFailure(accepted), {}};
            std::vector<float> samples(payload.frames);
            machine_->sound().render(samples.data(), samples.size(), payload.sampleRate);
            return ok(std::move(samples));
        }
        case RuntimeCommandKind::dequeueFrame:
            if (runtimeState_ == RuntimeState::faulted) {
                return invalidState("cannot dequeue output while runtime is faulted");
            }
            {
                auto result = frameOutput_.dequeue();
                lastOutputStatus_ = result.status.code;
                return ok(std::move(result));
            }
        case RuntimeCommandKind::drainAudio:
            if (runtimeState_ == RuntimeState::faulted) {
                return invalidState("cannot drain output while runtime is faulted");
            }
            {
                auto result = audioOutput_.drain(
                    static_cast<std::size_t>(std::get<std::uint64_t>(request.payload)));
                lastOutputStatus_ = result.status.code;
                return ok(std::move(result));
            }
        case RuntimeCommandKind::outputDiagnostics:
            return ok(captureOutputDiagnostics(machine_->cpu().state().cycles, frameOutput_,
                                               audioOutput_, lastOutputStatus_));
        case RuntimeCommandKind::shutdown:
            break;
        }
        return {status(RuntimeStatusCode::internalFailure, "unhandled runtime command", accepted),
                {}};
    }

    Completion executeBounded(std::uint64_t cycles, std::uint64_t accepted) {
        if (shouldFail(RuntimeAllocationFailurePoint::boundedExecution))
            return {allocationFailure(accepted), {}};
        auto before = machine_->checkpoint();
        auto outputBefore = frameOutput_;
        auto audioBefore = audioOutput_;
        const auto numberBefore = nextOutputFrameNumber_;
        const auto audioRemainderBefore = audioCycleRemainder_;
        const auto statusBefore = lastOutputStatus_;
        try {
            return {status(RuntimeStatusCode::ok, {}, accepted), runWithFramePublication(cycles)};
        } catch (const std::runtime_error& error) {
            const auto retained = machine_->cpu().state().cycles - before.cpu.cycles;
            runtimeState_ = RuntimeState::faulted;
            setFaultMessage(error.what());
            return {
                status(RuntimeStatusCode::executionFailed, faultMessage_, accepted), {}, retained};
        } catch (const std::bad_alloc&) {
            machine_->restore(std::move(before));
            frameOutput_ = std::move(outputBefore);
            audioOutput_ = std::move(audioBefore);
            nextOutputFrameNumber_ = numberBefore;
            audioCycleRemainder_ = audioRemainderBefore;
            lastOutputStatus_ = statusBefore;
            return {allocationFailure(accepted), {}};
        } catch (...) {
            machine_->restore(std::move(before));
            frameOutput_ = std::move(outputBefore);
            audioOutput_ = std::move(audioBefore);
            nextOutputFrameNumber_ = numberBefore;
            audioCycleRemainder_ = audioRemainderBefore;
            lastOutputStatus_ = statusBefore;
            runtimeState_ = RuntimeState::faulted;
            setFaultMessage("unknown execution failure");
            return {status(RuntimeStatusCode::internalFailure, faultMessage_, accepted), {}};
        }
    }

    Completion executeUntilFrame(std::uint64_t cycles, std::uint64_t accepted) {
        auto before = machine_->checkpoint();
        auto outputBefore = frameOutput_;
        auto audioBefore = audioOutput_;
        const auto numberBefore = nextOutputFrameNumber_;
        const auto audioRemainderBefore = audioCycleRemainder_;
        const auto statusBefore = lastOutputStatus_;
        try {
            const auto completed = runUntilFrameWithOutputPublication(cycles);
            return {status(RuntimeStatusCode::ok, {}, accepted), completed};
        } catch (const std::runtime_error& error) {
            const auto retained = machine_->cpu().state().cycles - before.cpu.cycles;
            runtimeState_ = RuntimeState::faulted;
            setFaultMessage(error.what());
            return {
                status(RuntimeStatusCode::executionFailed, faultMessage_, accepted), {}, retained};
        } catch (const std::bad_alloc&) {
            machine_->restore(std::move(before));
            frameOutput_ = std::move(outputBefore);
            audioOutput_ = std::move(audioBefore);
            nextOutputFrameNumber_ = numberBefore;
            audioCycleRemainder_ = audioRemainderBefore;
            lastOutputStatus_ = statusBefore;
            return {allocationFailure(accepted), {}};
        } catch (...) {
            machine_->restore(std::move(before));
            frameOutput_ = std::move(outputBefore);
            audioOutput_ = std::move(audioBefore);
            nextOutputFrameNumber_ = numberBefore;
            audioCycleRemainder_ = audioRemainderBefore;
            lastOutputStatus_ = statusBefore;
            runtimeState_ = RuntimeState::faulted;
            setFaultMessage("unknown execution failure");
            return {status(RuntimeStatusCode::internalFailure, faultMessage_, accepted), {}};
        }
    }

    void executeRunningSlice() noexcept {
        if (shouldFail(RuntimeAllocationFailurePoint::sustainedExecution)) {
            runtimeState_ = RuntimeState::faulted;
            faultMessage_.clear();
            const auto sequence = nextLedgerSequence_++;
            const auto safePoint = currentSafePoint(sequence);
            appendLedger({sequence, 0, LedgerEventKind::executionSlice,
                          RuntimeCommandKind::runCycles, MachineRuntime::executionSliceCycles, 0, 0,
                          0, RuntimeStatusCode::resourceExhausted, safePoint});
            return;
        }
        std::optional<BBCMicro::Checkpoint> before;
        std::optional<CompletedFrameQueue> outputBefore;
        std::optional<AudioSampleQueue> audioBefore;
        std::uint64_t numberBefore = nextOutputFrameNumber_;
        std::uint64_t audioRemainderBefore = audioCycleRemainder_;
        OutputStatusCode statusBefore = lastOutputStatus_;
        try {
            before = machine_->checkpoint();
            outputBefore = frameOutput_;
            audioBefore = audioOutput_;
        } catch (const std::bad_alloc&) {
            runtimeState_ = RuntimeState::faulted;
            faultMessage_.clear();
            const auto sequence = nextLedgerSequence_++;
            const auto safePoint = currentSafePoint(sequence);
            appendLedger({sequence, 0, LedgerEventKind::executionSlice,
                          RuntimeCommandKind::runCycles, MachineRuntime::executionSliceCycles, 0, 0,
                          0, RuntimeStatusCode::resourceExhausted, safePoint});
            return;
        } catch (...) {
            runtimeState_ = RuntimeState::faulted;
            faultMessage_.clear();
            const auto sequence = nextLedgerSequence_++;
            const auto safePoint = currentSafePoint(sequence);
            appendLedger({sequence, 0, LedgerEventKind::executionSlice,
                          RuntimeCommandKind::runCycles, MachineRuntime::executionSliceCycles, 0, 0,
                          0, RuntimeStatusCode::internalFailure, safePoint});
            return;
        }
        RuntimeStatusCode code = RuntimeStatusCode::ok;
        std::uint64_t actual = 0;
        const auto restoreBoundary = [&]() noexcept {
            if (!before) return false;
            machine_->restore(std::move(*before));
            if (outputBefore) frameOutput_ = std::move(*outputBefore);
            if (audioBefore) audioOutput_ = std::move(*audioBefore);
            nextOutputFrameNumber_ = numberBefore;
            audioCycleRemainder_ = audioRemainderBefore;
            lastOutputStatus_ = statusBefore;
            return true;
        };
        try {
            actual = runWithFramePublication(MachineRuntime::executionSliceCycles);
        } catch (const std::runtime_error& error) {
            actual = machine_->cpu().state().cycles - before->cpu.cycles;
            runtimeState_ = RuntimeState::faulted;
            setFaultMessage(error.what());
            code = RuntimeStatusCode::executionFailed;
        } catch (const std::bad_alloc&) {
            (void)restoreBoundary();
            actual = 0;
            runtimeState_ = RuntimeState::faulted;
            faultMessage_.clear();
            code = RuntimeStatusCode::resourceExhausted;
        } catch (...) {
            (void)restoreBoundary();
            actual = 0;
            runtimeState_ = RuntimeState::faulted;
            setFaultMessage("unknown execution failure");
            code = RuntimeStatusCode::internalFailure;
        }

        const auto sequence = nextLedgerSequence_++;
        const auto safePoint = currentSafePoint(sequence);
        appendLedger({sequence, 0, LedgerEventKind::executionSlice, RuntimeCommandKind::runCycles,
                      MachineRuntime::executionSliceCycles, actual, 0, mix(1, actual), code,
                      safePoint});
    }

    void setFaultMessage(const char* message) noexcept {
        try {
            faultMessage_ = message;
        } catch (...) {
            faultMessage_.clear();
        }
    }

    std::uint64_t runWithFramePublication(std::uint64_t cycles) {
        const auto started = machine_->cpu().state().cycles;
        std::uint64_t actual = 0;
        do {
            const auto completed = runUntilFrameWithOutputPublication(cycles - actual);
            actual = machine_->cpu().state().cycles - started;
            if (!completed) break;
        } while (actual < cycles);
        return actual;
    }

    bool runUntilFrameWithOutputPublication(std::uint64_t cycles) {
        const auto previousFrame = machine_->frame().number;
        const auto started = machine_->cpu().state().cycles;
        bool completed = false;
        try {
            completed = machine_->runUntilFrame(cycles);
        } catch (...) {
            publishAudioForCycles(machine_->cpu().state().cycles - started);
            throw;
        }
        publishAudioForCycles(machine_->cpu().state().cycles - started);
        if (completed && machine_->frame().number != previousFrame) publishCompletedFrame();
        return completed;
    }

    void publishAudioForCycles(std::uint64_t cycles) {
        const auto fractional = audioCycleRemainder_ + (cycles % 125) * 3;
        std::uint64_t remainingSamples = (cycles / 125) * 3 + fractional / 125;
        audioCycleRemainder_ = fractional % 125;
        std::array<float, audioSampleCapacity> samples{};
        while (remainingSamples != 0) {
            const auto count =
                static_cast<std::size_t>(std::min<std::uint64_t>(remainingSamples, samples.size()));
            machine_->sound().render(samples.data(), count, static_cast<double>(audioSampleRate));
            const auto outcome = audioOutput_.publish(std::span(samples.data(), count));
            lastOutputStatus_ = outcome.code;
            remainingSamples -= count;
        }
    }

    void publishCompletedFrame() {
        const auto& source = machine_->frame();
        CompletedFrame frame;
        frame.number = nextOutputFrameNumber_;
        frame.width = source.width;
        frame.height = source.height;
        frame.rgba = source.rgba;
        const auto outcome = frameOutput_.publish(std::move(frame));
        if (outcome.code == OutputStatusCode::invalidArgument)
            throw std::runtime_error("completed frame violated the bounded-output contract");
        lastOutputStatus_ = outcome.code;
        ++nextOutputFrameNumber_;
    }

    SafePoint currentSafePoint(std::uint64_t ledgerSequence) const {
        const auto cpu = machine_->cpu().state();
        return {cpu.cycles, machine_->frame().number, runtimeState_, ledgerSequence};
    }

    void appendLedger(LedgerEntry entry) noexcept {
        if (!ledgerEnabled_ || ledgerAllocationFailed_) return;
        try {
            std::lock_guard lock(ledgerMutex_);
            ledger_.push_back(std::move(entry));
        } catch (...) {
            ledgerAllocationFailed_ = true;
        }
    }

    static std::uint64_t requestedCycles(const Request& request) noexcept {
        if (request.kind == RuntimeCommandKind::runCycles ||
            request.kind == RuntimeCommandKind::runUntilFrame) {
            return std::get<std::uint64_t>(request.payload);
        }
        return 0;
    }

    static std::uint64_t actualCycles(const Completion& completion) noexcept {
        if (const auto* value = std::get_if<std::uint64_t>(&completion.value)) return *value;
        return completion.retainedCycles;
    }
};

MachineRuntime::MachineRuntime(MachineRuntimeOptions options)
    : impl_(std::make_unique<Impl>(options)) {}

MachineRuntime::~MachineRuntime() {
    if (impl_) (void)impl_->shutdown();
}

RuntimeResult<RuntimeState> MachineRuntime::state() {
    return resultFromCompletion<RuntimeState>(impl_->submit(RuntimeCommandKind::runtimeState));
}

RuntimeStatus MachineRuntime::start() {
    return impl_->submit(RuntimeCommandKind::start).status;
}

RuntimeStatus MachineRuntime::pause() {
    return impl_->submit(RuntimeCommandKind::pause).status;
}

RuntimeStatus MachineRuntime::reset() {
    return impl_->submit(RuntimeCommandKind::reset).status;
}

RuntimeResult<std::uint64_t> MachineRuntime::runFor(std::uint64_t cycles) {
    return resultFromCompletion<std::uint64_t>(
        impl_->submit(RuntimeCommandKind::runCycles, cycles, cycles));
}

RuntimeResult<bool> MachineRuntime::runUntilFrame(std::uint64_t maximumCycles) {
    return resultFromCompletion<bool>(
        impl_->submit(RuntimeCommandKind::runUntilFrame, maximumCycles, maximumCycles));
}

RuntimeStatus MachineRuntime::loadOSROM(std::span<const std::uint8_t> rom) {
    try {
        std::vector<std::uint8_t> copy(rom.begin(), rom.end());
        const auto digest = hashBytes(copy);
        return impl_->submit(RuntimeCommandKind::loadOSROM, std::move(copy), digest).status;
    } catch (const std::bad_alloc&) {
        return allocationFailure();
    }
}

RuntimeStatus MachineRuntime::loadSidewaysROM(std::uint8_t bank,
                                              std::span<const std::uint8_t> rom) {
    if (rom.empty())
        return status(RuntimeStatusCode::invalidArgument,
                      "sideways ROM must contain 1...16384 bytes");
    try {
        SidewaysPayload payload{bank, {rom.begin(), rom.end()}};
        const auto digest = mix(hashBytes(payload.bytes), bank);
        return impl_->submit(RuntimeCommandKind::loadSidewaysROM, std::move(payload), digest)
            .status;
    } catch (const std::bad_alloc&) {
        return allocationFailure();
    }
}

RuntimeStatus MachineRuntime::mountDisc(unsigned drive, std::span<const std::uint8_t> bytes,
                                        DiscImage::Layout layout, bool writable) {
    try {
        DiscPayload payload{drive, {bytes.begin(), bytes.end()}, layout, writable};
        auto digest = mix(hashBytes(payload.bytes), drive);
        digest = mix(digest, layout == DiscImage::Layout::DSD ? 1 : 0);
        digest = mix(digest, writable ? 1 : 0);
        return impl_->submit(RuntimeCommandKind::mountDisc, std::move(payload), digest).status;
    } catch (const std::bad_alloc&) {
        return allocationFailure();
    }
}

RuntimeStatus MachineRuntime::setKey(std::uint8_t column, std::uint8_t row, bool pressed) {
    const auto digest = mix(mix(column, row), pressed ? 1 : 0);
    return impl_->submit(RuntimeCommandKind::setKey, KeyPayload{column, row, pressed}, digest)
        .status;
}

RuntimeStatus MachineRuntime::setBreak(bool pressed) {
    return impl_->submit(RuntimeCommandKind::setBreak, pressed, pressed ? 1 : 0).status;
}

RuntimeResult<CPUState> MachineRuntime::cpuState() {
    return resultFromCompletion<CPUState>(impl_->submit(RuntimeCommandKind::cpuState));
}

RuntimeResult<OwnedFrame> MachineRuntime::frame() {
    return resultFromCompletion<OwnedFrame>(impl_->submit(RuntimeCommandKind::frame));
}

RuntimeResult<std::vector<float>> MachineRuntime::renderAudio(std::size_t frames,
                                                              double sampleRate) {
    auto digest = mix(frames, std::bit_cast<std::uint64_t>(sampleRate));
    return resultFromCompletion<std::vector<float>>(
        impl_->submit(RuntimeCommandKind::renderAudio, AudioPayload{frames, sampleRate}, digest));
}

FrameDequeueResult MachineRuntime::dequeueFrame() {
    return frameResultFromCompletion(impl_->submit(RuntimeCommandKind::dequeueFrame));
}

AudioDrainResult MachineRuntime::drainAudio(std::size_t maximumSamples) {
    return audioResultFromCompletion(impl_->submit(RuntimeCommandKind::drainAudio,
                                                   static_cast<std::uint64_t>(maximumSamples),
                                                   maximumSamples));
}

RuntimeResult<OutputDiagnostics> MachineRuntime::outputDiagnostics() {
    return resultFromCompletion<OutputDiagnostics>(
        impl_->submit(RuntimeCommandKind::outputDiagnostics));
}

RuntimeResult<SafePoint> MachineRuntime::safePoint() {
    return resultFromCompletion<SafePoint>(impl_->submit(RuntimeCommandKind::safePoint));
}

RuntimeResult<RuntimeFault> MachineRuntime::fault() {
    return resultFromCompletion<RuntimeFault>(impl_->submit(RuntimeCommandKind::fault));
}

RuntimeStatus MachineRuntime::shutdown() noexcept {
    return impl_ ? impl_->shutdown() : status(RuntimeStatusCode::ok);
}

std::vector<LedgerEntry> MachineRuntime::ledger() const {
    return impl_->ledger();
}

std::uint64_t MachineRuntime::acceptedCommandCount() const noexcept {
    return impl_->acceptedCommandCount();
}

} // namespace beeb
