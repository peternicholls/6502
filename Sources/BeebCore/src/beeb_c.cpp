#include "beeb_c.h"

// C0-DOC-RATIONALE: docs/code/host-boundary.md owns C token and status invariants.

#include "beeb/runtime.hpp"

#include <algorithm>
#include <condition_variable>
#include <cstring>
#include <exception>
#include <memory>
#include <mutex>
#include <new>
#include <span>
#include <string_view>
#include <unordered_map>
#include <utility>

/// @cond INTERNAL
/// Opaque allocation used only as a stable registry key; machine state lives in HandleState.
struct beeb_machine final {};
/// @endcond

namespace {

/// Creates a zero-filled, null-terminated operation status without throwing.
beeb_status makeStatus(beeb_status_code code, std::string_view message = {}) noexcept {
    beeb_status result{};
    result.code = code;
    const auto length =
        std::min(message.size(), static_cast<std::size_t>(BEEB_STATUS_MESSAGE_CAPACITY - 1));
    if (length != 0) std::memcpy(result.message, message.data(), length);
    result.message[length] = '\0';
    return result;
}

/// Maps the closed C++ status vocabulary one-to-one into the C ABI vocabulary.
beeb_status_code translateStatusCode(beeb::RuntimeStatusCode code) noexcept {
    switch (code) {
    case beeb::RuntimeStatusCode::ok:
        return BEEB_STATUS_OK;
    case beeb::RuntimeStatusCode::invalidArgument:
        return BEEB_STATUS_INVALID_ARGUMENT;
    case beeb::RuntimeStatusCode::invalidState:
        return BEEB_STATUS_INVALID_STATE;
    case beeb::RuntimeStatusCode::executionFailed:
        return BEEB_STATUS_EXECUTION_FAILED;
    case beeb::RuntimeStatusCode::resourceExhausted:
        return BEEB_STATUS_RESOURCE_EXHAUSTED;
    case beeb::RuntimeStatusCode::unavailable:
        return BEEB_STATUS_UNAVAILABLE;
    case beeb::RuntimeStatusCode::reentrantCall:
        return BEEB_STATUS_REENTRANT_CALL;
    case beeb::RuntimeStatusCode::internalFailure:
        return BEEB_STATUS_INTERNAL_FAILURE;
    }
    return BEEB_STATUS_INTERNAL_FAILURE;
}

/// Copies a C++ category and diagnostic into one self-contained C result.
beeb_status translateStatus(const beeb::RuntimeStatus& status) noexcept {
    return makeStatus(translateStatusCode(status.code), status.message);
}

/// Maps structured output pressure and failures into the extended C vocabulary.
beeb_status translateOutputStatus(const beeb::OutputStatus& status) noexcept {
    beeb_status_code code = BEEB_STATUS_INTERNAL_FAILURE;
    std::string_view fallback = "bounded output failed";
    switch (status.code) {
    case beeb::OutputStatusCode::ok:
        code = BEEB_STATUS_OK;
        fallback = {};
        break;
    case beeb::OutputStatusCode::empty:
        code = BEEB_STATUS_EMPTY;
        fallback = "no completed output is available";
        break;
    case beeb::OutputStatusCode::underrun:
        code = BEEB_STATUS_UNDERRUN;
        fallback = "output demand exceeded available data";
        break;
    case beeb::OutputStatusCode::overrun:
        code = BEEB_STATUS_OVERRUN;
        fallback = "oldest bounded output was discarded";
        break;
    case beeb::OutputStatusCode::capacityExceeded:
        code = BEEB_STATUS_CAPACITY_EXCEEDED;
        fallback = "output capacity was exceeded";
        break;
    case beeb::OutputStatusCode::invalidArgument:
        code = BEEB_STATUS_INVALID_ARGUMENT;
        fallback = "output argument is invalid";
        break;
    case beeb::OutputStatusCode::invalidState:
        code = BEEB_STATUS_INVALID_STATE;
        fallback = "output operation is invalid in the current lifecycle";
        break;
    case beeb::OutputStatusCode::resourceExhausted:
        code = BEEB_STATUS_RESOURCE_EXHAUSTED;
        fallback = "output storage could not be allocated";
        break;
    case beeb::OutputStatusCode::unavailable:
        code = BEEB_STATUS_UNAVAILABLE;
        fallback = "output runtime is unavailable";
        break;
    case beeb::OutputStatusCode::productionFailed:
        code = BEEB_STATUS_OUTPUT_FAILED;
        fallback = "output production failed";
        break;
    case beeb::OutputStatusCode::internalFailure:
        break;
    }
    return makeStatus(code, status.message.empty() ? fallback : std::string_view(status.message));
}

/// Maps runtime lifecycle state without exposing implementation storage.
beeb_runtime_state translateState(beeb::RuntimeState state) noexcept {
    switch (state) {
    case beeb::RuntimeState::paused:
        return BEEB_RUNTIME_STATE_PAUSED;
    case beeb::RuntimeState::running:
        return BEEB_RUNTIME_STATE_RUNNING;
    case beeb::RuntimeState::faulted:
        return BEEB_RUNTIME_STATE_FAULTED;
    case beeb::RuntimeState::shuttingDown:
        return BEEB_RUNTIME_STATE_SHUTTING_DOWN;
    }
    return BEEB_RUNTIME_STATE_FAULTED;
}

/// Copies a runtime safe point into its portable C aggregate.
beeb_safe_point translateSafePoint(const beeb::SafePoint& point) noexcept {
    return {
        point.cpuCycles,
        point.frameNumber,
        translateState(point.state),
        point.ledgerSequence,
    };
}

/// Shared runtime state retained until destruction and every entered call complete.
/// The host-boundary contract (docs/code/host-boundary.md) requires registry admission
/// before state locking, stable opaque tokens that are never dereferenced, admitted-call
/// retention, and first-destroy ownership. Later destroyers wait for the owner's result;
/// calls arriving after destruction starts are rejected as unavailable.
struct HandleState final {
    explicit HandleState(beeb::MachineRuntimeOptions options = {}) : runtime(options) {}

