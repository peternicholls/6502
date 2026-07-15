#include "beeb/runtime.hpp"

#include "beeb/machine.hpp"

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

RuntimeStatus status(RuntimeStatusCode code, std::string message = {},
                     std::uint64_t acceptanceSequence = 0) {
    return {code, std::move(message), acceptanceSequence};
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

struct SidewaysPayload {
    std::uint8_t bank = 0;
    std::vector<std::uint8_t> bytes;
};

struct DiscPayload {
    unsigned drive = 0;
    std::vector<std::uint8_t> bytes;
    DiscImage::Layout layout = DiscImage::Layout::SSD;
    bool writable = false;
};

struct KeyPayload {
    std::uint8_t column = 0;
    std::uint8_t row = 0;
    bool pressed = false;
};

struct AudioPayload {
    std::size_t frames = 0;
    double sampleRate = 0;
};

using CommandPayload = std::variant<
    std::monostate,
    std::uint64_t,
    std::vector<std::uint8_t>,
    SidewaysPayload,
    DiscPayload,
    KeyPayload,
    bool,
    AudioPayload>;

using CompletionValue = std::variant<
    std::monostate,
    RuntimeState,
    std::uint64_t,
    bool,
    CPUState,
    OwnedFrame,
    std::vector<float>,
    SafePoint,
    RuntimeFault>;

std::uint64_t hashString(const std::string& value) noexcept {
    return hashBytes(std::span(
        reinterpret_cast<const std::uint8_t*>(value.data()), value.size()));
}

std::uint64_t completionDigest(const CompletionValue& value) noexcept {
    return std::visit([](const auto& result) -> std::uint64_t {
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
    }, value);
}

struct Completion {
    RuntimeStatus status;
    CompletionValue value;
};

struct Request {
    RuntimeCommandKind kind = RuntimeCommandKind::runtimeState;
    CommandPayload payload;
    std::uint64_t acceptanceSequence = 0;
    std::uint64_t payloadDigest = 0;
    std::promise<Completion> completion;
};

template <typename T>
RuntimeResult<T> resultFromCompletion(Completion completion) {
    RuntimeResult<T> result;
    result.status = std::move(completion.status);
    if (!result.status.isOK()) return result;
    if (auto* value = std::get_if<T>(&completion.value)) {
        result.value = std::move(*value);
        return result;
    }
    result.status = status(
        RuntimeStatusCode::internalFailure,
        "runtime command completed without its declared result",
        result.status.acceptanceSequence);
    return result;
}

} // namespace

class MachineRuntime::Impl final {
public:
    explicit Impl(MachineRuntimeOptions options)
        : ledgerEnabled_(options.enableLedger), owner_([this] { ownerLoop(); }) {
        std::unique_lock lock(mutex_);
        stateChanged_.wait(lock, [this] { return ready_; });
        if (startupError_) std::rethrow_exception(startupError_);
    }

    ~Impl() = default;

