#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace beeb {

/// Deterministic SN76489 register and mono waveform model.
/// The owning machine serializes writes, reset, and rendering; no background
/// callbacks occur, so rendering is synchronous and reusable.
class SN76489 {
  public:
    /// Restores register, phase, and noise-generator defaults.
    void reset();
    /// Applies one SN76489 latch/data protocol byte.
    /// @param value Byte written to the sound chip.
    void write(std::uint8_t value);
    /// Fills a caller-owned mono buffer and advances sampling phase.
    /// @param mono Writable storage for `frames` samples; must be non-null.
    /// @param frames Number of samples to generate.
    /// @param sampleRate Finite, positive sample rate in hertz.
    void render(float* mono, std::size_t frames, double sampleRate);

    /// Returns a tone channel's programmed period.
    /// @param channel Channel selector reduced modulo three.
    /// @return Programmed ten-bit period.
    [[nodiscard]] std::uint16_t tonePeriod(unsigned channel) const noexcept {
        return tone_[channel % 3];
    }
    /// Returns a channel's attenuation nibble, where 15 is silent.
    /// @param channel Channel selector reduced modulo four.
    /// @return Attenuation in the range 0...15.
    [[nodiscard]] std::uint8_t volume(unsigned channel) const noexcept {
        return volume_[channel % 4];
    }

  private:
    std::array<std::uint16_t, 3> tone_{1, 1, 1};
    std::array<std::uint8_t, 4> volume_{15, 15, 15, 15};
    std::array<double, 4> phase_{};
    std::uint8_t noise_ = 0;
    std::uint8_t latchedChannel_ = 0;
    bool latchedVolume_ = false;
    std::uint16_t lfsr_ = 0x4000;
    bool noiseLevel_ = false;
};

} // namespace beeb