    std::mutex mutex;
    std::condition_variable callsFinished;
    std::size_t activeCalls = 0;
    bool destroying = false;
    bool destroyComplete = false;
    beeb_status destroyStatus =
        makeStatus(BEEB_STATUS_INTERNAL_FAILURE, "destroy did not complete");
    beeb::MachineRuntime runtime;
};

/// Serializes raw-token admission and removal without dereferencing the token.
/// Keep this lock before `HandleState::mutex` everywhere to preserve the documented order.
std::mutex registryMutex;
/// Retains each live state independently of its opaque allocation lifetime.
std::unordered_map<beeb_machine*, std::shared_ptr<HandleState>> registry;

/// RAII admission record that prevents handle release while one C call is inside.
class ActiveCall final {
  public:
    explicit ActiveCall(beeb_machine* machine) noexcept {
        if (!machine) {
            status_ = makeStatus(BEEB_STATUS_INVALID_ARGUMENT, "machine handle is null");
            return;
        }
        try {
            std::lock_guard registryLock(registryMutex);
            const auto entry = registry.find(machine);
            if (entry == registry.end()) {
                status_ = makeStatus(BEEB_STATUS_INVALID_ARGUMENT, "machine handle is not live");
                return;
            }
            auto state = entry->second;
            std::lock_guard stateLock(state->mutex);
            if (state->destroying) {
                status_ = makeStatus(BEEB_STATUS_UNAVAILABLE, "machine is being destroyed");
                return;
            }
            ++state->activeCalls;
            state_ = std::move(state);
            status_ = makeStatus(BEEB_STATUS_OK);
        } catch (const std::exception& error) {
            status_ = makeStatus(BEEB_STATUS_INTERNAL_FAILURE, error.what());
        } catch (...) {
            status_ =
                makeStatus(BEEB_STATUS_INTERNAL_FAILURE, "unknown C handle admission failure");
        }
    }

    ActiveCall(const ActiveCall&) = delete;
    ActiveCall& operator=(const ActiveCall&) = delete;

    ~ActiveCall() {
        if (!state_) return;
        try {
            {
                std::lock_guard lock(state_->mutex);
                --state_->activeCalls;
            }
            state_->callsFinished.notify_all();
        } catch (...) {
            // Mutex destruction cannot race while this shared state is retained.
        }
    }

    explicit operator bool() const noexcept { return state_ != nullptr; }
    const beeb_status& status() const noexcept { return status_; }
    beeb::MachineRuntime& runtime() noexcept { return state_->runtime; }

