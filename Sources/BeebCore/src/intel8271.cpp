#include "beeb/intel8271.hpp"

#include <algorithm>
#include <utility>

namespace beeb {

void Intel8271::reset() {
    special_.fill(0);
    currentTrack_.fill(0);
    parameters_.clear();
    transferBuffer_.clear();
    status_ = result_ = command_ = expectedParameters_ = 0;
    transferIndex_ = countdown_ = 0;
    specifyTail_ = false;
    transfer_ = Transfer::None;
}

bool Intel8271::mount(unsigned drive, std::span<const std::uint8_t> bytes, DiscImage::Layout layout,
                      bool writable) {
    return drive < drives_.size() && drives_[drive].load(bytes, layout, writable);
}

unsigned Intel8271::selectedDrive() const noexcept {
    if ((command_ & 0x80) != 0) return 1;
    return 0;
}

std::uint8_t Intel8271::read(std::uint8_t reg) {
    switch (reg & 0x07) {
    case 0:
        return status_;
    case 1: {
        const auto value = result_;
        status_ &= static_cast<std::uint8_t>(~(ResultReady | NMI));
        return value;
    }
    case 4:
    case 5:
    case 6:
    case 7: {
        if (transfer_ != Transfer::Read || transferIndex_ >= transferBuffer_.size()) return 0xFF;
        const auto value = transferBuffer_[transferIndex_++];
        status_ &= static_cast<std::uint8_t>(~(NeedData | NMI));
        if (transferIndex_ >= transferBuffer_.size()) {
            countdown_ = 64;
        } else {
            countdown_ = 64;
        }
        return value;
    }
    default:
        return 0xFF;
    }
}

void Intel8271::write(std::uint8_t reg, std::uint8_t value) {
    switch (reg & 0x07) {
    case 0:
        command(value);
        break;
    case 1:
        parameter(value);
        break;
    case 2:
        if (value != 0) reset();
        break;
    case 4:
    case 5:
    case 6:
    case 7:
        if (transfer_ == Transfer::Write && transferIndex_ < transferBuffer_.size()) {
            transferBuffer_[transferIndex_++] = value;
            status_ &= static_cast<std::uint8_t>(~(NeedData | NMI));
            countdown_ = 64;
        }
        break;
    default:
        break;
    }
}

void Intel8271::command(std::uint8_t value) {
    command_ = value;
    parameters_.clear();
    transferBuffer_.clear();
    transfer_ = Transfer::None;
    transferIndex_ = 0;
    result_ = 0;
    status_ = Busy;
    specifyTail_ = false;
    expectedParameters_ = (value & 0x18) != 0 ? static_cast<std::uint8_t>(value & 0x03) : 5;
    if (expectedParameters_ == 0) startCommand();
}

void Intel8271::parameter(std::uint8_t value) {
    status_ &= static_cast<std::uint8_t>(~ResultReady);
    if (expectedParameters_ == 0) return;
    parameters_.push_back(value);
    if (--expectedParameters_ != 0) return;
    if ((command_ & 0x3C) == 0x34 && !specifyTail_) {
        // SPECIFY's first parameter selects an internal register; three values follow.
        specifyTail_ = true;
        expectedParameters_ = 3;
        return;
    }
    startCommand();
}

void Intel8271::startCommand() {
    switch (command_ & 0x3C) {
    case 0x08:
        startWrite();
        break;
    case 0x10:
        startRead();
        break;
    case 0x18: // verify
        startRead();
        if (transfer_ == Transfer::Read) {
            transfer_ = Transfer::None;
            countdown_ = 64;
        }
        break;
    case 0x28: // seek
        if (!parameters_.empty()) currentTrack_[selectedDrive()] = parameters_[0];
        finish(0, false);
        break;
    case 0x2C: { // read drive status
        const auto drive = selectedDrive();
        result_ =
            drives_[drive].present() ? static_cast<std::uint8_t>(drive == 0 ? 0x04 : 0x40) : 0;
        finish(result_, false);
        break;
    }
    case 0x34: { // specify
        if (parameters_.size() >= 4) {
            const auto start = parameters_[0] & 0x3F;
            for (unsigned i = 0; i < 3 && start + i < special_.size(); ++i)
                special_[start + i] = parameters_[1 + i];
        }
        finish(0, false);
        break;
    }
    case 0x38: // write special register
        if (parameters_.size() >= 2 && parameters_[0] < special_.size())
            special_[parameters_[0]] = parameters_[1];
        finish(0, false);
        break;
    case 0x3C: // read special register
        result_ = (!parameters_.empty() && parameters_[0] < special_.size())
                      ? special_[parameters_[0]]
                      : 0;
        finish(result_, false);
        break;
    default:
        finish(0x18, true); // unsupported / sector not found
        break;
    }
}

void Intel8271::startRead() {
    if (parameters_.size() < 3) {
        finish(0x18, true);
        return;
    }
    transferDrive_ = selectedDrive();
    transferTrack_ = parameters_[0];
    transferSector_ = parameters_[1];
    transferSectorSize_ = 128u << (parameters_[2] >> 5);
    transferSectorCount_ = parameters_[2] & 0x1F;
    if (transferSectorCount_ == 0) transferSectorCount_ = 1;
    transferSide_ = (special_[0x23] & 0x20) != 0 ? 1u : 0u;
    if (transferSectorSize_ > 256 || !drives_[transferDrive_].present()) {
        finish(0x10, true);
        return;
    }

    transferBuffer_.assign(transferSectorSize_ * transferSectorCount_, 0);
    for (unsigned index = 0; index < transferSectorCount_; ++index) {
        auto sector =
            std::span(transferBuffer_).subspan(index * transferSectorSize_, transferSectorSize_);
        if (!drives_[transferDrive_].readSector(transferTrack_, transferSide_,
                                                transferSector_ + index, sector)) {
            finish(0x18, true);
            return;
        }
    }
    transferIndex_ = 0;
    transfer_ = Transfer::Read;
    countdown_ = 64;
}

void Intel8271::startWrite() {
    if (parameters_.size() < 3) {
        finish(0x18, true);
        return;
    }
    transferDrive_ = selectedDrive();
    transferTrack_ = parameters_[0];
    transferSector_ = parameters_[1];
    transferSectorSize_ = 128u << (parameters_[2] >> 5);
    transferSectorCount_ = parameters_[2] & 0x1F;
    if (transferSectorCount_ == 0) transferSectorCount_ = 1;
    transferSide_ = (special_[0x23] & 0x20) != 0 ? 1u : 0u;
    if (transferSectorSize_ > 256 || !drives_[transferDrive_].writable()) {
        finish(0x12, true);
        return;
    }
    transferBuffer_.assign(transferSectorSize_ * transferSectorCount_, 0);
    transferIndex_ = 0;
    transfer_ = Transfer::Write;
    countdown_ = 64;
}

void Intel8271::requestTransferByte() {
    if (transfer_ == Transfer::Read && transferIndex_ >= transferBuffer_.size()) {
        finish(0, true);
        return;
    }
    if (transfer_ == Transfer::Write && transferIndex_ >= transferBuffer_.size()) {
        commitWrite();
        return;
    }
    status_ |= static_cast<std::uint8_t>(NMI | NeedData);
    if (nmi_) nmi_();
}

void Intel8271::commitWrite() {
    for (unsigned index = 0; index < transferSectorCount_; ++index) {
        const auto sector = std::span<const std::uint8_t>(transferBuffer_)
                                .subspan(index * transferSectorSize_, transferSectorSize_);
        if (!drives_[transferDrive_].writeSector(transferTrack_, transferSide_,
                                                 transferSector_ + index, sector)) {
            finish(0x12, true);
            return;
        }
    }
    finish(0, true);
}

void Intel8271::finish(std::uint8_t result, bool interrupt) {
    result_ = result;
    transfer_ = Transfer::None;
    countdown_ = 0;
    status_ &= static_cast<std::uint8_t>(~(Busy | NeedData | NMI));
    status_ |= ResultReady;
    if (interrupt) {
        status_ |= NMI;
        if (nmi_) nmi_();
    }
}

void Intel8271::tick(std::uint32_t cpuCycles) {
    if (countdown_ == 0) return;
    if (cpuCycles >= countdown_) {
        countdown_ = 0;
        requestTransferByte();
    } else {
        countdown_ -= cpuCycles;
    }
}

} // namespace beeb
