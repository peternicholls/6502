#pragma once

// C0-DOC-RATIONALE: docs/code/timing-model.md owns controller timing limits.

#include "beeb/disc_image.hpp"

#include <array>
#include <cstdint>
#include <functional>
#include <span>
#include <vector>

namespace beeb {

/// Command, status, transfer, and timing model of the Intel 8271 FDC.
/// One emulated machine owns each instance and serializes host-register writes
/// with ticking. The owned NMI callback runs synchronously on that caller's
/// thread; callback exceptions propagate and the callable remains installed
/// until replaced or destruction.
class Intel8271 {
  public:
    /// Observer invoked when the controller asserts its NMI condition.
    using NMICallback = std::function<void()>;

    /// Clears command and transfer state while retaining mounted images.
    void reset();
    /// Replaces the callback used to signal a controller NMI.
    /// @param callback Observer to own, or an empty function to disable it.
    void setNMICallback(NMICallback callback) { nmi_ = std::move(callback); }
    /// Validates, copies, and mounts an image in drive 0 or 1.
    /// @param drive Drive number; values above one fail.
    /// @param bytes Complete image bytes; the span is not retained.
    /// @param layout SSD or DSD byte ordering.
    /// @param writable Whether controller writes may modify the private image.
    /// @return Whether the drive and image were valid.
    bool mount(unsigned drive, std::span<const std::uint8_t> bytes, DiscImage::Layout layout,
               bool writable = false);
    /// Returns one of the two mounted image slots.
    /// @param drive Drive selector; values are reduced modulo two.
    /// @return Borrowed drive image valid for this controller's lifetime.
    [[nodiscard]] const DiscImage& disc(unsigned drive) const { return drives_[drive % 2]; }

    /// Reads an 8271 host register, applying its documented read side effects.
    /// @param reg Register offset; the implementation decodes the low bits.
    /// @return Host-visible register value.
    std::uint8_t read(std::uint8_t reg);
    /// Writes an 8271 host register and may begin or feed a command.
    /// @param reg Register offset.
    /// @param value Value presented by the host.
    void write(std::uint8_t reg, std::uint8_t value);
    /// Advances command latency and byte-transfer timing.
    /// @param cpuCycles Elapsed cycles on the BBC Micro 2 MHz CPU timebase.
    void tick(std::uint32_t cpuCycles);

    /// Returns the current host-visible status register.
    /// @return Status bit field.
    [[nodiscard]] std::uint8_t status() const noexcept { return status_; }
    /// Returns the current host-visible command result.
    /// @return Result byte.
    [[nodiscard]] std::uint8_t result() const noexcept { return result_; }

  private:
    /// Host-visible readiness and interrupt indications maintained by commands.
    enum Status : std::uint8_t {
        Busy = 0x80,
        CommandFull = 0x40,
        ParameterFull = 0x20,
        ResultReady = 0x10,
        NMI = 0x08,
        NeedData = 0x04,
    };
    /// Mutually exclusive sector-stream phase; None means no transfer active.
    enum class Transfer { None, Read, Write };

    std::array<DiscImage, 2> drives_;
    std::array<std::uint8_t, 64> special_{};
    std::array<std::uint8_t, 2> currentTrack_{};
    std::vector<std::uint8_t> parameters_;
    std::vector<std::uint8_t> transferBuffer_;
    NMICallback nmi_;
    std::uint8_t status_ = 0;
    std::uint8_t result_ = 0;
    std::uint8_t command_ = 0;
    std::uint8_t expectedParameters_ = 0;
    std::size_t transferIndex_ = 0;
    std::uint32_t countdown_ = 0;
    unsigned transferDrive_ = 0;
    unsigned transferTrack_ = 0;
    unsigned transferSide_ = 0;
    unsigned transferSector_ = 0;
    unsigned transferSectorSize_ = 0;
    unsigned transferSectorCount_ = 0;
    bool specifyTail_ = false;
    Transfer transfer_ = Transfer::None;

    void command(std::uint8_t value);
    void parameter(std::uint8_t value);
    void startCommand();
    void startRead();
    void startWrite();
    void requestTransferByte();
    void finish(std::uint8_t result, bool interrupt);
    void commitWrite();
    [[nodiscard]] unsigned selectedDrive() const noexcept;
};

} // namespace beeb
