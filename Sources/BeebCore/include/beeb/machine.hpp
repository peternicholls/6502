#pragma once

#include "beeb/bus.hpp"
#include "beeb/cpu6502.hpp"
#include "beeb/crtc6845.hpp"
#include "beeb/intel8271.hpp"
#include "beeb/sn76489.hpp"
#include "beeb/teletext_renderer.hpp"
#include "beeb/via6522.hpp"
#include "beeb/video_ula.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace beeb {

struct VideoFrame {
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    std::uint64_t number = 0;
    std::vector<std::uint8_t> rgba;
};

class BBCMicro final : public Bus {
public:
    BBCMicro();

    std::uint8_t read(std::uint16_t address) override;
    void write(std::uint16_t address, std::uint8_t value) override;
    void tick(std::uint32_t cpuCycles) override;

    bool loadOSROM(std::span<const std::uint8_t> rom);
    bool loadSidewaysROM(std::uint8_t bank, std::span<const std::uint8_t> rom);
    bool loadRAM(std::uint16_t address, std::span<const std::uint8_t> bytes);
    bool mountDisc(unsigned drive, std::span<const std::uint8_t> bytes,
                   DiscImage::Layout layout, bool writable = false);
    void reset();
    std::uint64_t runFor(std::uint64_t cpuCycles);
    bool runUntilFrame(std::uint64_t maximumCycles = 100'000);
    void renderFrame();

    [[nodiscard]] CPU6502& cpu() noexcept { return cpu_; }
    [[nodiscard]] const CPU6502& cpu() const noexcept { return cpu_; }
    [[nodiscard]] VIA6522& systemVIA() noexcept { return systemVIA_; }
    [[nodiscard]] VIA6522& userVIA() noexcept { return userVIA_; }
    [[nodiscard]] CRTC6845& crtc() noexcept { return crtc_; }
    [[nodiscard]] VideoULA& videoULA() noexcept { return videoULA_; }
    [[nodiscard]] SN76489& sound() noexcept { return sound_; }
    [[nodiscard]] Intel8271& discController() noexcept { return fdc_; }
    [[nodiscard]] const VideoFrame& frame() const noexcept { return frame_; }
    [[nodiscard]] std::span<const std::uint8_t> ram() const noexcept { return ram_; }
    [[nodiscard]] std::uint8_t selectedROM() const noexcept { return selectedROM_; }
    [[nodiscard]] bool hasOSROM() const noexcept { return osROMLoaded_; }

    void setKey(std::uint8_t column, std::uint8_t row, bool pressed);
    void setBreak(bool pressed);

private:
    std::array<std::uint8_t, 0x8000> ram_{};
    std::array<std::uint8_t, 0x4000> osROM_{};
    std::array<std::array<std::uint8_t, 0x4000>, 16> sidewaysROM_{};
    std::array<bool, 16> sidewaysPresent_{};
    std::array<std::uint16_t, 16> keyboard_{};
    std::array<bool, 8> ic32_{};
    CPU6502 cpu_;
    VIA6522 systemVIA_;
    VIA6522 userVIA_;
    CRTC6845 crtc_;
    VideoULA videoULA_;
    SN76489 sound_;
    TeletextRenderer teletextRenderer_;
    Intel8271 fdc_;
    VideoFrame frame_;
    std::uint8_t selectedROM_ = 0;
    std::uint32_t viaRemainder_ = 0;
    std::uint32_t crtcRemainder_ = 0;
    bool osROMLoaded_ = false;
    bool breakPressed_ = false;

    std::uint8_t readIO(std::uint16_t address);
    void writeIO(std::uint16_t address, std::uint8_t value);
    void configureSystemVIA();
    std::uint8_t keyboardPortA() const;
    void updateIC32(std::uint8_t value, std::uint8_t ddr);
    static std::array<std::uint8_t, 4> rgbaForColour(std::uint8_t colour);
};

} // namespace beeb
