#include "beeb/video_ula.hpp"

namespace beeb {

void VideoULA::reset() {
    control_ = 0;
    for (std::uint8_t logical = 0; logical < palette_.size(); ++logical) {
        palette_[logical] = static_cast<std::uint8_t>(logical & 0x07);
    }
}

void VideoULA::writePalette(std::uint8_t value) noexcept {
    const auto logical = static_cast<std::uint8_t>(value >> 4);
    // The BBC's colour outputs are active-low; software writes physical XOR 7.
    palette_[logical] = static_cast<std::uint8_t>((value & 0x0F) ^ 0x07);
}

std::uint8_t VideoULA::physicalColour(std::uint8_t logical, bool flashPhase) const noexcept {
    auto colour = palette_[logical & 0x0F];
    if ((colour & 0x08) != 0 && flashPhase) colour ^= 0x07;
    return static_cast<std::uint8_t>(colour & 0x07);
}

unsigned VideoULA::bitsPerPixel() const noexcept {
    // ULA bits 2-3 select 10/20/40/80-character serializer rates. The normal
    // bitmap modes use the latter three as 4/2/1 bits per logical pixel.
    switch ((control_ >> 2) & 0x03) {
    case 1:
        return 4;
    case 2:
        return 2;
    case 3:
        return 1;
    default:
        return 1;
    }
}

} // namespace beeb
