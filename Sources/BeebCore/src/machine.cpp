#include "beeb/machine.hpp"

#include <algorithm>
#include <bit>
#include <cstring>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace beeb {

BBCMicro::BBCMicro() : BBCMicro(MachineTargetProfile::modelB()) {}

BBCMicro::BBCMicro(MachineTargetProfile profile) : profile_(std::move(profile)), cpu_(*this) {
    if (validateMachineTargetProfile(profile_).support != ProfileSupport::supported)
        throw std::invalid_argument("machine profile is not supported");
    // This wiring is the BBC Model B collaboration boundary: System VIA owns keyboard
    // scanning and IC32/sound outputs, the FDC can raise NMI, and all devices advance
    // from CPU cycles through tick(). Address decoding below intentionally preserves
    // mirrored I/O, open-bus reads, ROM selection, and ignored ROM writes.
    ram_.fill(0);
    osROM_.fill(0xFF);
    for (auto& bank : sidewaysROM_)
        bank.fill(0xFF);
    sidewaysPresent_.fill(false);
    keyboard_.fill(0);
    ic32_.fill(false);
    configureSystemVIA();
    fdc_.setNMICallback([this] { cpu_.requestNMI(); });
    reset();
}

void BBCMicro::configureSystemVIA() {
    systemVIA_.setPortAInput([this] { return keyboardPortA(); });
    systemVIA_.setPortAOutput([this](std::uint8_t value, std::uint8_t ddr) {
        if (!ic32_[0] && ddr != 0) sound_.write(value);
    });
    systemVIA_.setPortBOutput(
        [this](std::uint8_t value, std::uint8_t ddr) { updateIC32(value, ddr); });
}

void BBCMicro::reset() {
    selectedROM_ = 0;
    viaRemainder_ = crtcRemainder_ = 0;
    ic32_.fill(false);
    systemVIA_.reset();
    userVIA_.reset();
    crtc_.reset();
    videoULA_.reset();
    sound_.reset();
    fdc_.reset();
    frame_ = {};
    cpu_.setState({});
    cpu_.reset();
}

bool BBCMicro::loadOSROM(std::span<const std::uint8_t> rom) {
    if (rom.size() != osROM_.size()) return false;
    std::copy(rom.begin(), rom.end(), osROM_.begin());
    osROMLoaded_ = true;
    return true;
}

bool BBCMicro::loadSidewaysROM(std::uint8_t bank, std::span<const std::uint8_t> rom) {
    if (bank >= sidewaysROM_.size() || rom.size() > sidewaysROM_[bank].size()) return false;
    sidewaysROM_[bank].fill(0xFF);
    std::copy(rom.begin(), rom.end(), sidewaysROM_[bank].begin());
    sidewaysPresent_[bank] = true;
    return true;
}

bool BBCMicro::loadRAM(std::uint16_t address, std::span<const std::uint8_t> bytes) {
    if (address >= ram_.size() || bytes.size() > ram_.size() - address) return false;
    std::copy(bytes.begin(), bytes.end(), ram_.begin() + address);
    return true;
}

void BBCMicro::restoreRAM(std::span<const std::uint8_t> bytes) noexcept {
    if (bytes.size() == ram_.size()) std::copy(bytes.begin(), bytes.end(), ram_.begin());
}

BBCMicro::Checkpoint BBCMicro::checkpoint() const {
    return {cpu_.state(),   cpu_.irqLine_, cpu_.nmiPending_, ram_,         keyboard_,
            ic32_,          systemVIA_,    userVIA_,         crtc_,        videoULA_,
            sound_,         fdc_,          frame_,           selectedROM_, viaRemainder_,
            crtcRemainder_, breakPressed_};
}

