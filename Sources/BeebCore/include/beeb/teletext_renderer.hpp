#pragma once

#include "beeb/crtc6845.hpp"

#include <array>
#include <cstdint>
#include <span>
#include <vector>

namespace beeb {

struct TeletextBitmap {
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    std::vector<std::uint8_t> rgba;
};

// A clean-room, readable Mode 7 renderer. It models the SAA5050 control-code
// state and mosaics, but deliberately uses an original 5x7 host font rather
// than embedding Acorn/Mullard character-ROM data.
class TeletextRenderer {
public:
    TeletextBitmap render(std::span<const std::uint8_t> ram, const CRTC6845& crtc,
                          std::uint64_t frameNumber) const;

private:
    static std::array<std::uint8_t, 7> glyph(char character);
    static std::array<std::uint8_t, 4> colour(std::uint8_t physical);
    static void pixel(TeletextBitmap& bitmap, unsigned x, unsigned y,
                      const std::array<std::uint8_t, 4>& rgba);
};

} // namespace beeb
