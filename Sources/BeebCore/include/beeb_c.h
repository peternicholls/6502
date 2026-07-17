#pragma once

/// @file
/// Structured, exception-safe C ABI for owning one serialized emulator runtime.
///
/// @par Migration from 0.1
/// The 0.2 boundary intentionally replaces sentinel returns and
/// `beeb_last_error()`. Every fallible call now returns a self-contained
/// `beeb_status`; successful values are written through required out-parameters.
/// `beeb_create()` also uses an out-parameter, frames are caller-owned values
/// released with `beeb_frame_release()`, and all machine operations serialize
/// through the runtime owner. Repository consumers migrate atomically because
/// the project is still pre-1.0.

#include "beeb/version.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/// Fixed storage available for each operation-scoped UTF-8 diagnostic.
#define BEEB_STATUS_MESSAGE_CAPACITY 256

/// Stable status categories shared with the C++ runtime and output contracts.
/// Documentation rationale: docs/code/host-boundary.md owns the cross-language
/// category and recovery contract represented by this closed vocabulary.
typedef enum beeb_status_code {
    BEEB_STATUS_OK = 0,                 ///< Operation completed successfully.
    BEEB_STATUS_INVALID_ARGUMENT = 1,   ///< Pointer, size, value, or output is invalid.
    BEEB_STATUS_INVALID_STATE = 2,      ///< Command is not legal in the current state.
    BEEB_STATUS_EXECUTION_FAILED = 3,   ///< Emulated execution faulted at a safe point.
    BEEB_STATUS_RESOURCE_EXHAUSTED = 4, ///< Required allocation or capacity failed.
    BEEB_STATUS_UNAVAILABLE = 5,        ///< Runtime is shutting down or no longer accepts work.
    BEEB_STATUS_REENTRANT_CALL = 6,     ///< Reserved: owner-thread re-entry would deadlock.
    BEEB_STATUS_INTERNAL_FAILURE = 7,   ///< Unexpected implementation failure was contained.
    BEEB_STATUS_EMPTY = 8,              ///< No complete output value is currently retained.
    BEEB_STATUS_UNDERRUN = 9,           ///< Partial output includes an exact demand shortfall.
    BEEB_STATUS_OVERRUN = 10,           ///< Oldest bounded output was discarded under pressure.
    BEEB_STATUS_CAPACITY_EXCEEDED = 11, ///< Caller capacity or fixed output limit was exceeded.
    BEEB_STATUS_OUTPUT_FAILED = 12      ///< Output conversion or production failed safely.
} beeb_status_code;

/// Complete result of one C operation.
///
/// Success is exactly `BEEB_STATUS_OK` with an empty message. Failure contains
/// a null-terminated, possibly truncated diagnostic owned by this value; later
/// calls cannot overwrite it.
typedef struct beeb_status {
    beeb_status_code code;                      ///< Stable machine-readable category.
    char message[BEEB_STATUS_MESSAGE_CAPACITY]; ///< Operation-owned UTF-8 diagnostic.
} beeb_status;

/// Opaque token for one independent runtime and its machine owner.
///
/// Create with `beeb_create()` and release with `beeb_destroy()`. Operations
/// may enter concurrently. Destroy waits for calls already inside the API and
/// invalidates the token when it returns; no later call may use that pointer.
typedef struct beeb_machine beeb_machine;

/// Public lifecycle states observable at runtime safe points.
typedef enum beeb_runtime_state {
    BEEB_RUNTIME_STATE_PAUSED = 0,       ///< Quiescent and accepting commands.
    BEEB_RUNTIME_STATE_RUNNING = 1,      ///< Executing deterministic slices.
    BEEB_RUNTIME_STATE_FAULTED = 2,      ///< Execution failed; reset is required.
    BEEB_RUNTIME_STATE_SHUTTING_DOWN = 3 ///< Acceptance stopped while work drains.
} beeb_runtime_state;