void BBCMicro::restore(Checkpoint&& checkpoint) noexcept {
    static_assert(std::is_nothrow_move_assignable_v<VIA6522>);
    static_assert(std::is_nothrow_move_assignable_v<CRTC6845>);
    static_assert(std::is_nothrow_move_assignable_v<VideoULA>);
    static_assert(std::is_nothrow_move_assignable_v<SN76489>);
    static_assert(std::is_nothrow_move_assignable_v<Intel8271>);
    static_assert(std::is_nothrow_move_assignable_v<VideoFrame>);

    cpu_.setState(checkpoint.cpu);
    cpu_.irqLine_ = checkpoint.cpuIRQ;
    cpu_.nmiPending_ = checkpoint.cpuNMI;
    ram_ = std::move(checkpoint.ram);
    keyboard_ = std::move(checkpoint.keyboard);
    ic32_ = std::move(checkpoint.ic32);
    systemVIA_ = std::move(checkpoint.systemVIA);
    userVIA_ = std::move(checkpoint.userVIA);
    crtc_ = std::move(checkpoint.crtc);
    videoULA_ = std::move(checkpoint.videoULA);
    sound_ = std::move(checkpoint.sound);
    fdc_ = std::move(checkpoint.fdc);
    frame_ = std::move(checkpoint.frame);
    selectedROM_ = checkpoint.selectedROM;
    viaRemainder_ = checkpoint.viaRemainder;
    crtcRemainder_ = checkpoint.crtcRemainder;
    breakPressed_ = checkpoint.breakPressed;
}

std::uint64_t BBCMicro::testDigest() const noexcept {
    auto digest = std::uint64_t{1469598103934665603ULL};
    const auto add = [&digest](auto value) {
        auto bits = static_cast<std::uint64_t>(value);
        for (std::size_t index = 0; index < sizeof(value); ++index) {
            digest ^= static_cast<std::uint8_t>(bits & 0xFFu);
            digest *= 1099511628211ULL;
            bits >>= 8;
        }
    };
    const auto addBytes = [&add](std::span<const std::uint8_t> bytes) {
        for (const auto byte : bytes)
            add(byte);
    };

    add(profile_.schemaVersion());
    add(profile_.base().identifier());
    add(profile_.base().version());
    add(profile_.base().reserved());
    add(profile_.expansionCount());
    for (const auto& expansion : profile_.expansions()) {
        add(expansion.identifier());
        add(expansion.version());
        add(expansion.reserved());
    }

    const auto cpu = cpu_.state();
    add(cpu.a);
    add(cpu.x);
    add(cpu.y);
    add(cpu.sp);
    add(cpu.p);
    add(cpu.pc);
    add(cpu.cycles);
    add(static_cast<std::uint8_t>(cpu_.irqLine_));
    add(static_cast<std::uint8_t>(cpu_.nmiPending_));
    addBytes(ram_);
    addBytes(osROM_);
    for (const auto& bank : sidewaysROM_)
        addBytes(bank);
    for (const auto present : sidewaysPresent_)
        add(static_cast<std::uint8_t>(present));
    add(static_cast<std::uint8_t>(osROMLoaded_));
    for (const auto row : keyboard_)
        add(row);
    for (const auto value : ic32_)
        add(static_cast<std::uint8_t>(value));
    for (std::uint8_t index = 0; index < 32; ++index)
        add(crtc_.reg(index));
    add(crtc_.selected());
    add(crtc_.horizontalCharacter());
    add(crtc_.characterRow());
    add(crtc_.rasterRow());
    add(crtc_.verticalAdjust_);
    add(crtc_.frameNumber());
    add(static_cast<std::uint8_t>(crtc_.inVerticalAdjust_));
    add(static_cast<std::uint8_t>(crtc_.frameReady()));
    for (const auto* via : {&systemVIA_, &userVIA_}) {
        add(via->outputA());
        add(via->outputB());
        add(via->ddra());
        add(via->ddrb());
        add(via->t1Counter_);
        add(via->t1Latch_);
        add(via->t2Counter_);
        add(via->t2LatchLow_);
        add(via->shift_);
        add(via->acr_);
        add(via->pcr_);
        add(via->ifr_);
        add(via->ier_);
        add(static_cast<std::uint8_t>(via->t1Running_));
        add(static_cast<std::uint8_t>(via->t2Running_));
        add(static_cast<std::uint8_t>(via->ca1_));
        add(static_cast<std::uint8_t>(via->cb1_));
        add(static_cast<std::uint8_t>(via->irq()));
    }
    add(videoULA_.control());
    for (std::uint8_t logical = 0; logical < 16; ++logical) {
        add(videoULA_.physicalColour(logical));
        add(videoULA_.physicalColour(logical, true));
    }
    for (unsigned channel = 0; channel < 3; ++channel)
        add(sound_.tonePeriod(channel));
    for (unsigned channel = 0; channel < 4; ++channel)
        add(sound_.volume(channel));
    for (const auto phase : sound_.phase_)
        add(std::bit_cast<std::uint64_t>(phase));
    add(sound_.noise_);
    add(sound_.latchedChannel_);
    add(static_cast<std::uint8_t>(sound_.latchedVolume_));
    add(sound_.lfsr_);
    add(static_cast<std::uint8_t>(sound_.noiseLevel_));
    add(fdc_.status());
    add(fdc_.result());
    for (unsigned drive = 0; drive < 2; ++drive) {
        const auto& disc = fdc_.disc(drive);
        add(static_cast<std::uint8_t>(disc.present()));
        add(static_cast<std::uint8_t>(disc.writable()));
        add(static_cast<std::uint8_t>(disc.layout_));
        add(disc.tracks());
        add(disc.sides());
        addBytes(disc.bytes());
    }
    addBytes(fdc_.special_);
    addBytes(fdc_.currentTrack_);
    addBytes(fdc_.parameters_);
    addBytes(fdc_.transferBuffer_);
    add(fdc_.command_);
    add(fdc_.expectedParameters_);
    add(fdc_.transferIndex_);
    add(fdc_.countdown_);
    add(fdc_.transferDrive_);
    add(fdc_.transferTrack_);
    add(fdc_.transferSide_);
    add(fdc_.transferSector_);
    add(fdc_.transferSectorSize_);
    add(fdc_.transferSectorCount_);
    add(static_cast<std::uint8_t>(fdc_.specifyTail_));
    add(static_cast<std::uint8_t>(fdc_.transfer_));
    add(frame_.width);
    add(frame_.height);
    add(frame_.number);
    addBytes(frame_.rgba);
    add(selectedROM_);
    add(viaRemainder_);
    add(crtcRemainder_);
    add(static_cast<std::uint8_t>(breakPressed_));
    return digest;
}

