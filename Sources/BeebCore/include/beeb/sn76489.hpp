#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace beeb {

class SN76489 {
public:
    void reset();
    void write(std::uint8_t value);
    void render(float* mono, std::size_t frames, double sampleRate);

    [[nodiscard]] std::uint16_t tonePeriod(unsigned channel) const noexcept { return tone_[channel % 3]; }
    [[nodiscard]] std::uint8_t volume(unsigned channel) const noexcept { return volume_[channel % 4]; }

private:
    std::array<std::uint16_t, 3> tone_{1,1,1};
    std::array<std::uint8_t, 4> volume_{15,15,15,15};
    std::array<double, 4> phase_{};
    std::uint8_t noise_ = 0;
    std::uint8_t latchedChannel_ = 0;
    bool latchedVolume_ = false;
    std::uint16_t lfsr_ = 0x4000;
    bool noiseLevel_ = false;
};

} // namespace beeb
