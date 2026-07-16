#pragma once

#include "beeb/crtc6845.hpp"

#include <array>
#include <cstdint>
#include <span>
#include <vector>

namespace beeb {

/// Owned result of rendering one Mode 7 frame.
struct TeletextBitmap {
    std::uint32_t width = 0;        ///< Pixel width.
    std::uint32_t height = 0;       ///< Pixel height.
    std::vector<std::uint8_t> rgba; ///< Packed 8-bit RGBA pixels.
};

/// Clean-room Mode 7 renderer with no proprietary character-ROM bytes.
///
/// It models SAA5050-style control state and mosaics while using an original
/// 5x7 host font. Instances hold no mutable frame state and can be reused.
/// Rendering is synchronous and safe for concurrent calls because output state
/// is returned by value.
class TeletextRenderer {
  public:
    /// Renders the CRTC-selected display from a RAM snapshot.
    /// @param ram Machine RAM containing Mode 7 character cells.
    /// @param crtc Current display geometry and start address.
    /// @param frameNumber Frame identity used for flash phase.
    /// @return An independently owned RGBA bitmap.
    TeletextBitmap render(std::span<const std::uint8_t> ram, const CRTC6845& crtc,
                          std::uint64_t frameNumber) const;

  private:
    static std::array<std::uint8_t, 7> glyph(char character);
    static std::array<std::uint8_t, 4> colour(std::uint8_t physical);
    static void pixel(TeletextBitmap& bitmap, unsigned x, unsigned y,
                      const std::array<std::uint8_t, 4>& rgba);
};

} // namespace beeb
