#include "beeb/sn76489.hpp"

#include <algorithm>
#include <array>
#include <cmath>

namespace beeb {

// The write path implements the SN76489 latch/data protocol. Tone divisors and noise
// rates are in chip-clock units; zero divisors are coerced to one, latch writes reset
// the 15-bit LFSR, and the fixed amplitude table/mix scale provide deterministic audible
// output while remaining an explicitly approximate model (see docs/REFERENCES.md).

void SN76489::reset() {
    tone_ = {1, 1, 1};
    volume_ = {15, 15, 15, 15};
    phase_.fill(0.0);
    noise_ = latchedChannel_ = 0;
    latchedVolume_ = false;
    lfsr_ = 0x4000;
    noiseLevel_ = false;
}

void SN76489::write(std::uint8_t value) {
    if ((value & 0x80) != 0) {
        latchedChannel_ = static_cast<std::uint8_t>((value >> 5) & 0x03);
        latchedVolume_ = (value & 0x10) != 0;
        if (latchedVolume_) {
            volume_[latchedChannel_] = static_cast<std::uint8_t>(value & 0x0F);
        } else if (latchedChannel_ == 3) {
            noise_ = static_cast<std::uint8_t>(value & 0x07);
            lfsr_ = 0x4000;
        } else {
            tone_[latchedChannel_] =
                static_cast<std::uint16_t>((tone_[latchedChannel_] & 0x3F0) | (value & 0x0F));
            if (tone_[latchedChannel_] == 0) tone_[latchedChannel_] = 1;
        }
        return;
    }

    if (latchedVolume_) {
        volume_[latchedChannel_] = static_cast<std::uint8_t>(value & 0x0F);
    } else if (latchedChannel_ == 3) {
        noise_ = static_cast<std::uint8_t>(value & 0x07);
        lfsr_ = 0x4000;
    } else {
        tone_[latchedChannel_] =
            static_cast<std::uint16_t>(((value & 0x3F) << 4) | (tone_[latchedChannel_] & 0x0F));
        if (tone_[latchedChannel_] == 0) tone_[latchedChannel_] = 1;
    }
}

void SN76489::render(float* mono, std::size_t frames, double sampleRate) {
    static constexpr std::array<float, 16> amplitude{
        1.0000f, 0.7943f, 0.6310f, 0.5012f, 0.3981f, 0.3162f, 0.2512f, 0.1995f,
        0.1585f, 0.1259f, 0.1000f, 0.0794f, 0.0631f, 0.0501f, 0.0398f, 0.0f};
    constexpr double chipClock = 4'000'000.0;

    for (std::size_t frame = 0; frame < frames; ++frame) {
        float sample = 0.0f;
        for (unsigned channel = 0; channel < 3; ++channel) {
            const auto frequency = chipClock / (32.0 * std::max<std::uint16_t>(1, tone_[channel]));
            phase_[channel] += frequency / sampleRate;
            phase_[channel] -= std::floor(phase_[channel]);
            sample += (phase_[channel] < 0.5 ? 1.0f : -1.0f) * amplitude[volume_[channel]];
        }

        const unsigned rate = noise_ & 0x03;
        const double noiseFrequency =
            rate == 3 ? chipClock / (32.0 * std::max<std::uint16_t>(1, tone_[2]))
                      : chipClock / (32.0 * (16u << rate));
        phase_[3] += noiseFrequency / sampleRate;
        if (phase_[3] >= 1.0) {
            phase_[3] -= std::floor(phase_[3]);
            const auto feedback = (noise_ & 0x04) != 0
                                      ? static_cast<std::uint16_t>((lfsr_ ^ (lfsr_ >> 1)) & 1)
                                      : static_cast<std::uint16_t>(lfsr_ & 1);
            lfsr_ = static_cast<std::uint16_t>((lfsr_ >> 1) | (feedback << 14));
            noiseLevel_ = (lfsr_ & 1) != 0;
        }
        sample += (noiseLevel_ ? 1.0f : -1.0f) * amplitude[volume_[3]];
        mono[frame] = std::clamp(sample * 0.20f, -1.0f, 1.0f);
    }
}

} // namespace beeb
