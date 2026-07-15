#pragma once

#include <array>
#include <cstdint>

namespace beeb {

class VideoULA {
public:
    void reset();
    void writeControl(std::uint8_t value) noexcept { control_ = value; }
    void writePalette(std::uint8_t value) noexcept;

    [[nodiscard]] std::uint8_t control() const noexcept { return control_; }
    [[nodiscard]] std::uint8_t physicalColour(std::uint8_t logical, bool flashPhase = false) const noexcept;
    [[nodiscard]] unsigned bitsPerPixel() const noexcept;
    [[nodiscard]] unsigned pixelsPerByte() const noexcept { return 8u / bitsPerPixel(); }
    [[nodiscard]] unsigned horizontalScale() const noexcept { return bitsPerPixel(); }
    [[nodiscard]] bool teletext() const noexcept { return (control_ & 0x02) != 0; }
    [[nodiscard]] bool crtcTwoMHz() const noexcept { return (control_ & 0x10) != 0; }

private:
    std::uint8_t control_ = 0;
    std::array<std::uint8_t, 16> palette_{};
};

} // namespace beeb
