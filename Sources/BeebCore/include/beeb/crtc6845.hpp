#pragma once

#include <array>
#include <cstdint>

namespace beeb {

/// Character-clock timing and register model of the MC6845-compatible CRTC.
class CRTC6845 {
  public:
    /// Restores registers, counters, and frame notification state.
    void reset();

    /// Selects one of the 32 register addresses.
    /// @param reg Register number; only the low five bits are used.
    void select(std::uint8_t reg) noexcept { selected_ = static_cast<std::uint8_t>(reg & 0x1F); }

    /// Writes the currently selected register.
    /// @param value Register value, masked where the hardware width is narrower.
    void write(std::uint8_t value);

    /// Reads the currently selected readable register.
    /// @return Register value, or zero for a write-only register.
    [[nodiscard]] std::uint8_t read() const;

    /// Returns the selected register number.
    /// @return Register index in the range 0...31.
    [[nodiscard]] std::uint8_t selected() const noexcept { return selected_; }

    /// Reads a register without selected-register side effects.
    /// @param index Register number; only the low five bits are used.
    /// @return Stored register value.
    [[nodiscard]] std::uint8_t reg(std::uint8_t index) const noexcept {
        return registers_[index & 0x1F];
    }

    /// Advances horizontal, raster, row, and frame counters.
    /// @param characterClocks Character clocks elapsed at the current CRTC rate.
    void tick(std::uint32_t characterClocks);

    /// Returns the 14-bit display-start address from registers 12 and 13.
    /// @return Display-start character address.
    [[nodiscard]] std::uint16_t displayStart() const noexcept;
    /// Returns the 14-bit cursor address from registers 14 and 15.
    /// @return Cursor character address.
    [[nodiscard]] std::uint16_t cursorAddress() const noexcept;
    /// Reports whether at least one frame completed since consumeFrame().
    /// @return Pending frame-completion state.
    [[nodiscard]] bool frameReady() const noexcept { return frameReady_; }
    /// Acknowledges all completed-frame notification currently represented.
    void consumeFrame() noexcept { frameReady_ = false; }
    /// Returns the count of completed frames since reset.
    /// @return Monotonic completed-frame count.
    [[nodiscard]] std::uint64_t frameNumber() const noexcept { return frameNumber_; }
    /// Returns the current horizontal character-clock position.
    /// @return Zero-based horizontal character counter.
    [[nodiscard]] std::uint16_t horizontalCharacter() const noexcept { return hCharacter_; }
    /// Returns the current character row within the frame.
    /// @return Zero-based character row.
    [[nodiscard]] std::uint16_t characterRow() const noexcept { return characterRow_; }
    /// Returns the current raster row within a character.
    /// @return Zero-based raster row.
    [[nodiscard]] std::uint8_t rasterRow() const noexcept { return rasterRow_; }
    /// Reports whether the current counters lie in the programmed display area.
    /// @return `true` while display output is enabled.
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