  private:
    std::shared_ptr<HandleState> state_;
    beeb_status status_ = makeStatus(BEEB_STATUS_INTERNAL_FAILURE);
};

/// Admits one handle call, contains adapter exceptions, and releases admission on return.
template <typename Callable>
beeb_status operation(beeb_machine* machine, Callable&& callable) noexcept {
    ActiveCall call(machine);
    if (!call) return call.status();
    try {
        return callable(call.runtime());
    } catch (const std::bad_alloc&) {
        return makeStatus(BEEB_STATUS_RESOURCE_EXHAUSTED, "C adapter allocation failed");
    } catch (const std::exception& error) {
        return makeStatus(BEEB_STATUS_INTERNAL_FAILURE, error.what());
    } catch (...) {
        return makeStatus(BEEB_STATUS_INTERNAL_FAILURE, "unknown C++ exception");
    }
}

/// Creates the consistent invalid-argument result for a required null pointer.
beeb_status missingOutput(const char* name) noexcept {
    return makeStatus(BEEB_STATUS_INVALID_ARGUMENT, name);
}

beeb_status createMachine(beeb_machine** out_machine, beeb::MachineRuntimeOptions options) {
    if (!out_machine) return missingOutput("machine output is null");
    try {
        auto token = std::make_unique<beeb_machine>();
        auto state = std::make_shared<HandleState>(options);
        {
            std::lock_guard lock(registryMutex);
            registry.emplace(token.get(), std::move(state));
        }
        *out_machine = token.release();
        return makeStatus(BEEB_STATUS_OK);
    } catch (const std::bad_alloc&) {
        return makeStatus(BEEB_STATUS_RESOURCE_EXHAUSTED, "machine allocation failed");
    } catch (const std::exception& error) {
        return makeStatus(BEEB_STATUS_INTERNAL_FAILURE, error.what());
    } catch (...) {
        return makeStatus(BEEB_STATUS_INTERNAL_FAILURE, "unknown machine creation failure");
    }
}

} // namespace

beeb_status beeb_test_create_with_allocation_failure(beeb_machine** out_machine,
                                                     beeb::RuntimeAllocationFailurePoint point) {
    return createMachine(out_machine, {.failAllocationAt = point});
}