    Completion submit(RuntimeCommandKind kind, CommandPayload payload = {},
                      std::uint64_t payloadDigest = 0) {
        {
            std::lock_guard lock(mutex_);
            if (std::this_thread::get_id() == ownerId_) {
                return {status(RuntimeStatusCode::reentrantCall,
                               "runtime commands cannot be submitted by the owner thread"), {}};
            }
        }

        std::shared_ptr<Request> request;
        std::future<Completion> future;
        try {
            request = std::make_shared<Request>();
            request->kind = kind;
            request->payload = std::move(payload);
            request->payloadDigest = payloadDigest;
            future = request->completion.get_future();
        } catch (const std::bad_alloc&) {
            return {status(RuntimeStatusCode::resourceExhausted,
                           "runtime command allocation failed"), {}};
        } catch (const std::exception& error) {
            return {status(RuntimeStatusCode::internalFailure, error.what()), {}};
        }

        {
            std::unique_lock lock(mutex_);
            capacityChanged_.wait(lock, [this] {
                return !accepting_ || incomplete_ < MachineRuntime::commandCapacity;
            });
            if (!accepting_) {
                return {status(RuntimeStatusCode::unavailable,
                               "runtime is shutting down"), {}};
            }
            try {
                queue_.push_back(request);
            } catch (const std::bad_alloc&) {
                return {status(RuntimeStatusCode::resourceExhausted,
                               "runtime queue allocation failed"), {}};
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

    RuntimeStatus shutdown() noexcept {
        std::uint64_t acceptanceSequence = 0;
        try {
            std::unique_lock lock(mutex_);
            if (std::this_thread::get_id() == ownerId_) {
                return status(RuntimeStatusCode::reentrantCall,
                              "runtime owner cannot join itself");
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

            capacityChanged_.wait(lock, [this] {
                return incomplete_ < MachineRuntime::commandCapacity;
            });
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
        } catch (const std::exception& error) {
            return status(RuntimeStatusCode::internalFailure, error.what(), acceptanceSequence);
        } catch (...) {
            return status(RuntimeStatusCode::internalFailure,
                          "unknown shutdown failure", acceptanceSequence);
        }
    }

    std::vector<LedgerEntry> ledger() const {
        if (!ledgerEnabled_) return {};
        std::lock_guard lock(ledgerMutex_);
        return ledger_;
    }

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

    RuntimeState runtimeState_ = RuntimeState::paused;
    std::string faultMessage_;
    std::unique_ptr<BBCMicro> machine_;

    bool ledgerEnabled_ = false;
    mutable std::mutex ledgerMutex_;
    std::vector<LedgerEntry> ledger_;
    std::uint64_t nextLedgerSequence_ = 1;
    bool ledgerAllocationFailed_ = false;

    std::jthread owner_;

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
                appendLedger({sequence, shutdownAcceptanceSequence_,
                              LedgerEventKind::command, RuntimeCommandKind::shutdown,
                              0, 0, 0, 0, RuntimeStatusCode::ok, safePoint});
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
            completion = processCommand(request);
        } catch (const std::bad_alloc&) {
            completion.status = status(RuntimeStatusCode::resourceExhausted,
                                       "runtime operation allocation failed",
                                       request.acceptanceSequence);
        } catch (const std::exception& error) {
            completion.status = status(RuntimeStatusCode::internalFailure,
                                       error.what(), request.acceptanceSequence);
        } catch (...) {
            completion.status = status(RuntimeStatusCode::internalFailure,
                                       "unknown runtime operation failure",
                                       request.acceptanceSequence);
        }

        const auto sequence = nextLedgerSequence_++;
        const auto safePoint = currentSafePoint(sequence);
        if (request.kind == RuntimeCommandKind::safePoint && completion.status.isOK()) {
            completion.value = safePoint;
        }
        if (request.kind == RuntimeCommandKind::fault && completion.status.isOK()) {
            completion.value = RuntimeFault{
                runtimeState_ == RuntimeState::faulted, faultMessage_, safePoint};
        }
        appendLedger({sequence, request.acceptanceSequence,
                      LedgerEventKind::command, request.kind,
                      requestedCycles(request), actualCycles(completion),
                      request.payloadDigest, completionDigest(completion.value),
                      completion.status.code, safePoint});

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
                               "OS ROM must be exactly 16384 bytes", accepted), {}};
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
                               "invalid sideways ROM bank or size", accepted), {}};
            }
            return ok();
        }
        case RuntimeCommandKind::mountDisc: {
            if (runtimeState_ == RuntimeState::faulted) {
                return invalidState("cannot mount media while runtime is faulted");
            }
            const auto& payload = std::get<DiscPayload>(request.payload);
            if (!machine_->mountDisc(payload.drive, payload.bytes,
                                     payload.layout, payload.writable)) {
                return {status(RuntimeStatusCode::invalidArgument,
                               "invalid drive or disc image", accepted), {}};
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
                               "keyboard coordinates must be in the range 0...15",
                               accepted), {}};
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
                               "audio sample rate must be finite and positive",
                               accepted), {}};
            }
            std::vector<float> samples(payload.frames);
            machine_->sound().render(samples.data(), samples.size(), payload.sampleRate);
            return ok(std::move(samples));
        }
        case RuntimeCommandKind::shutdown:
            break;
        }
        return {status(RuntimeStatusCode::internalFailure,
                       "unhandled runtime command", accepted), {}};
    }

    Completion executeBounded(std::uint64_t cycles, std::uint64_t accepted) {
        const auto before = machine_->cpu().state();
        try {
            return {status(RuntimeStatusCode::ok, {}, accepted), machine_->runFor(cycles)};
        } catch (const std::exception& error) {
            machine_->cpu().setState(before);
            runtimeState_ = RuntimeState::faulted;
            faultMessage_ = error.what();
            return {status(RuntimeStatusCode::executionFailed,
                           faultMessage_, accepted), {}};
        } catch (...) {
            machine_->cpu().setState(before);
            runtimeState_ = RuntimeState::faulted;
            faultMessage_ = "unknown execution failure";
            return {status(RuntimeStatusCode::executionFailed,
                           faultMessage_, accepted), {}};
        }
    }

    Completion executeUntilFrame(std::uint64_t cycles, std::uint64_t accepted) {
        const auto before = machine_->cpu().state();
        try {
            return {status(RuntimeStatusCode::ok, {}, accepted),
                    machine_->runUntilFrame(cycles)};
        } catch (const std::exception& error) {
            machine_->cpu().setState(before);
            runtimeState_ = RuntimeState::faulted;
            faultMessage_ = error.what();
            return {status(RuntimeStatusCode::executionFailed,
                           faultMessage_, accepted), {}};
        } catch (...) {
            machine_->cpu().setState(before);
            runtimeState_ = RuntimeState::faulted;
            faultMessage_ = "unknown execution failure";
            return {status(RuntimeStatusCode::executionFailed,
                           faultMessage_, accepted), {}};
        }
    }

    void executeRunningSlice() noexcept {
        const auto before = machine_->cpu().state();
        RuntimeStatusCode code = RuntimeStatusCode::ok;
        std::uint64_t actual = 0;
        try {
            actual = machine_->runFor(MachineRuntime::executionSliceCycles);
        } catch (const std::exception& error) {
            const auto after = machine_->cpu().state();
            actual = after.cycles - before.cycles;
            machine_->cpu().setState(before);
            runtimeState_ = RuntimeState::faulted;
            faultMessage_ = error.what();
            code = RuntimeStatusCode::executionFailed;
        } catch (...) {
            const auto after = machine_->cpu().state();
            actual = after.cycles - before.cycles;
            machine_->cpu().setState(before);
            runtimeState_ = RuntimeState::faulted;
            faultMessage_ = "unknown execution failure";
            code = RuntimeStatusCode::executionFailed;
        }

        const auto sequence = nextLedgerSequence_++;
        const auto safePoint = currentSafePoint(sequence);
        appendLedger({sequence, 0, LedgerEventKind::executionSlice,
                      RuntimeCommandKind::runCycles,
                      MachineRuntime::executionSliceCycles, actual, 0,
                      mix(1, actual), code, safePoint});
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
        return 0;
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
        return impl_->submit(RuntimeCommandKind::loadOSROM,
                             std::move(copy), digest).status;
    } catch (const std::bad_alloc&) {
        return status(RuntimeStatusCode::resourceExhausted,
                      "OS ROM command copy failed");
    }
}