bool BBCMicro::mountDisc(unsigned drive, std::span<const std::uint8_t> bytes,
                         DiscImage::Layout layout, bool writable) {
    return fdc_.mount(drive, bytes, layout, writable);
}

std::uint8_t BBCMicro::read(std::uint16_t address) {
    if (address < 0x8000) return ram_[address];
    if (address < 0xC000) return sidewaysROM_[selectedROM_][address - 0x8000];
    if (address >= 0xFC00 && address <= 0xFEFF) return readIO(address);
    return osROM_[address - 0xC000];
}

void BBCMicro::write(std::uint16_t address, std::uint8_t value) {
    if (address < 0x8000) {
        ram_[address] = value;
        return;
    }
    if (address >= 0xFC00 && address <= 0xFEFF) writeIO(address, value);
}

std::uint8_t BBCMicro::readIO(std::uint16_t address) {
    if ((address & 0xFFF8) == 0xFE00) {
        return (address & 1) != 0 ? crtc_.read() : crtc_.selected();
    }
    if ((address & 0xFFF0) == 0xFE40 || (address & 0xFFF0) == 0xFE50) {
        return systemVIA_.read(static_cast<std::uint8_t>(address));
    }
    if ((address & 0xFFF0) == 0xFE60 || (address & 0xFFF0) == 0xFE70) {
        return userVIA_.read(static_cast<std::uint8_t>(address));
    }
    if ((address & 0xFFF0) == 0xFE30) return selectedROM_;
    if ((address & 0xFFE0) == 0xFE80) return fdc_.read(static_cast<std::uint8_t>(address));
    return 0xFF;
}

void BBCMicro::writeIO(std::uint16_t address, std::uint8_t value) {
    if ((address & 0xFFF8) == 0xFE00) {
        if ((address & 1) != 0)
            crtc_.write(value);
        else
            crtc_.select(value);
        return;
    }
    if ((address & 0xFFFE) == 0xFE20) {
        if ((address & 1) != 0)
            videoULA_.writePalette(value);
        else
            videoULA_.writeControl(value);
        return;
    }
    if ((address & 0xFFF0) == 0xFE30) {
        selectedROM_ = static_cast<std::uint8_t>(value & 0x0F);
        return;
    }
    if ((address & 0xFFF0) == 0xFE40 || (address & 0xFFF0) == 0xFE50) {
        systemVIA_.write(static_cast<std::uint8_t>(address), value);
        return;
    }
    if ((address & 0xFFF0) == 0xFE60 || (address & 0xFFF0) == 0xFE70) {
        userVIA_.write(static_cast<std::uint8_t>(address), value);
        return;
    }
    if ((address & 0xFFE0) == 0xFE80) fdc_.write(static_cast<std::uint8_t>(address), value);
}