/// Portable snapshot of the emulated 6502 programmer-visible state.
typedef struct beeb_cpu_state {
    uint8_t a;       ///< Accumulator register.
    uint8_t x;       ///< X index register.
    uint8_t y;       ///< Y index register.
    uint8_t sp;      ///< Stack pointer register.
    uint8_t p;       ///< Processor status flags.
    uint16_t pc;     ///< Program counter.
    uint64_t cycles; ///< Total completed CPU cycles.
} beeb_cpu_state;

/// Identity of a completed-instruction and fully advanced-device safe point.
typedef struct beeb_safe_point {
    uint64_t cpu_cycles;      ///< Total completed CPU cycles.
    uint64_t frame_number;    ///< Latest completed CRTC frame number.
    beeb_runtime_state state; ///< Lifecycle state at this boundary.
    uint64_t ledger_sequence; ///< Latest total command/execution identity.
} beeb_safe_point;

/// Owned fault observation returned by `beeb_get_fault()`.
typedef struct beeb_fault_detail {
    int available;                              ///< Non-zero only while faulted.
    char message[BEEB_STATUS_MESSAGE_CAPACITY]; ///< Retained execution diagnostic.
    beeb_safe_point safe_point;                 ///< Boundary at which it is observed.
} beeb_fault_detail;

/// Caller-owned copy of one completed video frame.
///
/// A successful unavailable value has `available == 0`, null `rgba`, and zero
/// metadata. An available value owns exactly `rgba_size` allocated bytes.
/// Bytes are row-major red, green, blue, alpha with one byte per component and
/// `rgba_size == width * height * 4`; no byte aliases runtime or queue storage.
/// Initialize storage to zero before first use and release every successful
/// value with `beeb_frame_release()` before overwriting or discarding it.
typedef struct beeb_frame {
    int available;    ///< Non-zero when a complete frame exists.
    uint32_t width;   ///< Pixel width when available.
    uint32_t height;  ///< Pixel height when available.
    uint64_t number;  ///< Monotonic CRTC frame number.
    uint8_t* rgba;    ///< Caller-owned packed 8-bit RGBA storage.
    size_t rgba_size; ///< Allocated byte count at `rgba`.
} beeb_frame;

/// Atomic metadata returned with one caller-buffer continuous-audio drain.
///
/// `copied` samples at the start of the caller buffer are valid on OK or
/// UNDERRUN. `shortfall` is exactly `requested - copied`; demand and cumulative
/// pressure counters are observed after that same drain operation. Samples are
/// mono IEEE Float32 at 48,000 Hz and never alias runtime storage.
typedef struct beeb_audio_drain_result {
    uint64_t first_sample;   ///< Emulated sequence of the first copied sample.
    size_t copied;           ///< Valid Float32 values written to the caller buffer.
    size_t shortfall;        ///< Requested values unavailable for this drain.
    size_t demand;           ///< Post-drain samples needed to reach the 2,048 target.
    uint64_t overrun_count;  ///< Cumulative oldest samples discarded at capacity.
    uint64_t underrun_count; ///< Cumulative exact requested-sample shortfall.
} beeb_audio_drain_result;

/// Value-only observation of emulated progress and bounded output state.
typedef struct beeb_output_diagnostics {
    uint64_t total_cycles;           ///< Completed emulated CPU cycles.
    uint64_t latest_frame_number;    ///< Latest complete output-frame identity.
    size_t frame_depth;              ///< Complete frames currently retained.
    size_t frame_capacity;           ///< Fixed completed-frame capacity.
    size_t audio_depth;              ///< Continuous samples currently retained.
    size_t audio_capacity;           ///< Fixed continuous-sample capacity.
    size_t audio_demand;             ///< Samples needed to reach the target depth.
    uint64_t frames_produced;        ///< Complete frames offered to the FIFO.
    uint64_t frames_consumed;        ///< Complete frames transferred to callers.
    uint64_t frames_dropped;         ///< Oldest frames discarded at capacity.
    uint64_t audio_samples_produced; ///< Continuous samples offered to the FIFO.
    uint64_t audio_samples_consumed; ///< Continuous samples transferred to callers.
    uint64_t audio_samples_overrun;  ///< Oldest samples discarded at capacity.
    uint64_t audio_samples_underrun; ///< Exact requested-sample shortfall.
    beeb_status_code last_status;    ///< Latest recoverable output outcome.
} beeb_output_diagnostics;

