#include <array>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

/// Clean-room builder for deterministic fixed-size C000-origin 16 KiB demo
/// ROMs. The cursor is the next byte offset, address() is C000 + cursor, and
/// patch offsets name operand bytes returned by branch(). Writes are bounds
/// checked; rom() borrows storage valid while this builder lives.
/// Builds the fixed 16 KiB C000 clean-room ROM used by deterministic evidence workloads.
/// `cursor_` is a ROM offset and `address()` is C000+cursor_; patch offsets returned by
/// branch/address helpers are offsets into the same buffer, with bounds checked by at().
/// The returned array is owned by the builder and remains valid until it is destroyed.
class ROMBuilder {
  public:
    ROMBuilder() { rom_.fill(0xEA); }

    std::size_t position() const { return cursor_; }
    std::uint16_t address() const { return static_cast<std::uint16_t>(0xC000 + cursor_); }
    void byte(std::uint8_t value) { rom_.at(cursor_++) = value; }
    void word(std::uint16_t value) {
        byte(static_cast<std::uint8_t>(value));
        byte(static_cast<std::uint8_t>(value >> 8));
    }
    void bytes(std::string_view value) {
        for (const auto character : value)
            byte(static_cast<std::uint8_t>(character));
    }
    void ldaImmediate(std::uint8_t value) {
        byte(0xA9);
        byte(value);
    }
    void ldxImmediate(std::uint8_t value) {
        byte(0xA2);
        byte(value);
    }
    void staAbsolute(std::uint16_t address) {
        byte(0x8D);
        word(address);
    }
    void staAbsoluteX(std::uint16_t address) {
        byte(0x9D);
        word(address);
    }
    void ldaAbsoluteX(std::uint16_t address) {
        byte(0xBD);
        word(address);
    }
    std::size_t branch(std::uint8_t opcode) {
        byte(opcode);
        const auto operand = cursor_;
        byte(0);
        return operand;
    }
    void jump(std::uint16_t address) {
        byte(0x4C);
        word(address);
    }

    void patchBranch(std::size_t operand, std::size_t target) {
        const auto delta =
            static_cast<std::ptrdiff_t>(target) - static_cast<std::ptrdiff_t>(operand + 1);
        if (delta < -128 || delta > 127) throw std::runtime_error("branch is out of range");
        rom_[operand] = static_cast<std::uint8_t>(static_cast<std::int8_t>(delta));
    }

    void patchWord(std::size_t offset, std::uint16_t value) {
        rom_.at(offset) = static_cast<std::uint8_t>(value);
        rom_.at(offset + 1) = static_cast<std::uint8_t>(value >> 8);
    }

    const std::array<std::uint8_t, 0x4000>& rom() const { return rom_; }

  private:
    std::array<std::uint8_t, 0x4000> rom_{};
    std::size_t cursor_ = 0;
};

void writeFile(const std::string& path, const std::array<std::uint8_t, 0x4000>& rom) {
    std::ofstream output(path, std::ios::binary);
    if (!output) throw std::runtime_error("cannot create " + path);
    output.write(reinterpret_cast<const char*>(rom.data()),
                 static_cast<std::streamsize>(rom.size()));
    if (!output) throw std::runtime_error("cannot write " + path);
}

} // namespace

