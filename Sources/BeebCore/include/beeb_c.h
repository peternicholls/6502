#pragma once

#include "beeb/version.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/// Opaque owner of one independent emulator instance.
///
/// Create with beeb_create() and release with beeb_destroy(). The API does not
/// synchronize access; callers must serialize all operations on an instance.
typedef struct beeb_machine beeb_machine;

/// Portable snapshot of the emulated 6502 programmer-visible state.
typedef struct beeb_cpu_state {
    uint8_t a;       ///< Accumulator register.
    uint8_t x;       ///< X index register.
    uint8_t y;       ///< Y index register.
    uint8_t sp;      ///< Stack pointer register.
    uint8_t p;       ///< Processor status flags.
    uint16_t pc;     ///< Program counter.
    uint64_t cycles; ///< Total CPU cycles executed by this machine.
} beeb_cpu_state;

/// Returns the immutable library version string.
///
/// The process-owned, null-terminated string remains valid for the lifetime of
/// the loaded library and must not be freed.
const char* beeb_version_string(void);

/// Creates an emulator instance with no ROM or disc loaded.
///
/// @return An owned machine, or `NULL` when allocation or construction fails.
beeb_machine* beeb_create(void);

/// Destroys an emulator instance and invalidates all pointers obtained from it.
///
/// @param machine Owned instance from beeb_create(), or `NULL` for no action.
void beeb_destroy(beeb_machine* machine);

/// Returns the machine's most recent error message.
///
/// The borrowed, null-terminated string is empty when no error is present and
/// remains valid only until the next operation on `machine` or its destruction.
///
/// @param machine Machine to inspect, or `NULL`.
/// @return A borrowed string, or `NULL` when `machine` is `NULL`.
const char* beeb_last_error(const beeb_machine* machine);

/// Copies a 16 KiB operating-system ROM into the machine.
///
/// The input can be released as soon as this call returns. The operation clears
/// the previous error before validation and records a diagnostic on failure.
///
/// @param machine Machine to modify; `NULL` causes failure.
/// @param bytes Readable ROM bytes; must be non-null when called.
/// @param count Byte count; must equal 16,384.
/// @return `1` on success, otherwise `0`; inspect beeb_last_error() for details
/// when `machine` is non-null.
int beeb_load_os_rom(beeb_machine* machine, const uint8_t* bytes, size_t count);

/// Copies a sideways ROM into one of the 16 banks.
///
/// Unused bytes in the selected bank are filled with `0xFF`. The input can be
/// released when this call returns.
///
/// @param machine Machine to modify; `NULL` causes failure.
/// @param bank Sideways-ROM bank in the inclusive range 0...15.
/// @param bytes Readable ROM bytes; must be non-null when called.
/// @param count Byte count in the inclusive range 0...16,384.
/// @return `1` on success, otherwise `0`; inspect beeb_last_error() for details.
int beeb_load_sideways_rom(beeb_machine* machine, uint8_t bank, const uint8_t* bytes, size_t count);

/// Copies and mounts an SSD- or DSD-layout disc image.
///
/// The image storage becomes machine-owned and the input can be released when
/// this call returns. A writable mount changes only that private copy.
///
/// @param machine Machine to modify; `NULL` causes failure.
/// @param drive Drive number, 0 or 1.
/// @param bytes Readable image bytes; must be non-null when called.
/// @param count Image byte count for 1...80 tracks of ten 256-byte sectors.
/// @param double_sided Non-zero for interleaved DSD layout; zero for SSD.
/// @param writable Non-zero to allow writes to the machine-owned image copy.
/// @return `1` on success, otherwise `0`; inspect beeb_last_error() for details.
int beeb_mount_disc(beeb_machine* machine, unsigned drive, const uint8_t* bytes, size_t count,
                    int double_sided, int writable);

/// Resets CPU and device state while retaining loaded media.
///
/// @param machine Machine to reset; `NULL` causes no action.
void beeb_reset(beeb_machine* machine);

/// Advances the machine by at least the requested CPU-cycle budget.
///
/// An instruction already in progress is completed, so the returned count may
/// exceed `cycles`. Device time advances with the executed instruction cycles.
///
/// @param machine Machine to run; `NULL` causes failure.
/// @param cycles Minimum CPU-cycle budget; zero performs no work.
/// @return Actual cycles executed, or `0` if no work was requested or an error
/// occurred. Inspect beeb_last_error() to distinguish an error.
uint64_t beeb_run_cycles(beeb_machine* machine, uint64_t cycles);

/// Runs until a new video frame completes or the cycle limit is reached.
///
/// @param machine Machine to run; `NULL` causes failure.
/// @param maximum_cycles Maximum cycle budget before returning without a frame.
/// @return `1` when a frame completed, `0` at the limit, or `-1` on error.
int beeb_run_until_frame(beeb_machine* machine, uint64_t maximum_cycles);

/// Copies the current CPU state into a value with no borrowed storage.
///
/// @param machine Machine to inspect, or `NULL`.
/// @return Current state, or an all-zero state when `machine` is `NULL`.
beeb_cpu_state beeb_get_cpu_state(const beeb_machine* machine);

/// Borrows the most recently rendered frame as packed 8-bit RGBA pixels.
///
/// Each optional metadata output may be `NULL`. The returned bytes remain owned
/// by the machine and are valid only until the next non-const machine operation
/// or destruction. Callers that need a stable frame must copy them immediately.
///
/// @param machine Machine to inspect, or `NULL`.
/// @param width Optional output for pixel width.
/// @param height Optional output for pixel height.
/// @param number Optional output for the monotonically increasing frame number.
/// @return Borrowed RGBA bytes, or `NULL` for a null machine or absent frame.
const uint8_t* beeb_get_frame_rgba(const beeb_machine* machine, uint32_t* width, uint32_t* height, uint64_t* number);

/// Renders mono floating-point audio from the current sound-generator state.
///
/// This advances the sound generator's sampling phase but not machine CPU time.
/// Errors are reported through beeb_last_error().
///
/// @param machine Machine whose sound state is used; `NULL` causes no action.
/// @param mono Writable buffer for exactly `frames` samples; must be non-null.
/// @param frames Number of samples to write; zero is permitted.
/// @param sample_rate Finite, positive sample rate in hertz.
void beeb_render_audio(beeb_machine* machine, float* mono, size_t frames, double sample_rate);

/// Changes one key in the emulated keyboard matrix.
///
/// Coordinates outside columns 0...15 or rows 0...15 are ignored.
///
/// @param machine Machine to modify; `NULL` causes no action.
/// @param column Keyboard matrix column.
/// @param row Keyboard matrix row.
/// @param pressed Non-zero to press the key, zero to release it.
void beeb_set_key(beeb_machine* machine, uint8_t column, uint8_t row, int pressed);

/// Changes the emulated BREAK key state.
///
/// A released-to-pressed transition resets the machine; holding BREAK does not
/// repeatedly reset it.
///
/// @param machine Machine to modify; `NULL` causes no action.
/// @param pressed Non-zero to press BREAK, zero to release it.
void beeb_set_break(beeb_machine* machine, int pressed);

#ifdef __cplusplus
}
#endif