/// Returns the immutable library version string.
/// @return Borrowed process-owned semantic-version string; never null.
const char* beeb_version_string(void);

/// Creates a paused runtime with no ROM or disc loaded.
/// @param out_machine Required output, written only on success.
/// @return `BEEB_STATUS_OK`, or `BEEB_STATUS_INVALID_ARGUMENT` when output is null;
/// `BEEB_STATUS_RESOURCE_EXHAUSTED` if allocation fails.
beeb_status beeb_create(beeb_machine** out_machine);

/// Stops acceptance, drains accepted commands, joins, and releases a runtime.
/// @param machine Live token from `beeb_create()`; null is invalid.
/// @return `BEEB_STATUS_OK`, or `BEEB_STATUS_INVALID_ARGUMENT` for null/already
/// destroyed tokens, `BEEB_STATUS_RESOURCE_EXHAUSTED` when shutdown resources
/// cannot be obtained, or `BEEB_STATUS_INTERNAL_FAILURE` when an unexpected
/// shutdown failure is contained. Concurrent destroy calls share one result.
beeb_status beeb_destroy(beeb_machine* machine);

/// Reads lifecycle state from one FIFO safe point.
/// @param machine Live runtime token.
/// @param out_state Required output, written only on success.
/// @return `BEEB_STATUS_OK`, `BEEB_STATUS_INVALID_ARGUMENT` for a bad token or
/// output, `BEEB_STATUS_RESOURCE_EXHAUSTED` on allocation failure, or
/// `BEEB_STATUS_UNAVAILABLE` while shutting down.
beeb_status beeb_get_runtime_state(beeb_machine* machine, beeb_runtime_state* out_state);

/// Starts sustained deterministic execution; running is an idempotent success.
/// @param machine Live runtime token.
/// @return `BEEB_STATUS_OK`, `BEEB_STATUS_INVALID_ARGUMENT` for a bad token,
/// `BEEB_STATUS_INVALID_STATE` when faulted, `BEEB_STATUS_RESOURCE_EXHAUSTED`
/// on command allocation failure, or `BEEB_STATUS_UNAVAILABLE` during shutdown.
beeb_status beeb_start(beeb_machine* machine);

/// Pauses at a safe point; paused is an idempotent success.
/// @param machine Live runtime token.
/// @return `BEEB_STATUS_OK`, `BEEB_STATUS_INVALID_ARGUMENT` for a bad token,
/// `BEEB_STATUS_INVALID_STATE` when the transition is not legal,
/// `BEEB_STATUS_RESOURCE_EXHAUSTED` on command allocation failure, or
/// `BEEB_STATUS_UNAVAILABLE` during shutdown.
beeb_status beeb_pause(beeb_machine* machine);

/// Resets CPU and devices, clears a fault, retains media, and finishes paused.
/// @param machine Live runtime token.
/// @return `BEEB_STATUS_OK`, `BEEB_STATUS_INVALID_ARGUMENT` for a bad token,
/// `BEEB_STATUS_RESOURCE_EXHAUSTED` on command allocation failure, or
/// `BEEB_STATUS_UNAVAILABLE` during shutdown.
beeb_status beeb_reset(beeb_machine* machine);

/// Executes whole instructions while paused until the cycle budget is met.
/// @param machine Live runtime token.
/// @param cycles Minimum CPU-cycle budget; zero performs no work.
/// @param out_actual_cycles Required output, written only on success.
/// @return `BEEB_STATUS_OK`, `BEEB_STATUS_INVALID_ARGUMENT` for bad output, or
/// `BEEB_STATUS_INVALID_STATE` unless paused, `BEEB_STATUS_EXECUTION_FAILED`
/// for an emulated fault, `BEEB_STATUS_RESOURCE_EXHAUSTED` on allocation
/// failure, or `BEEB_STATUS_UNAVAILABLE` during shutdown.
beeb_status beeb_run_cycles(beeb_machine* machine, uint64_t cycles, uint64_t* out_actual_cycles);

