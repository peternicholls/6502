#pragma once

#include "beeb/disc_image.hpp"

#include <array>
#include <cstdint>
#include <functional>
#include <span>
#include <vector>

namespace beeb {

class Intel8271 {
public:
    using NMICallback = std::function<void()>;

    void reset();
    void setNMICallback(NMICallback callback) { nmi_ = std::move(callback); }
    bool mount(unsigned drive, std::span<const std::uint8_t> bytes, DiscImage::Layout layout, bool writable = false);
    [[nodiscard]] const DiscImage& disc(unsigned drive) const { return drives_[drive % 2]; }

    std::uint8_t read(std::uint8_t reg);
    void write(std::uint8_t reg, std::uint8_t value);
    void tick(std::uint32_t cpuCycles);

    [[nodiscard]] std::uint8_t status() const noexcept { return status_; }
    [[nodiscard]] std::uint8_t result() const noexcept { return result_; }

private:
    enum Status : std::uint8_t {
        Busy = 0x80,
        CommandFull = 0x40,
        ParameterFull = 0x20,
        ResultReady = 0x10,
        NMI = 0x08,
        NeedData = 0x04,
    };
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
