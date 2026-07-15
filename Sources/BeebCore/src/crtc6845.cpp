#include "beeb/crtc6845.hpp"

namespace beeb {

void CRTC6845::reset() {
    registers_.fill(0);
    selected_ = 0;
    hCharacter_ = characterRow_ = 0;
    rasterRow_ = verticalAdjust_ = 0;
    frameNumber_ = 0;
    inVerticalAdjust_ = frameReady_ = false;
}

void CRTC6845::write(std::uint8_t value) {
    if (selected_ > 17) return;
    static constexpr std::array<std::uint8_t, 18> masks{
        0xFF,0xFF,0xFF,0xFF,0x7F,0x1F,0x7F,0x7F,0xF3,0x1F,0x7F,0x1F,0x3F,0xFF,0x3F,0xFF,0x3F,0xFF
    };
    registers_[selected_] = static_cast<std::uint8_t>(value & masks[selected_]);
}

std::uint8_t CRTC6845::read() const {
    // On the original MC6845 only cursor/light-pen registers are readable.
    if (selected_ >= 14 && selected_ <= 17) return registers_[selected_];
    return 0;
}

std::uint16_t CRTC6845::displayStart() const noexcept {
    return static_cast<std::uint16_t>(((registers_[12] & 0x3F) << 8) | registers_[13]);
}

std::uint16_t CRTC6845::cursorAddress() const noexcept {
    return static_cast<std::uint16_t>(((registers_[14] & 0x3F) << 8) | registers_[15]);
}

bool CRTC6845::displayEnabled() const noexcept {
    return hCharacter_ < registers_[1] && characterRow_ < registers_[6] && !inVerticalAdjust_;
}

void CRTC6845::endScanline() {
    if (inVerticalAdjust_) {
        if (++verticalAdjust_ > registers_[5]) {
            inVerticalAdjust_ = false;
            verticalAdjust_ = 0;
            characterRow_ = 0;
            rasterRow_ = 0;
            ++frameNumber_;
            frameReady_ = true;
        }
        return;
    }

    if (rasterRow_ < registers_[9]) {
        ++rasterRow_;
        return;
    }
    rasterRow_ = 0;
    if (characterRow_ < registers_[4]) {
        ++characterRow_;
        return;
    }

    if (registers_[5] != 0) {
        inVerticalAdjust_ = true;
        verticalAdjust_ = 0;
    } else {
        characterRow_ = 0;
        ++frameNumber_;
        frameReady_ = true;
    }
}

void CRTC6845::tick(std::uint32_t characterClocks) {
    while (characterClocks-- != 0) {
        if (hCharacter_ >= registers_[0]) {
            hCharacter_ = 0;
            endScanline();
        } else {
            ++hCharacter_;
        }
    }
}

} // namespace beeb