/// Executes while paused until a frame completes or the budget is met.
/// @param machine Live runtime token.
/// @param maximum_cycles Maximum CPU-cycle budget.
/// @param out_completed Required output receiving non-zero when a frame completed.
/// @return `BEEB_STATUS_OK`, `BEEB_STATUS_INVALID_ARGUMENT` for bad input, or
/// `BEEB_STATUS_INVALID_STATE` when execution is not paused, or
/// `BEEB_STATUS_EXECUTION_FAILED` if the emulated CPU faults,
/// `BEEB_STATUS_RESOURCE_EXHAUSTED` on allocation failure, or
/// `BEEB_STATUS_UNAVAILABLE` during shutdown.
beeb_status beeb_run_until_frame(beeb_machine* machine, uint64_t maximum_cycles,
                                 int* out_completed);

/// Copies and installs an exact 16 KiB operating-system ROM.
/// @param machine Live runtime token.
/// @param bytes Required readable source; copied during the call.
/// @param count Byte count, which must be exactly 16,384.
/// @return `BEEB_STATUS_OK`, or `BEEB_STATUS_INVALID_ARGUMENT` for a null or
/// incorrectly sized image, or `BEEB_STATUS_INVALID_STATE` if media setup is
/// rejected by the current lifecycle, `BEEB_STATUS_RESOURCE_EXHAUSTED` if the
/// copy cannot be allocated, or `BEEB_STATUS_UNAVAILABLE` during shutdown.
beeb_status beeb_load_os_rom(beeb_machine* machine, const uint8_t* bytes, size_t count);

/// Copies and installs one sideways-ROM bank.
/// @param machine Live runtime token.
/// @param bank Bank in the inclusive range 0...15.
/// @param bytes Required readable source; copied during the call.
/// @param count Byte count in the inclusive range 1...16,384.
/// @return `BEEB_STATUS_OK`, or `BEEB_STATUS_INVALID_ARGUMENT` for a bad bank,
/// pointer, or size, `BEEB_STATUS_INVALID_STATE` when faulted,
/// `BEEB_STATUS_RESOURCE_EXHAUSTED` if copying fails, or
/// `BEEB_STATUS_UNAVAILABLE` during shutdown.
beeb_status beeb_load_sideways_rom(beeb_machine* machine, uint8_t bank, const uint8_t* bytes,
                                   size_t count);

/// Copies and mounts one SSD- or DSD-layout disc image.
/// @param machine Live runtime token.
/// @param drive Drive zero or one.
/// @param bytes Required readable image source; copied during the call.
/// @param count Complete image byte count.
/// @param double_sided Non-zero for interleaved DSD, zero for SSD.
/// @param writable Non-zero to permit writes to the private copy.
/// @return `BEEB_STATUS_OK`, or `BEEB_STATUS_INVALID_ARGUMENT` for bad geometry,
/// drive, pointer, or size, `BEEB_STATUS_INVALID_STATE` when faulted,
/// `BEEB_STATUS_RESOURCE_EXHAUSTED` if copying fails, or
/// `BEEB_STATUS_UNAVAILABLE` during shutdown.
beeb_status beeb_mount_disc(beeb_machine* machine, unsigned drive, const uint8_t* bytes,
                            size_t count, int double_sided, int writable);

/// Copies CPU registers and cycle count from one safe point.
/// @param machine Live runtime token.
/// @param out_state Required output, written only on success.
/// @return `BEEB_STATUS_OK`, `BEEB_STATUS_INVALID_ARGUMENT` for a bad token or
/// output, `BEEB_STATUS_RESOURCE_EXHAUSTED` on command allocation failure, or
/// `BEEB_STATUS_UNAVAILABLE` during shutdown.
beeb_status beeb_get_cpu_state(beeb_machine* machine, beeb_cpu_state* out_state);

