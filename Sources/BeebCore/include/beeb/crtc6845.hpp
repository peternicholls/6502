#pragma once

#include <array>
#include <cstdint>

namespace beeb {

class CRTC6845 {
public:
    void reset();
    void select(std::uint8_t reg) noexcept { selected_ = static_cast<std::uint8_t>(reg & 0x1F); }
    void write(std::uint8_t value);
    [[nodiscard]] std::uint8_t read() const;
    [[nodiscard]] std::uint8_t selected() const noexcept { return selected_; }
    [[nodiscard]] std::uint8_t reg(std::uint8_t index) const noexcept { return registers_[index & 0x1F]; }
    void tick(std::uint32_t characterClocks);

    [[nodiscard]] std::uint16_t displayStart() const noexcept;
    [[nodiscard]] std::uint16_t cursorAddress() const noexcept;
    [[nodiscard]] bool frameReady() const noexcept { return frameReady_; }
    void consumeFrame() noexcept { frameReady_ = false; }
    [[nodiscard]] std::uint64_t frameNumber() const noexcept { return frameNumber_; }
    [[nodiscard]] std::uint16_t horizontalCharacter() const noexcept { return hCharacter_; }
    [[nodiscard]] std::uint16_t characterRow() const noexcept { return characterRow_; }
    [[nodiscard]] std::uint8_t rasterRow() const noexcept { return rasterRow_; }
    [[nodiscard]] bool displayEnabled() const noexcept;

private:
    std::array<std::uint8_t, 32> registers_{};
    std::uint8_t selected_ = 0;
    std::uint16_t hCharacter_ = 0;
    std::uint16_t characterRow_ = 0;
    std::uint8_t rasterRow_ = 0;
    std::uint8_t verticalAdjust_ = 0;
    std::uint64_t frameNumber_ = 0;
    bool inVerticalAdjust_ = false;
    bool frameReady_ = false;

    void endScanline();
};

} // namespace beeb