void BBCMicro::tick(std::uint32_t cpuCycles) {
    // C0-DOC-RATIONALE: docs/code/timing-model.md owns the CPU/VIA/CRTC rate
    // conversion and remainder invariants represented by this aggregation.
    const auto viaTotal = viaRemainder_ + cpuCycles;
    systemVIA_.tick(viaTotal / 2);
    userVIA_.tick(viaTotal / 2);
    viaRemainder_ = viaTotal % 2;

    const unsigned divider = videoULA_.crtcTwoMHz() ? 1u : 2u;
    const auto crtcTotal = crtcRemainder_ + cpuCycles;
    crtc_.tick(crtcTotal / divider);
    crtcRemainder_ = crtcTotal % divider;
    fdc_.tick(cpuCycles);

    if (crtc_.frameReady()) {
        renderFrame();
        systemVIA_.setCA1(false);
        systemVIA_.setCA1(true);
        crtc_.consumeFrame();
    }
    cpu_.setIRQ(systemVIA_.irq() || userVIA_.irq());
}

std::uint64_t BBCMicro::runFor(std::uint64_t cpuCycles) {
    const auto start = cpu_.state().cycles;
    while (cpu_.state().cycles - start < cpuCycles)
        cpu_.step();
    return cpu_.state().cycles - start;
}

bool BBCMicro::runUntilFrame(std::uint64_t maximumCycles) {
    const auto oldFrame = frame_.number;
    const auto start = cpu_.state().cycles;
    while (cpu_.state().cycles - start < maximumCycles) {
        cpu_.step();
        if (frame_.number != oldFrame) return true;
    }
    return false;
}

std::array<std::uint8_t, 4> BBCMicro::rgbaForColour(std::uint8_t colour) {
    // BBC physical colour bits are BGR in significance: 1=red, 2=green, 4=blue.
    return {static_cast<std::uint8_t>((colour & 1) ? 255 : 0),
            static_cast<std::uint8_t>((colour & 2) ? 255 : 0),
            static_cast<std::uint8_t>((colour & 4) ? 255 : 0), 255};
}