extern "C" {

const char* beeb_version_string(void) {
    return BEEB_VERSION_STRING;
}

beeb_status beeb_create(beeb_machine** out_machine) {
    return createMachine(out_machine, {});
}

beeb_status beeb_destroy(beeb_machine* machine) {
    // Destruction is a single-owner transaction: one caller marks the state, waits for
    // admitted calls, shuts down, removes the token, and publishes the shared result.
    if (!machine) return makeStatus(BEEB_STATUS_INVALID_ARGUMENT, "machine handle is null");
    try {
        std::shared_ptr<HandleState> state;
        bool ownsDestroy = false;
        {
            std::lock_guard registryLock(registryMutex);
            const auto entry = registry.find(machine);
            if (entry == registry.end()) {
                return makeStatus(BEEB_STATUS_INVALID_ARGUMENT, "machine handle is not live");
            }
            state = entry->second;
            std::lock_guard stateLock(state->mutex);
            if (!state->destroying) {
                state->destroying = true;
                ownsDestroy = true;
            }
        }

        if (!ownsDestroy) {
            std::unique_lock lock(state->mutex);
            state->callsFinished.wait(lock, [&] { return state->destroyComplete; });
            return state->destroyStatus;
        }

        {
            std::unique_lock lock(state->mutex);
            state->callsFinished.wait(lock, [&] { return state->activeCalls == 0; });
        }
        const auto result = translateStatus(state->runtime.shutdown());
        {
            std::lock_guard registryLock(registryMutex);
            const auto entry = registry.find(machine);
            if (entry != registry.end() && entry->second == state) registry.erase(entry);
        }
        delete machine;
        {
            std::lock_guard lock(state->mutex);
            state->destroyStatus = result;
            state->destroyComplete = true;
        }
        state->callsFinished.notify_all();
        return result;
    } catch (const std::exception& error) {
        return makeStatus(BEEB_STATUS_INTERNAL_FAILURE, error.what());
    } catch (...) {
        return makeStatus(BEEB_STATUS_INTERNAL_FAILURE, "unknown machine destruction failure");
    }
}

beeb_status beeb_get_runtime_state(beeb_machine* machine, beeb_runtime_state* out_state) {
    if (!out_state) return missingOutput("runtime-state output is null");
    return operation(machine, [&](beeb::MachineRuntime& runtime) {
        const auto result = runtime.state();
        if (!result.status) return translateStatus(result.status);
        *out_state = translateState(*result.value);
        return makeStatus(BEEB_STATUS_OK);
    });
}

beeb_status beeb_start(beeb_machine* machine) {
    return operation(
        machine, [](beeb::MachineRuntime& runtime) { return translateStatus(runtime.start()); });
}

beeb_status beeb_pause(beeb_machine* machine) {
    return operation(
        machine, [](beeb::MachineRuntime& runtime) { return translateStatus(runtime.pause()); });
}

beeb_status beeb_reset(beeb_machine* machine) {
    return operation(
        machine, [](beeb::MachineRuntime& runtime) { return translateStatus(runtime.reset()); });
}

beeb_status beeb_run_cycles(beeb_machine* machine, uint64_t cycles, uint64_t* out_actual_cycles) {
    if (!out_actual_cycles) return missingOutput("cycle-count output is null");
    return operation(machine, [&](beeb::MachineRuntime& runtime) {
        const auto result = runtime.runFor(cycles);
        if (!result.status) return translateStatus(result.status);
        *out_actual_cycles = *result.value;
        return makeStatus(BEEB_STATUS_OK);
    });
}

beeb_status beeb_run_until_frame(beeb_machine* machine, uint64_t maximum_cycles,
                                 int* out_completed) {
    if (!out_completed) return missingOutput("frame-completion output is null");
    return operation(machine, [&](beeb::MachineRuntime& runtime) {
        const auto result = runtime.runUntilFrame(maximum_cycles);
        if (!result.status) return translateStatus(result.status);
        *out_completed = *result.value ? 1 : 0;
        return makeStatus(BEEB_STATUS_OK);
    });
}

beeb_status beeb_load_os_rom(beeb_machine* machine, const uint8_t* bytes, size_t count) {
    if (!bytes) return missingOutput("OS ROM data is null");
    return operation(machine, [&](beeb::MachineRuntime& runtime) {
        return translateStatus(runtime.loadOSROM(std::span(bytes, count)));
    });
}

beeb_status beeb_load_sideways_rom(beeb_machine* machine, uint8_t bank, const uint8_t* bytes,
                                   size_t count) {
    if (!bytes) return missingOutput("sideways ROM data is null");
    return operation(machine, [&](beeb::MachineRuntime& runtime) {
        return translateStatus(runtime.loadSidewaysROM(bank, std::span(bytes, count)));
    });
}

beeb_status beeb_mount_disc(beeb_machine* machine, unsigned drive, const uint8_t* bytes,
                            size_t count, int double_sided, int writable) {
    if (!bytes) return missingOutput("disc image data is null");
    return operation(machine, [&](beeb::MachineRuntime& runtime) {
        const auto layout =
            double_sided != 0 ? beeb::DiscImage::Layout::DSD : beeb::DiscImage::Layout::SSD;
        return translateStatus(
            runtime.mountDisc(drive, std::span(bytes, count), layout, writable != 0));
    });
}

beeb_status beeb_get_cpu_state(beeb_machine* machine, beeb_cpu_state* out_state) {
    if (!out_state) return missingOutput("CPU-state output is null");
    return operation(machine, [&](beeb::MachineRuntime& runtime) {
        const auto result = runtime.cpuState();
        if (!result.status) return translateStatus(result.status);
        const auto& state = *result.value;
        const beeb_cpu_state output{state.a, state.x,  state.y,     state.sp,
                                    state.p, state.pc, state.cycles};
        *out_state = output;
        return makeStatus(BEEB_STATUS_OK);
    });
}

beeb_status beeb_get_frame(beeb_machine* machine, beeb_frame* out_frame) {
    if (!out_frame) return missingOutput("frame output is null");
    if (out_frame->rgba != nullptr) {
        return makeStatus(BEEB_STATUS_INVALID_ARGUMENT,
                          "frame output must be released before reuse");
    }
    return operation(machine, [&](beeb::MachineRuntime& runtime) {
        auto result = runtime.frame();
        if (!result.status) return translateStatus(result.status);
        const auto& frame = *result.value;
        beeb_frame output{};
        if (frame.available) {
            auto bytes = std::make_unique<uint8_t[]>(frame.rgba.size());
            std::copy(frame.rgba.begin(), frame.rgba.end(), bytes.get());
            output.available = 1;
            output.width = frame.width;
            output.height = frame.height;
            output.number = frame.number;
            output.rgba_size = frame.rgba.size();
            output.rgba = bytes.release();
        }
        *out_frame = output;
        return makeStatus(BEEB_STATUS_OK);
    });
}

beeb_status beeb_dequeue_frame(beeb_machine* machine, beeb_frame* out_frame) {
    if (!out_frame) return missingOutput("frame output is null");
    if (out_frame->rgba != nullptr) {
        return makeStatus(BEEB_STATUS_INVALID_ARGUMENT,
                          "frame output must be released before reuse");
    }
    return operation(machine, [&](beeb::MachineRuntime& runtime) {
        auto result = runtime.dequeueFrame();
        if (!result.status.isOK()) return translateOutputStatus(result.status);
        if (!result.frame) {
            return makeStatus(BEEB_STATUS_INTERNAL_FAILURE,
                              "frame dequeue succeeded without an owned value");
        }
        auto bytes = std::make_unique<uint8_t[]>(result.frame->rgba.size());
        std::copy(result.frame->rgba.begin(), result.frame->rgba.end(), bytes.get());
        const beeb_frame output{1,
                                result.frame->width,
                                result.frame->height,
                                result.frame->number,
                                bytes.release(),
                                result.frame->rgba.size()};
        *out_frame = output;
        return makeStatus(BEEB_STATUS_OK);
    });
}

beeb_status beeb_frame_release(beeb_frame* frame) {
    if (!frame) return missingOutput("frame is null");
    delete[] frame->rgba;
    *frame = {};
    return makeStatus(BEEB_STATUS_OK);
}

beeb_status beeb_render_audio(beeb_machine* machine, float* mono, size_t frames,
                              double sample_rate) {
    if (!mono) return missingOutput("audio output buffer is null");
    return operation(machine, [&](beeb::MachineRuntime& runtime) {
        auto result = runtime.renderAudio(frames, sample_rate);
        if (!result.status) return translateStatus(result.status);
        std::copy(result.value->begin(), result.value->end(), mono);
        return makeStatus(BEEB_STATUS_OK);
    });
}

beeb_status beeb_set_key(beeb_machine* machine, uint8_t column, uint8_t row, int pressed) {
    return operation(machine, [&](beeb::MachineRuntime& runtime) {
        return translateStatus(runtime.setKey(column, row, pressed != 0));
    });
}

beeb_status beeb_set_break(beeb_machine* machine, int pressed) {
    return operation(machine, [&](beeb::MachineRuntime& runtime) {
        return translateStatus(runtime.setBreak(pressed != 0));
    });
}

beeb_status beeb_get_safe_point(beeb_machine* machine, beeb_safe_point* out_safe_point) {
    if (!out_safe_point) return missingOutput("safe-point output is null");
    return operation(machine, [&](beeb::MachineRuntime& runtime) {
        const auto result = runtime.safePoint();
        if (!result.status) return translateStatus(result.status);
        *out_safe_point = translateSafePoint(*result.value);
        return makeStatus(BEEB_STATUS_OK);
    });
}

beeb_status beeb_get_fault(beeb_machine* machine, beeb_fault_detail* out_fault) {
    if (!out_fault) return missingOutput("fault output is null");
    return operation(machine, [&](beeb::MachineRuntime& runtime) {
        const auto result = runtime.fault();
        if (!result.status) return translateStatus(result.status);
        beeb_fault_detail output{};
        output.available = result.value->available ? 1 : 0;
        const auto message = std::string_view(result.value->message);
        const auto length =
            std::min(message.size(), static_cast<std::size_t>(BEEB_STATUS_MESSAGE_CAPACITY - 1));
        if (length != 0) std::memcpy(output.message, message.data(), length);
        output.message[length] = '\0';
        output.safe_point = translateSafePoint(result.value->safePoint);
        *out_fault = output;
        return makeStatus(BEEB_STATUS_OK);
    });
}

} // extern "C"
