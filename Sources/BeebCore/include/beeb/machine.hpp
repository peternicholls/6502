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

/// Owned rendering result for the most recently completed video frame.
struct VideoFrame {
    std::uint32_t width = 0;  ///< Pixel width.
    std::uint32_t height = 0; ///< Pixel height.
    std::uint64_t number = 0; ///< CRTC frame sequence number.
    std::vector<std::uint8_t> rgba; ///< Packed 8-bit RGBA pixels.
};

/// Aggregate BBC Model B core and the owner of all emulated devices and media.
///
/// The machine is deterministic for a given initial state and input sequence.
/// It has no internal synchronization. Supported hosts use MachineRuntime;
/// direct construction is reserved for low-level single-threaded core tests.
class BBCMicro final : public Bus {
public:
    /// Constructs a machine, connects device callbacks, and resets all state.
    BBCMicro();

    /// Reads the CPU memory map, including device side effects.
    /// @param address 16-bit CPU address.
    /// @return Byte supplied by RAM, ROM, or the addressed device.
    std::uint8_t read(std::uint16_t address) override;
    /// Writes the CPU memory map, ignoring writes to ROM regions.
    /// @param address 16-bit CPU address.
    /// @param value Value written by the processor.
    void write(std::uint16_t address, std::uint8_t value) override;
    /// Advances all devices from completed CPU instruction time.
    /// @param cpuCycles Elapsed cycles on the 2 MHz CPU timebase.
    void tick(std::uint32_t cpuCycles) override;

    /// Copies an exact 16 KiB operating-system ROM.
    /// @param rom Source bytes; not retained.
    /// @return Whether the required size was supplied.
    bool loadOSROM(std::span<const std::uint8_t> rom);
    /// Copies up to 16 KiB into a sideways-ROM bank and fills the rest with FF.
    /// @param bank Bank in the range 0...15.
    /// @param rom Source bytes; not retained.
    /// @return Whether bank and size were valid.
    bool loadSidewaysROM(std::uint8_t bank, std::span<const std::uint8_t> rom);
    /// Copies bytes into RAM without wrapping.
    /// @param address First RAM address.
    /// @param bytes Source bytes; not retained.
    /// @return Whether the complete range fits in 32 KiB RAM.
    bool loadRAM(std::uint16_t address, std::span<const std::uint8_t> bytes);
    /// Copies and mounts a validated DFS image.
    /// @param drive Drive 0 or 1.
    /// @param bytes Complete image bytes; not retained.
    /// @param layout SSD or DSD ordering.
    /// @param writable Whether writes to the private image copy are permitted.
    /// @return Whether drive and image were valid.
    bool mountDisc(unsigned drive, std::span<const std::uint8_t> bytes,
                   DiscImage::Layout layout, bool writable = false);
    /// Resets processor and devices while retaining loaded ROMs and discs.
    void reset();
    /// Executes whole instructions until at least `cpuCycles` have elapsed.
    /// @param cpuCycles Minimum CPU-cycle budget.
    /// @return Actual cycles, which may exceed the budget by one instruction.
    std::uint64_t runFor(std::uint64_t cpuCycles);
    /// Executes until a new CRTC frame or the instruction-cycle limit.
    /// @param maximumCycles Maximum budget before returning without a frame.
    /// @return Whether a frame completed.
    bool runUntilFrame(std::uint64_t maximumCycles = 100'000);
    /// Renders the current CRTC/ULA state into the machine-owned frame buffer.
    void renderFrame();

    /// Borrows the processor for advanced core integration.
    /// @return Mutable processor reference valid for this machine's lifetime.
    [[nodiscard]] CPU6502& cpu() noexcept { return cpu_; }
    /// Borrows the processor for inspection.
    /// @return Read-only processor reference valid for this machine's lifetime.
    [[nodiscard]] const CPU6502& cpu() const noexcept { return cpu_; }
    /// Borrows the system VIA.
    /// @return Mutable system-VIA reference valid for this machine's lifetime.
    [[nodiscard]] VIA6522& systemVIA() noexcept { return systemVIA_; }
    /// Borrows the user VIA.
    /// @return Mutable user-VIA reference valid for this machine's lifetime.
    [[nodiscard]] VIA6522& userVIA() noexcept { return userVIA_; }
    /// Borrows the CRTC.
    /// @return Mutable CRTC reference valid for this machine's lifetime.
    [[nodiscard]] CRTC6845& crtc() noexcept { return crtc_; }
    /// Borrows the Video ULA.
    /// @return Mutable Video ULA reference valid for this machine's lifetime.
    [[nodiscard]] VideoULA& videoULA() noexcept { return videoULA_; }
    /// Borrows the sound generator.
    /// @return Mutable sound-chip reference valid for this machine's lifetime.
    [[nodiscard]] SN76489& sound() noexcept { return sound_; }
    /// Borrows the disc controller.
    /// @return Mutable FDC reference valid for this machine's lifetime.
    [[nodiscard]] Intel8271& discController() noexcept { return fdc_; }
    /// Borrows the last rendered frame until the next machine mutation.
    /// @return Read-only frame reference.
    [[nodiscard]] const VideoFrame& frame() const noexcept { return frame_; }
    /// Borrows all 32 KiB RAM until the next machine mutation.
    /// @return Read-only RAM span.
    [[nodiscard]] std::span<const std::uint8_t> ram() const noexcept { return ram_; }
    /// Restores the complete RAM image from a previously captured snapshot.
    /// @param bytes Exactly 32 KiB of RAM contents.
    void restoreRAM(std::span<const std::uint8_t> bytes) noexcept;
    /// Returns the currently selected sideways-ROM bank.
    /// @return Bank number in the range 0...15.
    [[nodiscard]] std::uint8_t selectedROM() const noexcept { return selectedROM_; }
    /// Reports whether a valid operating-system ROM has been loaded.
    /// @return ROM-loaded state.
    [[nodiscard]] bool hasOSROM() const noexcept { return osROMLoaded_; }

    /// Changes one keyboard-matrix bit; out-of-range coordinates are ignored.
    /// @param column Matrix column in the range 0...15.
    /// @param row Matrix row in the range 0...15.
    /// @param pressed Whether the key is pressed.
    void setKey(std::uint8_t column, std::uint8_t row, bool pressed);
    /// Changes BREAK state; a rising edge resets the machine.
    /// @param pressed Whether BREAK is held.
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