/// Copies the latest frame into caller-owned storage.
/// @param machine Live runtime token.
/// @param out_frame Required output, written only on success and subsequently
/// released with `beeb_frame_release()`.
/// @return `BEEB_STATUS_OK`, `BEEB_STATUS_INVALID_ARGUMENT` for a bad token or
/// output, `BEEB_STATUS_RESOURCE_EXHAUSTED` when command or frame storage cannot
/// be allocated, or `BEEB_STATUS_UNAVAILABLE` during shutdown.
beeb_status beeb_get_frame(beeb_machine* machine, beeb_frame* out_frame);

/// Transfers the oldest retained completed frame into caller-owned storage.
/// Frames are published only after a complete instruction and its aggregate
/// device tick. The queue retains at most three frames and drops exactly the
/// oldest unconsumed frame on overflow; successful values never alias producer
/// storage and remain unchanged while emulation continues.
/// @param machine Live runtime token.
/// @param out_frame Required zero-initialized output, written only on success and
/// subsequently released with `beeb_frame_release()`.
/// @return `BEEB_STATUS_OK`, `BEEB_STATUS_EMPTY` when no frame is retained,
/// `BEEB_STATUS_INVALID_ARGUMENT` for a bad token, null output, or unreleased
/// output value, `BEEB_STATUS_INVALID_STATE` when faulted,
/// `BEEB_STATUS_RESOURCE_EXHAUSTED` when command or frame storage cannot be
/// allocated, or `BEEB_STATUS_UNAVAILABLE` during shutdown.
beeb_status beeb_dequeue_frame(beeb_machine* machine, beeb_frame* out_frame);

/// Releases one frame value and clears all of its fields.
/// @param frame Required frame previously initialized to zero or returned by
/// `beeb_get_frame()` or `beeb_dequeue_frame()`; null is invalid.
/// @return `BEEB_STATUS_OK`, or `BEEB_STATUS_INVALID_ARGUMENT` for an invalid
/// frame pointer/value.
beeb_status beeb_frame_release(beeb_frame* frame);

/// Renders mono audio synchronously without advancing CPU time.
/// @param machine Live runtime token.
/// @param mono Required writable storage for `frames` samples when frames is nonzero.
/// @param frames Number of samples; zero is permitted with a non-null pointer.
/// @param sample_rate Finite positive sample rate in hertz.
/// @return `BEEB_STATUS_OK`, `BEEB_STATUS_INVALID_ARGUMENT` for a bad token,
/// rate, frame count, or buffer, `BEEB_STATUS_INVALID_STATE` when faulted,
/// `BEEB_STATUS_RESOURCE_EXHAUSTED` on command or sample allocation failure, or
/// `BEEB_STATUS_UNAVAILABLE` during shutdown; output changes only on success.
beeb_status beeb_render_audio(beeb_machine* machine, float* mono, size_t frames,
                              double sample_rate);

/// Drains continuous 48 kHz mono Float32 output into caller storage.
/// The runtime retains at most 4,096 samples and reports demand against a 2,048
/// target. Producer overflow drops oldest samples; consumer underrun copies all
/// available samples and reports the exact remainder. Host time is not an input
/// to this operation or to core production.
/// @param machine Live runtime token.
/// @param mono Writable storage for `capacity` samples; required when capacity is nonzero.
/// @param capacity Maximum samples to copy and the requested drain count.
/// @param out_result Required metadata, written with the buffer only on OK or UNDERRUN.
/// @return `BEEB_STATUS_OK` when all requested samples were copied,
/// `BEEB_STATUS_UNDERRUN` with a valid partial copy and exact shortfall,
/// `BEEB_STATUS_INVALID_ARGUMENT` for bad pointers, `BEEB_STATUS_INVALID_STATE`
/// when faulted, `BEEB_STATUS_RESOURCE_EXHAUSTED` if result storage cannot be
/// allocated, or `BEEB_STATUS_UNAVAILABLE` during shutdown.
beeb_status beeb_drain_audio(beeb_machine* machine, float* mono, size_t capacity,
                             beeb_audio_drain_result* out_result);

