#include "beeb/via6522.hpp"

#include <utility>

namespace beeb {

void VIA6522::reset() {
    orb_ = ora_ = ddrb_ = ddra_ = 0;
    t1Counter_ = t1Latch_ = t2Counter_ = 0xFFFF;
    t2LatchLow_ = 0xFF;
    shift_ = acr_ = pcr_ = ifr_ = ier_ = 0;
    t1Running_ = t2Running_ = false;
    ca1_ = cb1_ = true;
    notifyA();
    notifyB();
}

bool VIA6522::irq() const noexcept {
    return (ifr_ & ier_ & 0x7F) != 0;
}

std::uint8_t VIA6522::portAValue() const {
    const auto input = inputA_ ? inputA_() : 0xFF;
    return static_cast<std::uint8_t>((ora_ & ddra_) | (input & ~ddra_));
}

std::uint8_t VIA6522::portBValue() const {
    const auto input = inputB_ ? inputB_() : 0xFF;
    auto value = static_cast<std::uint8_t>((orb_ & ddrb_) | (input & ~ddrb_));
    if ((acr_ & 0x80) != 0) {
        value = static_cast<std::uint8_t>((value & 0x7F) | (orb_ & 0x80));
    }
    return value;
}

void VIA6522::notifyA() {
    if (outputA_) outputA_(ora_, ddra_);
}

void VIA6522::notifyB() {
    if (outputB_) outputB_(orb_, ddrb_);
}

std::uint8_t VIA6522::read(std::uint8_t reg) {
    switch (reg & 0x0F) {
    case 0x0:
        clearIFR(0x18);
        return portBValue();
    case 0x1:
        clearIFR(0x03);
        return portAValue();
    case 0x2:
        return ddrb_;
    case 0x3:
        return ddra_;
    case 0x4:
        clearIFR(0x40);
        return static_cast<std::uint8_t>(t1Counter_);
    case 0x5:
        return static_cast<std::uint8_t>(t1Counter_ >> 8);
    case 0x6:
        return static_cast<std::uint8_t>(t1Latch_);
    case 0x7:
        return static_cast<std::uint8_t>(t1Latch_ >> 8);
    case 0x8:
        clearIFR(0x20);
        return static_cast<std::uint8_t>(t2Counter_);
    case 0x9:
        return static_cast<std::uint8_t>(t2Counter_ >> 8);
    case 0xA:
        clearIFR(0x04);
        return shift_;
    case 0xB:
        return acr_;
    case 0xC:
        return pcr_;
    case 0xD:
        return static_cast<std::uint8_t>(ifr_ | (irq() ? 0x80 : 0));
    case 0xE:
        return static_cast<std::uint8_t>(ier_ | 0x80);
    case 0xF:
        return portAValue();
    default:
        return 0xFF;
    }
}

void VIA6522::write(std::uint8_t reg, std::uint8_t value) {
    switch (reg & 0x0F) {
    case 0x0:
        orb_ = value;
        clearIFR(0x18);
        notifyB();
        break;
    case 0x1:
        ora_ = value;
        clearIFR(0x03);
        notifyA();
        break;
    case 0x2:
        ddrb_ = value;
        notifyB();
        break;
    case 0x3:
        ddra_ = value;
        notifyA();
        break;
    case 0x4:
        t1Latch_ = static_cast<std::uint16_t>((t1Latch_ & 0xFF00) | value);
        break;
    case 0x5:
        t1Latch_ = static_cast<std::uint16_t>((static_cast<std::uint16_t>(value) << 8) |
                                              (t1Latch_ & 0x00FF));
        t1Counter_ = t1Latch_;
        t1Running_ = true;
        clearIFR(0x40);
        if ((acr_ & 0x80) != 0) orb_ &= 0x7F;
        notifyB();
        break;
    case 0x6:
        t1Latch_ = static_cast<std::uint16_t>((t1Latch_ & 0xFF00) | value);
        break;
    case 0x7:
        t1Latch_ = static_cast<std::uint16_t>((static_cast<std::uint16_t>(value) << 8) |
                                              (t1Latch_ & 0x00FF));
        clearIFR(0x40);
        break;
    case 0x8:
        t2LatchLow_ = value;
        break;
    case 0x9:
        t2Counter_ =
            static_cast<std::uint16_t>((static_cast<std::uint16_t>(value) << 8) | t2LatchLow_);
        t2Running_ = true;
        clearIFR(0x20);
        break;
    case 0xA:
        shift_ = value;
        clearIFR(0x04);
        break;
    case 0xB:
        acr_ = value;
        break;
    case 0xC:
        pcr_ = value;
        break;
    case 0xD:
        clearIFR(static_cast<std::uint8_t>(value & 0x7F));
        break;
    case 0xE:
        if ((value & 0x80) != 0)
            ier_ |= static_cast<std::uint8_t>(value & 0x7F);
        else
            ier_ &= static_cast<std::uint8_t>(~value & 0x7F);
        break;
    case 0xF:
        ora_ = value;
        notifyA();
        break;
    default:
        break;
    }
}

void VIA6522::tick(std::uint32_t cycles) {
    while (cycles-- != 0) {
        if (t1Running_) {
            if (t1Counter_ == 0) {
                ifr_ |= 0x40;
                if ((acr_ & 0x80) != 0) {
                    orb_ ^= 0x80;
                    notifyB();
                }
                if ((acr_ & 0x40) != 0)
                    t1Counter_ = t1Latch_;
                else {
                    t1Counter_ = 0xFFFF;
                    t1Running_ = false;
                }
            } else {
                --t1Counter_;
            }
        }
        if (t2Running_ && (acr_ & 0x20) == 0) {
            if (t2Counter_ == 0) {
                ifr_ |= 0x20;
                t2Counter_ = 0xFFFF;
                t2Running_ = false;
            } else {
                --t2Counter_;
            }
        }
    }
}

void VIA6522::setCA1(bool level) {
    const bool rising = (pcr_ & 0x01) != 0;
    if (level != ca1_ && ((level && rising) || (!level && !rising))) ifr_ |= 0x02;
    ca1_ = level;
}

void VIA6522::setCB1(bool level) {
    const bool rising = (pcr_ & 0x10) != 0;
    if (level != cb1_ && ((level && rising) || (!level && !rising))) ifr_ |= 0x10;
    cb1_ = level;
}

} // namespace beeb