int main(int argc, char** argv) {
    try {
        std::string workload = "mode7";
        std::string outputPath;
        if (argc == 2) {
            outputPath = argv[1];
        } else if (argc == 4 && std::string_view(argv[1]) == "--workload") {
            workload = argv[2];
            outputPath = argv[3];
        } else {
            std::cerr << "usage: make-demo-rom [--workload bitmap|mode7] OUTPUT.rom\n";
            return 1;
        }
        if (workload != "bitmap" && workload != "mode7") {
            throw std::runtime_error("unknown workload: " + workload);
        }

        // Both workloads install vectors, program the CRTC/ULA, fill a known screen region,
        // then park in an idle loop. This shape makes reset placement, branch patches, and
        // rendered output stable across evidence runs; see docs/code/evidence-and-testing.md.
        ROMBuilder b;
        const auto reset = b.address();
        b.byte(0x78); // SEI
        b.byte(0xD8); // CLD
        b.ldxImmediate(0xFF);
        b.byte(0x9A); // TXS

        if (workload == "bitmap") {
            constexpr std::array<std::pair<std::uint8_t, std::uint8_t>, 12> crtc{
                std::pair{std::uint8_t{0}, std::uint8_t{63}},
                {1, 40},
                {2, 51},
                {3, 0x24},
                {4, 30},
                {5, 2},
                {6, 25},
                {7, 28},
                {8, 0},
                {9, 7},
                {12, 0x06},
                {13, 0}};
            for (const auto& [reg, value] : crtc) {
                b.ldaImmediate(reg);
                b.staAbsolute(0xFE00);
                b.ldaImmediate(value);
                b.staAbsolute(0xFE01);
            }
            b.ldaImmediate(0x1C); // one bit per pixel, 2 MHz CRTC
            b.staAbsolute(0xFE20);

            b.ldxImmediate(0);
            b.ldaImmediate(0xAA);
            const auto firstFill = b.position();
            for (std::uint16_t page = 0x3000; page < 0x4000; page += 0x0100) {
                b.staAbsoluteX(page);
            }
            b.byte(0xE8);                                // INX
            const auto firstFillBranch = b.branch(0xD0); // BNE
            b.patchBranch(firstFillBranch, firstFill);

            b.ldxImmediate(0);
            b.ldaImmediate(0x55);
            const auto secondFill = b.position();
            for (std::uint16_t page = 0x4000; page < 0x5000; page += 0x0100) {
                b.staAbsoluteX(page);
            }
            b.byte(0xE8);                                 // INX
            const auto secondFillBranch = b.branch(0xD0); // BNE
            b.patchBranch(secondFillBranch, secondFill);

            const auto idle = b.address();
            b.jump(idle);
            b.patchWord(0x3FFA, idle);
            b.patchWord(0x3FFC, reset);
            b.patchWord(0x3FFE, idle);
            writeFile(outputPath, b.rom());
            std::cout << "wrote 16384-byte clean-room bitmap ROM to " << outputPath << '\n';
            return 0;
        }

        // The 40-column Mode 7 table exercises modeled teletext timing; following clear/copy
        // loops create stable control-code and glyph evidence before the idle trap.
        constexpr std::array<std::pair<std::uint8_t, std::uint8_t>, 12> crtc{
            std::pair{std::uint8_t{0}, std::uint8_t{63}},
            {1, 40},
            {2, 51},
            {3, 0x24},
            {4, 30},
            {5, 2},
            {6, 25},
            {7, 28},
            {8, 0x93},
            {9, 18},
            {12, 0x28},
            {13, 0}};
        for (const auto& [reg, value] : crtc) {
            b.ldaImmediate(reg);
            b.staAbsolute(0xFE00);
            b.ldaImmediate(value);
            b.staAbsolute(0xFE01);
        }
        b.ldaImmediate(0x02); // teletext output, 1 MHz CRTC
        b.staAbsolute(0xFE20);

        // Clear the 1 KiB Mode 7 screen.
        b.ldxImmediate(0);
        b.ldaImmediate(0x20);
        const auto clearLoop = b.position();
        b.staAbsoluteX(0x7C00);
        b.staAbsoluteX(0x7D00);
        b.staAbsoluteX(0x7E00);
        b.staAbsoluteX(0x7F00);
        b.byte(0xE8);                            // INX
        const auto clearBranch = b.branch(0xD0); // BNE
        b.patchBranch(clearBranch, clearLoop);

        // Copy two zero-terminated strings. Their leading bytes select colour.
        b.ldxImmediate(0);
        const auto firstLoop = b.position();
        b.byte(0xBD);
        const auto firstAddressPatch = b.position();
        b.word(0);
        const auto firstDoneBranch = b.branch(0xF0);
        b.staAbsoluteX(0x7C00);
        b.byte(0xE8);
        const auto firstLoopBranch = b.branch(0xD0);
        b.patchBranch(firstLoopBranch, firstLoop);
        const auto firstDone = b.position();
        b.patchBranch(firstDoneBranch, firstDone);

        b.ldxImmediate(0);
        const auto secondLoop = b.position();
        b.byte(0xBD);
        const auto secondAddressPatch = b.position();
        b.word(0);
        const auto secondDoneBranch = b.branch(0xF0);
        b.staAbsoluteX(0x7C50);
        b.byte(0xE8);
        const auto secondLoopBranch = b.branch(0xD0);
        b.patchBranch(secondLoopBranch, secondLoop);
        const auto idle = b.position();
        b.patchBranch(secondDoneBranch, idle);
        b.jump(static_cast<std::uint16_t>(0xC000 + idle));

        const auto firstString = b.address();
        b.byte(0x03);
        b.bytes("BEEB6502 CLEAN-ROOM ROM");
        b.byte(0);
        const auto secondString = b.address();
        b.byte(0x06);
        b.bytes("CPU + VIA + CRTC + ULA ONLINE");
        b.byte(0);
        b.patchWord(firstAddressPatch, firstString);
        b.patchWord(secondAddressPatch, secondString);

        b.patchWord(0x3FFA, static_cast<std::uint16_t>(0xC000 + idle));
        b.patchWord(0x3FFC, reset);
        b.patchWord(0x3FFE, static_cast<std::uint16_t>(0xC000 + idle));
        writeFile(outputPath, b.rom());
        std::cout << "wrote 16384-byte clean-room demo ROM to " << outputPath << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << '\n';
        return 1;
    }
}