RuntimeStatus MachineRuntime::loadSidewaysROM(
    std::uint8_t bank, std::span<const std::uint8_t> rom) {
    try {
        SidewaysPayload payload{bank, {rom.begin(), rom.end()}};
        const auto digest = mix(hashBytes(payload.bytes), bank);
        return impl_->submit(RuntimeCommandKind::loadSidewaysROM,
                             std::move(payload), digest).status;
    } catch (const std::bad_alloc&) {
        return status(RuntimeStatusCode::resourceExhausted,
                      "sideways ROM command copy failed");
    }
}

RuntimeStatus MachineRuntime::mountDisc(
    unsigned drive, std::span<const std::uint8_t> bytes,
    DiscImage::Layout layout, bool writable) {
    try {
        DiscPayload payload{drive, {bytes.begin(), bytes.end()}, layout, writable};
        auto digest = mix(hashBytes(payload.bytes), drive);
        digest = mix(digest, layout == DiscImage::Layout::DSD ? 1 : 0);
        digest = mix(digest, writable ? 1 : 0);
        return impl_->submit(RuntimeCommandKind::mountDisc,
                             std::move(payload), digest).status;
    } catch (const std::bad_alloc&) {
        return status(RuntimeStatusCode::resourceExhausted,
                      "disc command copy failed");
    }
}

RuntimeStatus MachineRuntime::setKey(
    std::uint8_t column, std::uint8_t row, bool pressed) {
    const auto digest = mix(mix(column, row), pressed ? 1 : 0);
    return impl_->submit(RuntimeCommandKind::setKey,
                         KeyPayload{column, row, pressed}, digest).status;
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

RuntimeResult<std::vector<float>> MachineRuntime::renderAudio(
    std::size_t frames, double sampleRate) {
    auto digest = mix(frames, std::bit_cast<std::uint64_t>(sampleRate));
    return resultFromCompletion<std::vector<float>>(
        impl_->submit(RuntimeCommandKind::renderAudio,
                      AudioPayload{frames, sampleRate}, digest));
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