void BBCMicro::renderFrame() {
    // C0-DOC-RATIONALE: docs/code/evidence-and-testing.md explains why the
    // complete RGBA buffer is the exact frame-evidence boundary.
    // The bitmap path follows BBC MA addressing: CRTC display start selects the base,
    // raster/bitplane packing follows ULA mode, and screen addresses wrap in 0x8000 RAM.
    // Frame evidence compares the resulting RGBA buffer; see docs/code/evidence-and-testing.md.
    if (videoULA_.teletext()) {
        auto teletext = teletextRenderer_.render(ram_, crtc_, crtc_.frameNumber());
        frame_.width = teletext.width;
        frame_.height = teletext.height;
        frame_.number = crtc_.frameNumber();
        frame_.rgba = std::move(teletext.rgba);
        return;
    }
    const unsigned columns = crtc_.reg(1);
    const unsigned rows = crtc_.reg(6);
    const unsigned scanlines = static_cast<unsigned>(crtc_.reg(9)) + 1;
    const unsigned bitsPerPixel = videoULA_.bitsPerPixel();
    const unsigned byteColumns = columns * bitsPerPixel;
    if (columns == 0 || rows == 0 || rows > 128 || scanlines > 32 || byteColumns > 1024) return;

    frame_.width = columns * 8;
    frame_.height = rows * scanlines;
    frame_.number = crtc_.frameNumber();
    frame_.rgba.assign(static_cast<std::size_t>(frame_.width) * frame_.height * 4, 0);

    const auto screenBytes = byteColumns * rows * scanlines;
    const auto wrapBase = screenBytes < 0x8000 ? 0x8000u - screenBytes : 0u;
    const auto initialMA = static_cast<unsigned>(crtc_.displayStart() & 0x0FFF);
    const bool flashPhase = ((frame_.number / 25) & 1) != 0;

    for (unsigned characterRow = 0; characterRow < rows; ++characterRow) {
        for (unsigned raster = 0; raster < scanlines; ++raster) {
            const auto y = characterRow * scanlines + raster;
            for (unsigned byteColumn = 0; byteColumn < byteColumns; ++byteColumn) {
                const auto ma = initialMA + characterRow * byteColumns + byteColumn;
                auto address = (ma << 3) + raster;
                if (address >= 0x8000)
                    address = wrapBase + ((address - 0x8000) % std::max(1u, screenBytes));
                const auto packed = ram_[address & 0x7FFF];
                const auto logicalX = byteColumn * (8 / bitsPerPixel);

                if (bitsPerPixel == 1) {
                    for (unsigned pixel = 0; pixel < 8; ++pixel) {
                        const auto logical = static_cast<std::uint8_t>((packed >> (7 - pixel)) & 1);
                        const auto rgba =
                            rgbaForColour(videoULA_.physicalColour(logical, flashPhase));
                        const auto out =
                            (static_cast<std::size_t>(y) * frame_.width + logicalX + pixel) * 4;
                        std::copy(rgba.begin(), rgba.end(),
                                  frame_.rgba.begin() + static_cast<std::ptrdiff_t>(out));
                    }
                } else if (bitsPerPixel == 2) {
                    for (unsigned pixel = 0; pixel < 4; ++pixel) {
                        const auto logical = static_cast<std::uint8_t>(
                            ((packed >> (7 - pixel)) & 1) << 1 | ((packed >> (3 - pixel)) & 1));
                        const auto rgba =
                            rgbaForColour(videoULA_.physicalColour(logical, flashPhase));
                        const auto out =
                            (static_cast<std::size_t>(y) * frame_.width + logicalX + pixel) * 4;
                        std::copy(rgba.begin(), rgba.end(),
                                  frame_.rgba.begin() + static_cast<std::ptrdiff_t>(out));
                    }
                } else {
                    for (unsigned pixel = 0; pixel < 2; ++pixel) {
                        std::uint8_t logical = 0;
                        for (unsigned plane = 0; plane < 4; ++plane) {
                            logical |= static_cast<std::uint8_t>(
                                ((packed >> (7 - pixel - plane * 2)) & 1) << (3 - plane));
                        }
                        const auto rgba =
                            rgbaForColour(videoULA_.physicalColour(logical, flashPhase));
                        const auto out =
                            (static_cast<std::size_t>(y) * frame_.width + logicalX + pixel) * 4;
                        std::copy(rgba.begin(), rgba.end(),
                                  frame_.rgba.begin() + static_cast<std::ptrdiff_t>(out));
                    }
                }
            }
        }
    }
}

void BBCMicro::updateIC32(std::uint8_t value, std::uint8_t ddr) {
    if ((ddr & 0x0F) != 0x0F) return;
    const auto address = static_cast<std::uint8_t>(value & 0x07);
    ic32_[address] = (value & 0x08) != 0;
}

std::uint8_t BBCMicro::keyboardPortA() const {
    const auto output = systemVIA_.outputA();
    const auto ddr = systemVIA_.ddra();
    const auto column = static_cast<std::uint8_t>(output & 0x0F);
    const auto row = static_cast<std::uint8_t>((output >> 4) & 0x07);
    auto input = static_cast<std::uint8_t>(0x7F | (output & ddr));
    if (column < keyboard_.size() && (keyboard_[column] & (1u << row)) != 0)
        input |= 0x80;
    else
        input &= 0x7F;
    return input;
}

void BBCMicro::setKey(std::uint8_t column, std::uint8_t row, bool pressed) {
    if (column >= keyboard_.size() || row >= 16) return;
    if (pressed)
        keyboard_[column] |= static_cast<std::uint16_t>(1u << row);
    else
        keyboard_[column] &= static_cast<std::uint16_t>(~(1u << row));
}

void BBCMicro::setBreak(bool pressed) {
    if (pressed && !breakPressed_) reset();
    breakPressed_ = pressed;
}

} // namespace beeb