/// Copies one owner-consistent bounded-output diagnostic observation.
/// @param machine Live runtime token.
/// @param out_diagnostics Required output, written only on success.
/// @return `BEEB_STATUS_OK`, `BEEB_STATUS_INVALID_ARGUMENT` for a bad token or
/// output, `BEEB_STATUS_RESOURCE_EXHAUSTED` on command allocation failure, or
/// `BEEB_STATUS_UNAVAILABLE` during shutdown.
beeb_status beeb_get_output_diagnostics(beeb_machine* machine,
                                        beeb_output_diagnostics* out_diagnostics);

/// Calculates informational emulation speed from two host observations.
/// @param before Earlier diagnostic observation.
/// @param after Later diagnostic observation with a nondecreasing cycle count.
/// @param host_seconds Positive finite elapsed host-observation seconds.
/// @param out_rate Required output receiving emulated seconds per host second.
/// @return `BEEB_STATUS_OK`, or `BEEB_STATUS_INVALID_ARGUMENT` for null pointers,
/// regressing cycles, a non-positive/non-finite interval, or non-finite result.
beeb_status beeb_calculate_emulation_rate(const beeb_output_diagnostics* before,
                                          const beeb_output_diagnostics* after, double host_seconds,
                                          double* out_rate);

/// Changes one keyboard-matrix bit in FIFO order.
/// @param machine Live runtime token.
/// @param column Matrix column in the inclusive range 0...15.
/// @param row Matrix row in the inclusive range 0...15.
/// @param pressed Non-zero to press, zero to release.
/// @return `BEEB_STATUS_OK`, `BEEB_STATUS_INVALID_ARGUMENT` for a bad token or
/// coordinates, `BEEB_STATUS_INVALID_STATE` when faulted,
/// `BEEB_STATUS_RESOURCE_EXHAUSTED` on command allocation failure, or
/// `BEEB_STATUS_UNAVAILABLE` during shutdown.
beeb_status beeb_set_key(beeb_machine* machine, uint8_t column, uint8_t row, int pressed);

/// Changes BREAK state in FIFO order without inventing a lifecycle transition.
/// @param machine Live runtime token.
/// @param pressed Non-zero to press, zero to release.
/// @return `BEEB_STATUS_OK`, `BEEB_STATUS_INVALID_ARGUMENT` for a bad token,
/// `BEEB_STATUS_INVALID_STATE` when faulted, `BEEB_STATUS_RESOURCE_EXHAUSTED`
/// on command allocation failure, or `BEEB_STATUS_UNAVAILABLE` during shutdown.
beeb_status beeb_set_break(beeb_machine* machine, int pressed);

/// Reads the current safe-point identity in FIFO order.
/// @param machine Live runtime token.
/// @param out_safe_point Required output, written only on success.
/// @return `BEEB_STATUS_OK`, `BEEB_STATUS_INVALID_ARGUMENT` for a bad token or
/// output, `BEEB_STATUS_RESOURCE_EXHAUSTED` on command allocation failure, or
/// `BEEB_STATUS_UNAVAILABLE` during shutdown.
beeb_status beeb_get_safe_point(beeb_machine* machine, beeb_safe_point* out_safe_point);

/// Reads retained execution-fault detail; absence is a successful value.
/// @param machine Live runtime token.
/// @param out_fault Required output, written only on success.
/// @return `BEEB_STATUS_OK`, `BEEB_STATUS_INVALID_ARGUMENT` for a bad token or
/// output, `BEEB_STATUS_RESOURCE_EXHAUSTED` on command or result allocation
/// failure, or `BEEB_STATUS_UNAVAILABLE` during shutdown.
beeb_status beeb_get_fault(beeb_machine* machine, beeb_fault_detail* out_fault);

#ifdef __cplusplus
}
#endif
