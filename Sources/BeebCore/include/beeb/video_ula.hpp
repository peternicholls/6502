#pragma once

#include <array>
#include <cstdint>

namespace beeb {

/// BBC Micro Video ULA control and logical-to-physical palette model.
class VideoULA {
public:
    /// Restores the control register and default logical palette.
    void reset();
    /// Replaces the host-visible control register.
    /// @param value New control byte.
    void writeControl(std::uint8_t value) noexcept { control_ = value; }
    /// Applies one packed logical-colour palette write.
    /// @param value High nibble selects logical colour; low nibble is physical.
    void writePalette(std::uint8_t value) noexcept;

    /// Returns the current control register.
    /// @return Control byte.
    [[nodiscard]] std::uint8_t control() const noexcept { return control_; }
    /// Maps a logical colour through the palette and optional flash inversion.
    /// @param logical Logical colour selector; only the low four bits are used.
    /// @param flashPhase Whether flashing colours are currently inverted.
    /// @return Three-bit physical colour.
    [[nodiscard]] std::uint8_t physicalColour(std::uint8_t logical, bool flashPhase = false) const noexcept;
    /// Returns the bitmap colour depth selected by the control register.
    /// @return One, two, or four bits per logical pixel.
    [[nodiscard]] unsigned bitsPerPixel() const noexcept;
    /// Returns the number of logical pixels encoded in one RAM byte.
    /// @return Eight divided by bitsPerPixel().
    [[nodiscard]] unsigned pixelsPerByte() const noexcept { return 8u / bitsPerPixel(); }
    /// Returns the horizontal host-pixel expansion factor.
    /// @return Current bits-per-pixel value.
    [[nodiscard]] unsigned horizontalScale() const noexcept { return bitsPerPixel(); }
    /// Reports whether Mode 7/teletext rendering is selected.
    /// @return Teletext-selection state.
    [[nodiscard]] bool teletext() const noexcept { return (control_ & 0x02) != 0; }
    /// Reports whether the CRTC receives the 2 MHz rather than 1 MHz clock.
    /// @return `true` for the 2 MHz CRTC clock.
    [[nodiscard]] bool crtcTwoMHz() const noexcept { return (control_ & 0x10) != 0; }

private:
    std::uint8_t control_ = 0;
    std::array<std::uint8_t, 16> palette_{};
};

} // namespace beeb
