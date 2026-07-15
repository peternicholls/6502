#include "beeb/cpu6502.hpp"
#include "beeb/crtc6845.hpp"
#include "beeb/disc_image.hpp"
#include "beeb/intel8271.hpp"
#include "beeb/machine.hpp"
#include "beeb/runtime.hpp"
#include "beeb/sn76489.hpp"
#include "beeb/via6522.hpp"
#include "beeb_c.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <fstream>
#include <functional>
#include <future>
#include <iostream>
#include <latch>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace {

class RAMBus final : public beeb::Bus {
public:
    std::array<std::uint8_t, 65536> memory{};
    std::uint64_t ticks = 0;
    std::vector<std::pair<std::uint16_t, std::uint8_t>> writes;

    std::uint8_t read(std::uint16_t address) override { return memory[address]; }
    void write(std::uint16_t address, std::uint8_t value) override {
        memory[address] = value;
        writes.emplace_back(address, value);
    }
    void tick(std::uint32_t cycles) override { ticks += cycles; }
};

struct TestFailure : std::runtime_error { using std::runtime_error::runtime_error; };

#define CHECK(expr) do { if (!(expr)) { \
    std::ostringstream os; os << __FILE__ << ':' << __LINE__ << ": CHECK(" #expr ") failed"; \
    throw TestFailure(os.str()); } } while (false)

#define CHECK_EQ(actual, expected) do { const auto a_ = (actual); const auto e_ = (expected); \
    if (a_ != e_) { std::ostringstream os; os << __FILE__ << ':' << __LINE__ \
    << ": expected " << +e_ << ", got " << +a_; throw TestFailure(os.str()); } } while (false)

using Test = std::pair<std::string, std::function<void()>>;

beeb::CPU6502 makeCPU(RAMBus& bus, std::uint16_t pc = 0x0200) {
    beeb::CPU6502 cpu(bus);
    beeb::CPUState state;
    state.pc = pc;
    state.sp = 0xFD;
    state.p = beeb::CPU6502::Unused;
    cpu.setState(state);
    return cpu;
}

void testResetAndSimpleProgram() {
    RAMBus bus;
    bus.memory[0xFFFC] = 0x00;
    bus.memory[0xFFFD] = 0x80;
    bus.memory[0x8000] = 0xA9; // LDA #$2A
    bus.memory[0x8001] = 0x2A;
    bus.memory[0x8002] = 0x85; // STA $70
    bus.memory[0x8003] = 0x70;
    bus.memory[0x8004] = 0xE8; // INX

    beeb::CPU6502 cpu(bus);
    cpu.reset();
    CHECK_EQ(cpu.state().pc, 0x8000);
    CHECK_EQ(cpu.state().sp, 0xFD);
    CHECK_EQ(cpu.state().cycles, 7);
    CHECK_EQ(cpu.step(), 2);
    CHECK_EQ(cpu.step(), 3);
    CHECK_EQ(cpu.step(), 2);
    CHECK_EQ(bus.memory[0x70], 0x2A);
    CHECK_EQ(cpu.state().x, 1);
    CHECK_EQ(bus.ticks, 14);
}

void testAddressingWrapAndPageCycles() {
    RAMBus bus;
    auto cpu = makeCPU(bus);
    bus.memory[0x0200] = 0xB1; // LDA ($FF),Y
    bus.memory[0x0201] = 0xFF;
    bus.memory[0x00FF] = 0xFF;
    bus.memory[0x0000] = 0x20; // zero-page pointer wraps
    bus.memory[0x2100] = 0x77;
    auto state = cpu.state();
    state.y = 1;
    cpu.setState(state);
    CHECK_EQ(cpu.step(), 6); // five plus page crossing
    CHECK_EQ(cpu.state().a, 0x77);
}

void testJMPIndirectHardwareBug() {
    RAMBus bus;
    auto cpu = makeCPU(bus);
    bus.memory[0x0200] = 0x6C;
    bus.memory[0x0201] = 0xFF;
    bus.memory[0x0202] = 0x30;
    bus.memory[0x30FF] = 0x34;
    bus.memory[0x3000] = 0x12;
    bus.memory[0x3100] = 0x99;
    CHECK_EQ(cpu.step(), 5);
    CHECK_EQ(cpu.state().pc, 0x1234);
}

void testSubroutineAndStack() {
    RAMBus bus;
    auto cpu = makeCPU(bus);
    bus.memory[0x0200] = 0x20; // JSR $0300
    bus.memory[0x0201] = 0x00;
    bus.memory[0x0202] = 0x03;
    bus.memory[0x0300] = 0x60; // RTS
    CHECK_EQ(cpu.step(), 6);
    CHECK_EQ(cpu.state().pc, 0x0300);
    CHECK_EQ(bus.memory[0x01FD], 0x02);
    CHECK_EQ(bus.memory[0x01FC], 0x02);
    CHECK_EQ(cpu.step(), 6);
    CHECK_EQ(cpu.state().pc, 0x0203);
    CHECK_EQ(cpu.state().sp, 0xFD);
}

void testInterrupts() {
    RAMBus bus;
    auto cpu = makeCPU(bus);
    bus.memory[0xFFFA] = 0x00;
    bus.memory[0xFFFB] = 0x40;
    bus.memory[0x4000] = 0x40; // RTI
    auto initial = cpu.state();
    initial.p = beeb::CPU6502::Unused | beeb::CPU6502::Decimal;
    cpu.setState(initial);
    cpu.requestNMI();
    CHECK_EQ(cpu.step(), 7);
    CHECK_EQ(cpu.state().pc, 0x4000);
    CHECK(cpu.flag(beeb::CPU6502::InterruptDisable));
    CHECK(cpu.flag(beeb::CPU6502::Decimal)); // NMOS interrupt does not clear D
    CHECK_EQ(cpu.step(), 6);
    CHECK_EQ(cpu.state().pc, 0x0200);
    CHECK(!cpu.flag(beeb::CPU6502::InterruptDisable));
    CHECK(cpu.flag(beeb::CPU6502::Decimal));
}

void testBranchTiming() {
    RAMBus bus;
    auto cpu = makeCPU(bus, 0x02FD);
    bus.memory[0x02FD] = 0xD0; // BNE +1, instruction ends at $02FF
    bus.memory[0x02FE] = 0x01; // target $0300: crosses page
    CHECK_EQ(cpu.step(), 4);
    CHECK_EQ(cpu.state().pc, 0x0300);

    bus.memory[0x0300] = 0xF0; // BEQ, not taken
    bus.memory[0x0301] = 0x7F;
    CHECK_EQ(cpu.step(), 2);
    CHECK_EQ(cpu.state().pc, 0x0302);
}

void testBinaryADCExhaustive() {
    RAMBus bus;
    bus.memory[0x0200] = 0x69;
    beeb::CPU6502 cpu(bus);
    for (unsigned a = 0; a < 256; ++a) {
        for (unsigned operand = 0; operand < 256; ++operand) {
            for (unsigned carry = 0; carry < 2; ++carry) {
                bus.memory[0x0201] = static_cast<std::uint8_t>(operand);
                beeb::CPUState s;
                s.a = static_cast<std::uint8_t>(a);
                s.p = static_cast<std::uint8_t>(beeb::CPU6502::Unused | (carry ? beeb::CPU6502::Carry : 0));
                s.pc = 0x0200;
                cpu.setState(s);
                cpu.step();
                const auto result = a + operand + carry;
                const auto byte = static_cast<std::uint8_t>(result);
                CHECK_EQ(cpu.state().a, byte);
                CHECK_EQ(cpu.flag(beeb::CPU6502::Carry), result > 255);
                CHECK_EQ(cpu.flag(beeb::CPU6502::Zero), byte == 0);
                CHECK_EQ(cpu.flag(beeb::CPU6502::Negative), (byte & 0x80) != 0);
                CHECK_EQ(cpu.flag(beeb::CPU6502::Overflow), ((~(a ^ operand) & (a ^ byte)) & 0x80) != 0);
            }
        }
    }
}

void testBinarySBCExhaustive() {
    RAMBus bus;
    bus.memory[0x0200] = 0xE9;
    beeb::CPU6502 cpu(bus);
    for (unsigned a = 0; a < 256; ++a) {
        for (unsigned operand = 0; operand < 256; ++operand) {
            for (unsigned carry = 0; carry < 2; ++carry) {
                bus.memory[0x0201] = static_cast<std::uint8_t>(operand);
                beeb::CPUState s;
                s.a = static_cast<std::uint8_t>(a);
                s.p = static_cast<std::uint8_t>(beeb::CPU6502::Unused | (carry ? beeb::CPU6502::Carry : 0));
                s.pc = 0x0200;
                cpu.setState(s);
                cpu.step();
                const int signedResult = static_cast<int>(a) - static_cast<int>(operand) - (carry ? 0 : 1);
                const auto byte = static_cast<std::uint8_t>(signedResult);
                CHECK_EQ(cpu.state().a, byte);
                CHECK_EQ(cpu.flag(beeb::CPU6502::Carry), signedResult >= 0);
                CHECK_EQ(cpu.flag(beeb::CPU6502::Zero), byte == 0);
                CHECK_EQ(cpu.flag(beeb::CPU6502::Negative), (byte & 0x80) != 0);
                CHECK_EQ(cpu.flag(beeb::CPU6502::Overflow), (((a ^ operand) & (a ^ byte)) & 0x80) != 0);
            }
        }
    }
}

std::uint8_t bcd(unsigned value) {
    return static_cast<std::uint8_t>(((value / 10) << 4) | (value % 10));
}

void testValidBCDArithmeticExhaustive() {
    RAMBus bus;
    beeb::CPU6502 cpu(bus);
    for (unsigned a = 0; a < 100; ++a) {
        for (unsigned operand = 0; operand < 100; ++operand) {
            for (unsigned carry = 0; carry < 2; ++carry) {
                bus.memory[0x0200] = 0x69;
                bus.memory[0x0201] = bcd(operand);
                beeb::CPUState s;
                s.a = bcd(a);
                s.p = static_cast<std::uint8_t>(beeb::CPU6502::Unused | beeb::CPU6502::Decimal | (carry ? beeb::CPU6502::Carry : 0));
                s.pc = 0x0200;
                cpu.setState(s);
                cpu.step();
                const auto sum = a + operand + carry;
                CHECK_EQ(cpu.state().a, bcd(sum % 100));
                CHECK_EQ(cpu.flag(beeb::CPU6502::Carry), sum >= 100);

                bus.memory[0x0200] = 0xE9;
                s.a = bcd(a);
                s.p = static_cast<std::uint8_t>(beeb::CPU6502::Unused | beeb::CPU6502::Decimal | (carry ? beeb::CPU6502::Carry : 0));
                s.pc = 0x0200;
                cpu.setState(s);
                cpu.step();
                int difference = static_cast<int>(a) - static_cast<int>(operand) - (carry ? 0 : 1);
                const bool noBorrow = difference >= 0;
                if (difference < 0) difference += 100;
                CHECK_EQ(cpu.state().a, bcd(static_cast<unsigned>(difference)));
                CHECK_EQ(cpu.flag(beeb::CPU6502::Carry), noBorrow);
            }
        }
    }
}

void testNMOSDecimalFlagVectors() {
    struct Vector { std::uint8_t opcode, a, value; bool carryIn; std::uint8_t result; bool n, v, z, c; };
    const std::array vectors{
        Vector{0x69, 0x00, 0x00, false, 0x00, false, false, true,  false},
        Vector{0x69, 0x79, 0x00, true,  0x80, true,  true,  false, false},
        Vector{0x69, 0x24, 0x56, false, 0x80, true,  true,  false, false},
        Vector{0x69, 0x93, 0x82, false, 0x75, false, true,  false, true},
        Vector{0x69, 0x89, 0x76, true,  0x66, false, false, true,  true},
        Vector{0x69, 0x80, 0xF0, false, 0xD0, false, true,  false, true},
        Vector{0x69, 0x80, 0xFA, false, 0xE0, true,  false, false, true},
        Vector{0xE9, 0x00, 0x00, false, 0x99, true,  false, false, false},
        Vector{0xE9, 0x00, 0x00, true,  0x00, false, false, true,  true},
        Vector{0xE9, 0x00, 0x01, true,  0x99, true,  false, false, false},
        Vector{0xE9, 0x0B, 0x00, false, 0x0A, false, false, false, true},
        Vector{0xE9, 0x9B, 0x00, false, 0x9A, true,  false, false, true},
    };

    RAMBus bus;
    beeb::CPU6502 cpu(bus);
    for (const auto& vector : vectors) {
        bus.memory[0x0200] = vector.opcode;
        bus.memory[0x0201] = vector.value;
        beeb::CPUState s;
        s.a = vector.a;
        s.pc = 0x0200;
        s.p = static_cast<std::uint8_t>(beeb::CPU6502::Unused | beeb::CPU6502::Decimal | (vector.carryIn ? beeb::CPU6502::Carry : 0));
        cpu.setState(s);
        cpu.step();
        CHECK_EQ(cpu.state().a, vector.result);
        CHECK_EQ(cpu.flag(beeb::CPU6502::Negative), vector.n);
        CHECK_EQ(cpu.flag(beeb::CPU6502::Overflow), vector.v);
        CHECK_EQ(cpu.flag(beeb::CPU6502::Zero), vector.z);
        CHECK_EQ(cpu.flag(beeb::CPU6502::Carry), vector.c);
    }
}

void testAllOfficialOpcodesDecode() {
    constexpr std::array<std::uint8_t, 151> official{
        0x00,0x01,0x05,0x06,0x08,0x09,0x0A,0x0D,0x0E,0x10,0x11,0x15,0x16,0x18,0x19,0x1D,0x1E,
        0x20,0x21,0x24,0x25,0x26,0x28,0x29,0x2A,0x2C,0x2D,0x2E,0x30,0x31,0x35,0x36,0x38,0x39,0x3D,0x3E,
        0x40,0x41,0x45,0x46,0x48,0x49,0x4A,0x4C,0x4D,0x4E,0x50,0x51,0x55,0x56,0x58,0x59,0x5D,0x5E,
        0x60,0x61,0x65,0x66,0x68,0x69,0x6A,0x6C,0x6D,0x6E,0x70,0x71,0x75,0x76,0x78,0x79,0x7D,0x7E,
        0x81,0x84,0x85,0x86,0x88,0x8A,0x8C,0x8D,0x8E,0x90,0x91,0x94,0x95,0x96,0x98,0x99,0x9A,0x9D,
        0xA0,0xA1,0xA2,0xA4,0xA5,0xA6,0xA8,0xA9,0xAA,0xAC,0xAD,0xAE,0xB0,0xB1,0xB4,0xB5,0xB6,0xB8,0xB9,0xBA,0xBC,0xBD,0xBE,
        0xC0,0xC1,0xC4,0xC5,0xC6,0xC8,0xC9,0xCA,0xCC,0xCD,0xCE,0xD0,0xD1,0xD5,0xD6,0xD8,0xD9,0xDD,0xDE,
        0xE0,0xE1,0xE4,0xE5,0xE6,0xE8,0xE9,0xEA,0xEC,0xED,0xEE,0xF0,0xF1,0xF5,0xF6,0xF8,0xF9,0xFD,0xFE,
    };

    for (const auto opcode : official) {
        RAMBus bus;
        bus.memory.fill(0);
        bus.memory[0x0200] = opcode;
        bus.memory[0xFFFE] = 0;
        bus.memory[0xFFFF] = 2;
        auto cpu = makeCPU(bus);
        try { cpu.step(); }
        catch (const std::exception& e) {
            throw TestFailure(std::string("official opcode rejected: ") + e.what());
        }
    }
}

void testIllegalOpcodeTraps() {
    RAMBus bus;
    auto cpu = makeCPU(bus);
    bus.memory[0x0200] = 0x02;
    bool threw = false;
    try { cpu.step(); } catch (const std::runtime_error&) { threw = true; }
    CHECK(threw);
}

void testCAPIContainsIllegalOpcodeErrors() {
    beeb_machine* machine = beeb_create();
    CHECK(machine != nullptr);

    std::array<std::uint8_t, 0x4000> os{};
    os.fill(0xEA);
    os[0] = 0x02; // unsupported opcode at the reset address
    os[0x3FFC] = 0x00;
    os[0x3FFD] = 0xC0;
    CHECK_EQ(beeb_load_os_rom(machine, os.data(), os.size()), 1);
    beeb_reset(machine);

    CHECK_EQ(beeb_run_cycles(machine, 1), 0);
    CHECK(std::string(beeb_last_error(machine)).find("unsupported NMOS 6502 opcode $02") != std::string::npos);

    beeb_reset(machine);
    CHECK(std::string(beeb_last_error(machine)).empty());
    CHECK_EQ(beeb_run_until_frame(machine, 1), -1);
    CHECK(std::string(beeb_last_error(machine)).find("unsupported NMOS 6502 opcode $02") != std::string::npos);

    beeb_destroy(machine);
}

void testCAPIBoundaryFailuresAreRecoverable() {
    CHECK(std::string(beeb_version_string()) == "0.1.0");
    CHECK(beeb_last_error(nullptr) == nullptr);
    beeb_destroy(nullptr);
    beeb_reset(nullptr);
    CHECK_EQ(beeb_run_cycles(nullptr, 1), 0);
    CHECK_EQ(beeb_run_until_frame(nullptr, 1), -1);
    const auto nullState = beeb_get_cpu_state(nullptr);
    CHECK_EQ(nullState.pc, 0);
    CHECK_EQ(nullState.cycles, 0);
    CHECK(beeb_get_frame_rgba(nullptr, nullptr, nullptr, nullptr) == nullptr);
    beeb_render_audio(nullptr, nullptr, 1, 44'100.0);
    beeb_set_key(nullptr, 0, 0, 1);
    beeb_set_break(nullptr, 1);

    beeb_machine* machine = beeb_create();
    CHECK(machine != nullptr);
    CHECK(std::string(beeb_last_error(machine)).empty());

    CHECK_EQ(beeb_load_os_rom(machine, nullptr, 0), 0);
    CHECK(std::string(beeb_last_error(machine)) == "OS ROM data is null");

    std::array<std::uint8_t, 0x4000> os{};
    os.fill(0xEA);
    os[0x3FFC] = 0x00;
    os[0x3FFD] = 0xC0;
    CHECK_EQ(beeb_load_os_rom(machine, os.data(), os.size() - 1), 0);
    CHECK(std::string(beeb_last_error(machine)) == "OS ROM must be exactly 16384 bytes");
    CHECK_EQ(beeb_load_os_rom(machine, os.data(), os.size()), 1);
    CHECK(std::string(beeb_last_error(machine)).empty());

    CHECK_EQ(beeb_load_sideways_rom(machine, 0, nullptr, 0), 0);
    CHECK(std::string(beeb_last_error(machine)) == "sideways ROM data is null");
    CHECK_EQ(beeb_load_sideways_rom(machine, 16, os.data(), os.size()), 0);
    CHECK(std::string(beeb_last_error(machine)) == "invalid sideways ROM bank or size");
    CHECK_EQ(beeb_load_sideways_rom(machine, 0, os.data(), os.size()), 1);
    CHECK(std::string(beeb_last_error(machine)).empty());

    CHECK_EQ(beeb_mount_disc(machine, 0, nullptr, 0, 0, 0), 0);
    CHECK(std::string(beeb_last_error(machine)) == "disc image data is null");
    const std::array<std::uint8_t, 1> invalidDisc{0};
    CHECK_EQ(beeb_mount_disc(machine, 2, invalidDisc.data(), invalidDisc.size(), 0, 0), 0);
    CHECK(std::string(beeb_last_error(machine)) == "invalid drive or disc image");

    beeb_reset(machine);
    CHECK(std::string(beeb_last_error(machine)).empty());
    const auto state = beeb_get_cpu_state(machine);
    CHECK_EQ(state.pc, 0xC000);
    std::uint32_t width = 99;
    std::uint32_t height = 99;
    std::uint64_t number = 99;
    CHECK(beeb_get_frame_rgba(machine, &width, &height, &number) == nullptr);
    CHECK_EQ(width, 0);
    CHECK_EQ(height, 0);
    CHECK_EQ(number, 0);

    beeb_render_audio(machine, nullptr, 1, 44'100.0);
    CHECK(std::string(beeb_last_error(machine)) == "audio output buffer is null");
    float sample = 0.0f;
    beeb_render_audio(machine, &sample, 1, 0.0);
    CHECK(std::string(beeb_last_error(machine)) == "audio sample rate must be finite and positive");
    beeb_render_audio(machine, &sample, 1, 44'100.0);
    CHECK(std::string(beeb_last_error(machine)).empty());

    beeb_set_key(machine, 255, 255, 1);
    CHECK(std::string(beeb_last_error(machine)).empty());
    beeb_set_break(machine, 1);
    beeb_set_break(machine, 0);
    CHECK(std::string(beeb_last_error(machine)).empty());
    beeb_destroy(machine);
}

void testVIATimerAndInterruptEnable() {
    beeb::VIA6522 via;
    via.reset();
    via.write(0xE, 0xC0); // enable Timer 1 interrupt
    via.write(0x4, 0x01);
    via.write(0x5, 0x00); // load counter = 1
    CHECK(!via.irq());
    via.tick(1);
    CHECK(!via.irq());
    via.tick(1);
    CHECK(via.irq());
    CHECK((via.read(0xD) & 0xC0) == 0xC0);
    (void)via.read(0x4); // reading T1 low clears its flag
    CHECK(!via.irq());
}

void testVIADataDirectionsAndEdges() {
    beeb::VIA6522 via;
    via.reset();
    via.setPortAInput([] { return static_cast<std::uint8_t>(0xA5); });
    via.write(0x3, 0xF0);
    via.write(0x1, 0x3C);
    CHECK_EQ(via.read(0xF), 0x35); // high nibble output, low nibble input
    via.write(0xE, 0x82); // enable CA1
    via.setCA1(false);    // default PCR selects falling edge
    CHECK(via.irq());
    (void)via.read(0x1);
    CHECK(!via.irq());
}

void testCRTCFrameTiming() {
    beeb::CRTC6845 crtc;
    crtc.reset();
    const auto set = [&](std::uint8_t reg, std::uint8_t value) { crtc.select(reg); crtc.write(value); };
    set(0, 3); // four character clocks per scanline
    set(1, 2);
    set(4, 1); // two character rows
    set(5, 0);
    set(6, 2);
    set(9, 1); // two raster rows per character
    crtc.tick(15);
    CHECK(!crtc.frameReady());
    crtc.tick(1);
    CHECK(crtc.frameReady());
    CHECK_EQ(crtc.frameNumber(), 1);
    crtc.consumeFrame();
    CHECK(!crtc.frameReady());
}

void checkCRTCVerticalAdjustTiming(std::uint8_t adjustment) {
    beeb::CRTC6845 crtc;
    crtc.reset();
    const auto set = [&](std::uint8_t reg, std::uint8_t value) { crtc.select(reg); crtc.write(value); };
    set(0, 0); // one character clock per scanline
    set(4, 0); // one character row
    set(5, adjustment);
    set(9, 0); // one raster row per character

    crtc.tick(1); // complete the character row and enter vertical adjustment
    CHECK(!crtc.frameReady());
    for (std::uint8_t line = 1; line < adjustment; ++line) {
        crtc.tick(1);
        CHECK(!crtc.frameReady());
    }
    crtc.tick(1);
    CHECK(crtc.frameReady());
    CHECK_EQ(crtc.frameNumber(), 1);
}

void testCRTCOneLineVerticalAdjust() {
    checkCRTCVerticalAdjustTiming(1);
}

void testCRTCTwoLineVerticalAdjust() {
    checkCRTCVerticalAdjustTiming(2);
}

void testSoundRegisterProtocolAndRendering() {
    beeb::SN76489 sound;
    sound.reset();
    sound.write(0x80 | 0x05); // tone 0 low nibble
    sound.write(0x10);        // tone 0 high six bits => $105
    sound.write(0x90 | 0x00); // channel 0, maximum volume
    CHECK_EQ(sound.tonePeriod(0), 0x105);
    CHECK_EQ(sound.volume(0), 0);
    std::array<float, 512> samples{};
    sound.render(samples.data(), samples.size(), 48'000.0);
    bool nonZero = false;
    for (const auto sample : samples) nonZero = nonZero || sample != 0.0f;
    CHECK(nonZero);
}

void testBBCMemoryMapAndROMSelection() {
    beeb::BBCMicro machine;
    std::array<std::uint8_t, 0x4000> os{};
    os.fill(0xEA);
    os[0x3FFC] = 0x00;
    os[0x3FFD] = 0xC0;
    CHECK(machine.loadOSROM(os));
    std::array<std::uint8_t, 0x4000> basic{};
    basic.fill(0x42);
    CHECK(machine.loadSidewaysROM(12, basic));
    machine.reset();
    CHECK_EQ(machine.cpu().state().pc, 0xC000);
    CHECK_EQ(machine.read(0xC000), 0xEA);
    machine.write(0xFE30, 12);
    CHECK_EQ(machine.selectedROM(), 12);
    CHECK_EQ(machine.read(0x8000), 0x42);
    machine.write(0x1234, 0x99);
    CHECK_EQ(machine.read(0x1234), 0x99);
    machine.write(0x8000, 0x11);
    CHECK_EQ(machine.read(0x8000), 0x42); // ROM remains read-only
}

void testBBCBitmapFrameRendering() {
    beeb::BBCMicro machine;
    const auto setCRTC = [&](std::uint8_t reg, std::uint8_t value) {
        machine.write(0xFE00, reg);
        machine.write(0xFE01, value);
    };
    setCRTC(1, 1);  // eight output pixels
    setCRTC(6, 1);  // one character row
    setCRTC(9, 0);  // one raster line
    setCRTC(12, 0);
    setCRTC(13, 0);
    machine.write(0xFE20, 0x1C); // 80-column serializer, 2 MHz CRTC
    machine.write(0x0000, 0xA0); // pixels 1,0,1,0,0,0,0,0
    machine.renderFrame();
    const auto& frame = machine.frame();
    CHECK_EQ(frame.width, 8);
    CHECK_EQ(frame.height, 1);
    CHECK_EQ(frame.rgba.size(), 32);
    CHECK_EQ(frame.rgba[0], 255); // colour 1 = red
    CHECK_EQ(frame.rgba[1], 0);
    CHECK_EQ(frame.rgba[4], 0);   // second pixel black
    CHECK_EQ(frame.rgba[8], 255); // third pixel red
}

void testSSDAndDSDSectorLayout() {
    std::vector<std::uint8_t> ssd(40 * 10 * 256, 0);
    ssd[(3 * 10 + 7) * 256] = 0x37;
    beeb::DiscImage image;
    CHECK(image.load(ssd, beeb::DiscImage::Layout::SSD));
    CHECK_EQ(image.tracks(), 40);
    CHECK_EQ(image.sides(), 1);
    std::array<std::uint8_t, 256> sector{};
    CHECK(image.readSector(3, 0, 7, sector));
    CHECK_EQ(sector[0], 0x37);

    std::vector<std::uint8_t> dsd(40 * 2 * 10 * 256, 0);
    dsd[((2 * 2 + 1) * 10 + 4) * 256] = 0xD4;
    CHECK(image.load(dsd, beeb::DiscImage::Layout::DSD));
    CHECK_EQ(image.sides(), 2);
    CHECK(image.readSector(2, 1, 4, sector));
    CHECK_EQ(sector[0], 0xD4);
}

void test8271SectorReadProtocol() {
    std::vector<std::uint8_t> ssd(40 * 10 * 256, 0);
    for (unsigned byte = 0; byte < 256; ++byte) ssd[(1 * 10 + 2) * 256 + byte] = static_cast<std::uint8_t>(byte);
    beeb::Intel8271 fdc;
    unsigned nmis = 0;
    fdc.setNMICallback([&] { ++nmis; });
    fdc.reset();
    CHECK(fdc.mount(0, ssd, beeb::DiscImage::Layout::SSD));
    fdc.write(0, 0x53); // drive 0 + READ SECTORS, three parameters
    fdc.write(1, 1);    // track
    fdc.write(1, 2);    // first sector
    fdc.write(1, 0x21); // one 256-byte sector
    CHECK((fdc.status() & 0x80) != 0);
    for (unsigned byte = 0; byte < 256; ++byte) {
        fdc.tick(64);
        CHECK((fdc.status() & 0x0C) == 0x0C);
        CHECK_EQ(fdc.read(4), static_cast<std::uint8_t>(byte));
    }
    fdc.tick(64);
    CHECK((fdc.status() & 0x18) == 0x18); // completion NMI + result
    CHECK_EQ(fdc.read(1), 0);
    CHECK_EQ(fdc.status() & 0x18, 0);
    CHECK_EQ(nmis, 257);
}

void testBBCFDCMemoryMap() {
    beeb::BBCMicro machine;
    std::vector<std::uint8_t> ssd(40 * 10 * 256, 0xA7);
    CHECK(machine.mountDisc(0, ssd, beeb::DiscImage::Layout::SSD));
    machine.write(0xFE80, 0x53);
    machine.write(0xFE81, 0);
    machine.write(0xFE81, 0);
    machine.write(0xFE81, 0x21);
    machine.discController().tick(64);
    CHECK_EQ(machine.read(0xFE84), 0xA7);
}

void testCleanRoomTeletextRendering() {
    beeb::CRTC6845 crtc;
    crtc.reset();
    crtc.select(1); crtc.write(2);
    crtc.select(6); crtc.write(1);
    crtc.select(12); crtc.write(0);
    crtc.select(13); crtc.write(0);
    std::array<std::uint8_t, 0x8000> ram{};
    ram[0x7C00] = 0x01; // alpha red
    ram[0x7C01] = 'A';
    beeb::TeletextRenderer renderer;
    const auto bitmap = renderer.render(ram, crtc, 0);
    CHECK_EQ(bitmap.width, 24);
    CHECK_EQ(bitmap.height, 20);
    unsigned redPixels = 0;
    for (std::size_t offset = 0; offset < bitmap.rgba.size(); offset += 4) {
        if (bitmap.rgba[offset] == 255 && bitmap.rgba[offset + 1] == 0 && bitmap.rgba[offset + 2] == 0) ++redPixels;
    }
    CHECK(redPixels > 0);
}

void testTeletextControlCellsUseActiveBackground() {
    beeb::CRTC6845 crtc;
    crtc.reset();
    crtc.select(1); crtc.write(4);
    crtc.select(6); crtc.write(1);
    crtc.select(12); crtc.write(0);
    crtc.select(13); crtc.write(0);
    std::array<std::uint8_t, 0x8000> ram{};
    ram[0x7C00] = 0x01; // alpha red: set after this cell
    ram[0x7C01] = 0x1D; // new background: set after this cell
    ram[0x7C02] = 0x02; // alpha green control on the active red background
    ram[0x7C03] = ' ';

    beeb::TeletextRenderer renderer;
    const auto bitmap = renderer.render(ram, crtc, 0);
    const auto pixel = [&](unsigned column) {
        return (static_cast<std::size_t>(10) * bitmap.width + column * 12 + 6) * 4;
    };

    const auto beforeBackground = pixel(1);
    CHECK_EQ(bitmap.rgba[beforeBackground], 0);
    CHECK_EQ(bitmap.rgba[beforeBackground + 1], 0);
    CHECK_EQ(bitmap.rgba[beforeBackground + 2], 0);

    const auto controlCell = pixel(2);
    CHECK_EQ(bitmap.rgba[controlCell], 255);
    CHECK_EQ(bitmap.rgba[controlCell + 1], 0);
    CHECK_EQ(bitmap.rgba[controlCell + 2], 0);
}

std::array<std::uint8_t, 0x4000> makeNOPOSROM() {
    std::array<std::uint8_t, 0x4000> os{};
    os.fill(0xEA);
    os[0x3FFC] = 0x00;
    os[0x3FFD] = 0xC0;
    return os;
}

void checkRuntimeOK(const beeb::RuntimeStatus& status) {
    CHECK(status.code == beeb::RuntimeStatusCode::ok);
    CHECK(status.message.empty());
}

template <typename T>
const T& runtimeValue(const beeb::RuntimeResult<T>& result) {
    checkRuntimeOK(result.status);
    CHECK(result.value.has_value());
    return *result.value;
}

void loadNOPFixture(beeb::MachineRuntime& runtime) {
    const auto os = makeNOPOSROM();
    checkRuntimeOK(runtime.loadOSROM(os));
    checkRuntimeOK(runtime.reset());
}

void testC1RuntimeContractLifecycleMatrix() {
    beeb::MachineRuntime runtime;
    CHECK(runtimeValue(runtime.state()) == beeb::RuntimeState::paused);
    loadNOPFixture(runtime);

    checkRuntimeOK(runtime.pause());
    checkRuntimeOK(runtime.start());
    checkRuntimeOK(runtime.start());
    CHECK(runtimeValue(runtime.state()) == beeb::RuntimeState::running);

    const auto boundedWhileRunning = runtime.runFor(1);
    CHECK(boundedWhileRunning.status.code == beeb::RuntimeStatusCode::invalidState);
    CHECK(!boundedWhileRunning.value.has_value());

    checkRuntimeOK(runtime.pause());
    checkRuntimeOK(runtime.pause());
    CHECK(runtimeValue(runtime.state()) == beeb::RuntimeState::paused);
    CHECK(runtimeValue(runtime.runFor(0)) == 0);

    checkRuntimeOK(runtime.shutdown());
    CHECK(runtime.start().code == beeb::RuntimeStatusCode::unavailable);
    CHECK(runtime.pause().code == beeb::RuntimeStatusCode::unavailable);
    CHECK(runtime.reset().code == beeb::RuntimeStatusCode::unavailable);
    CHECK(runtime.state().status.code == beeb::RuntimeStatusCode::unavailable);
}

void testC1RuntimeContractFaultAndRecoveryMatrix() {
    beeb::MachineRuntime runtime;
    auto illegal = makeNOPOSROM();
    illegal[0] = 0x02;
    checkRuntimeOK(runtime.loadOSROM(illegal));
    checkRuntimeOK(runtime.reset());

    const auto failed = runtime.runFor(1);
    CHECK(failed.status.code == beeb::RuntimeStatusCode::executionFailed);
    CHECK(!failed.status.message.empty());
    CHECK(!failed.value.has_value());
    CHECK(runtimeValue(runtime.state()) == beeb::RuntimeState::faulted);
    const auto fault = runtimeValue(runtime.fault());
    CHECK(fault.available);
    CHECK(fault.message == failed.status.message);
    CHECK(fault.safePoint.state == beeb::RuntimeState::faulted);

    CHECK(runtime.start().code == beeb::RuntimeStatusCode::invalidState);
    CHECK(runtime.pause().code == beeb::RuntimeStatusCode::invalidState);
    CHECK(runtime.loadOSROM(makeNOPOSROM()).code == beeb::RuntimeStatusCode::invalidState);
    (void)runtimeValue(runtime.cpuState());
    (void)runtimeValue(runtime.frame());

    checkRuntimeOK(runtime.reset());
    CHECK(runtimeValue(runtime.state()) == beeb::RuntimeState::paused);
    CHECK(!runtimeValue(runtime.fault()).available);
    checkRuntimeOK(runtime.loadOSROM(makeNOPOSROM()));
    checkRuntimeOK(runtime.reset());
    CHECK(runtimeValue(runtime.runFor(1)) >= 1);
}

void testC1RuntimeContractStructuredStatusIsolation() {
    beeb::MachineRuntime runtime;
    const auto first = runtime.loadOSROM(std::span<const std::uint8_t>{});
    CHECK(first.code == beeb::RuntimeStatusCode::invalidArgument);
    CHECK(!first.message.empty());

    const auto second = runtime.setKey(16, 0, true);
    CHECK(second.code == beeb::RuntimeStatusCode::invalidArgument);
    CHECK(!second.message.empty());
    CHECK(first.message != second.message);

    loadNOPFixture(runtime);
    const auto success = runtime.setKey(0, 0, true);
    checkRuntimeOK(success);
    CHECK(!first.message.empty());
    CHECK(runtimeValue(runtime.frame()).available == false);
}

struct C1ReplaySignature {
    beeb::CPUState cpu;
    beeb::SafePoint safePoint;
    std::vector<beeb::LedgerEntry> ledger;
};

C1ReplaySignature runC1ReplayScenario() {
    beeb::MachineRuntime runtime({.enableLedger = true});
    loadNOPFixture(runtime);
    CHECK(runtimeValue(runtime.runFor(17)) >= 17);
    checkRuntimeOK(runtime.setKey(1, 2, true));
    CHECK(runtimeValue(runtime.runFor(31)) >= 31);
    checkRuntimeOK(runtime.setKey(1, 2, false));
    return {runtimeValue(runtime.cpuState()), runtimeValue(runtime.safePoint()), runtime.ledger()};
}

void testC1ReplayDeterministicLedger() {
    const auto expected = runC1ReplayScenario();
    CHECK(!expected.ledger.empty());
    for (unsigned repetition = 1; repetition < 10; ++repetition) {
        const auto actual = runC1ReplayScenario();
        CHECK(actual.cpu == expected.cpu);
        CHECK(actual.safePoint == expected.safePoint);
        CHECK(actual.ledger == expected.ledger);
    }
}

struct C1CapturedReplay {
    beeb::CPUState cpu;
    beeb::SafePoint safePoint;
    std::vector<beeb::LedgerEntry> ledger;
};

C1CapturedReplay captureC1ConcurrentLedger() {
    beeb::MachineRuntime runtime({.enableLedger = true});
    loadNOPFixture(runtime);

    constexpr unsigned callerCount = 12;
    std::latch ready(callerCount);
    std::latch release(1);
    std::vector<std::future<beeb::RuntimeStatus>> calls;
    calls.reserve(callerCount);
    for (unsigned index = 0; index < callerCount; ++index) {
        calls.emplace_back(std::async(std::launch::async, [&runtime, &ready, &release, index] {
            ready.count_down();
            release.wait();
            switch (index % 3) {
            case 0: return runtime.start();
            case 1: return runtime.pause();
            default: return runtime.state().status;
            }
        }));
    }
    ready.wait();
    release.count_down();
    for (auto& call : calls) checkRuntimeOK(call.get());
    checkRuntimeOK(runtime.pause());

    const auto cpu = runtimeValue(runtime.cpuState());
    const auto safePoint = runtimeValue(runtime.safePoint());
    return {cpu, safePoint, runtime.ledger()};
}

struct C1ReplayOutcome {
    beeb::CPUState cpu;
    beeb::SafePoint safePoint;

    friend bool operator==(const C1ReplayOutcome&, const C1ReplayOutcome&) = default;
};

C1ReplayOutcome replayC1Ledger(const std::vector<beeb::LedgerEntry>& ledger) {
    beeb::BBCMicro machine;
    auto state = beeb::RuntimeState::paused;
    auto previousSequence = std::uint64_t{0};
    auto previousAcceptance = std::uint64_t{0};
    beeb::SafePoint finalSafePoint{};
    const auto os = makeNOPOSROM();

    for (const auto& entry : ledger) {
        CHECK(entry.sequence > previousSequence);
        previousSequence = entry.sequence;
        CHECK(entry.safePoint.ledgerSequence == entry.sequence);
        CHECK(entry.status == beeb::RuntimeStatusCode::ok);

        if (entry.event == beeb::LedgerEventKind::executionSlice) {
            CHECK(state == beeb::RuntimeState::running);
            CHECK_EQ(entry.requestedCycles, beeb::MachineRuntime::executionSliceCycles);
            CHECK_EQ(machine.runFor(entry.requestedCycles), entry.actualCycles);
        } else {
            CHECK(entry.acceptanceSequence > previousAcceptance);
            previousAcceptance = entry.acceptanceSequence;
            switch (entry.command) {
            case beeb::RuntimeCommandKind::loadOSROM:
                CHECK(entry.payloadDigest != 0);
                CHECK(machine.loadOSROM(os));
                break;
            case beeb::RuntimeCommandKind::reset:
                machine.reset();
                state = beeb::RuntimeState::paused;
                break;
            case beeb::RuntimeCommandKind::start:
                state = beeb::RuntimeState::running;
                break;
            case beeb::RuntimeCommandKind::pause:
                state = beeb::RuntimeState::paused;
                break;
            case beeb::RuntimeCommandKind::runtimeState:
                CHECK_EQ(entry.resultDigest, static_cast<std::uint64_t>(state) + 1);
                break;
            case beeb::RuntimeCommandKind::cpuState:
            case beeb::RuntimeCommandKind::safePoint:
                CHECK(entry.resultDigest != 0);
                break;
            default:
                throw TestFailure("unexpected command in C1 concurrent replay ledger");
            }
        }

        CHECK_EQ(machine.cpu().state().cycles, entry.safePoint.cpuCycles);
        CHECK_EQ(machine.frame().number, entry.safePoint.frameNumber);
        CHECK(state == entry.safePoint.state);
        finalSafePoint = entry.safePoint;
    }
    return {machine.cpu().state(), finalSafePoint};
}

void writeC1ReplayEvidence(const C1CapturedReplay& capture) {
    const auto* directory = std::getenv("BEEB_C1_EVIDENCE_DIR");
    if (!directory || *directory == '\0') return;
    std::filesystem::create_directories(directory);
    std::ofstream output(std::filesystem::path(directory) / "accepted-ledger.txt");
    CHECK(output.good());
    output << "sequence event command acceptance requested actual payload result status "
              "cpu_cycles frame state\n";
    for (const auto& entry : capture.ledger) {
        output << entry.sequence << ' '
               << static_cast<unsigned>(entry.event) << ' '
               << static_cast<unsigned>(entry.command) << ' '
               << entry.acceptanceSequence << ' '
               << entry.requestedCycles << ' '
               << entry.actualCycles << ' '
               << entry.payloadDigest << ' '
               << entry.resultDigest << ' '
               << static_cast<unsigned>(entry.status) << ' '
               << entry.safePoint.cpuCycles << ' '
               << entry.safePoint.frameNumber << ' '
               << static_cast<unsigned>(entry.safePoint.state) << '\n';
    }
}

void testC1ReplayCapturedConcurrentLedgerExactly() {
    const auto capture = captureC1ConcurrentLedger();
    CHECK(!capture.ledger.empty());
    writeC1ReplayEvidence(capture);

    const C1ReplayOutcome expected{capture.cpu, capture.safePoint};
    for (unsigned repetition = 0; repetition < 10; ++repetition) {
        CHECK(replayC1Ledger(capture.ledger) == expected);
    }
}

void testC1RuntimeFixedExecutionSlices() {
    beeb::MachineRuntime runtime({.enableLedger = true});
    loadNOPFixture(runtime);
    checkRuntimeOK(runtime.start());

    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    bool observedSlice = false;
    while (std::chrono::steady_clock::now() < deadline) {
        const auto ledger = runtime.ledger();
        observedSlice = std::any_of(ledger.begin(), ledger.end(), [](const auto& entry) {
            return entry.event == beeb::LedgerEventKind::executionSlice;
        });
        if (observedSlice) break;
        std::this_thread::yield();
    }
    CHECK(observedSlice);
    checkRuntimeOK(runtime.pause());

    const auto ledger = runtime.ledger();
    std::uint64_t previousSequence = 0;
    for (const auto& entry : ledger) {
        CHECK(entry.sequence > previousSequence);
        previousSequence = entry.sequence;
        CHECK(entry.safePoint.ledgerSequence == entry.sequence);
        if (entry.event == beeb::LedgerEventKind::executionSlice) {
            CHECK_EQ(entry.requestedCycles, beeb::MachineRuntime::executionSliceCycles);
            CHECK(entry.actualCycles >= entry.requestedCycles);
            CHECK(entry.safePoint.cpuCycles >= entry.actualCycles);
        }
    }
}

void testC1PauseCompletesWithinOneAcceptedSlice() {
    beeb::MachineRuntime runtime({.enableLedger = true});
    loadNOPFixture(runtime);
    checkRuntimeOK(runtime.start());

    const auto sliceDeadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    while (std::chrono::steady_clock::now() < sliceDeadline) {
        const auto entries = runtime.ledger();
        if (std::any_of(entries.begin(), entries.end(), [](const auto& entry) {
                return entry.event == beeb::LedgerEventKind::executionSlice;
            })) break;
        std::this_thread::yield();
    }

    const auto acceptedBeforePause = runtime.acceptedCommandCount();
    auto pause = std::async(std::launch::async, [&runtime] { return runtime.pause(); });
    const auto acceptanceDeadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    while (runtime.acceptedCommandCount() == acceptedBeforePause &&
           std::chrono::steady_clock::now() < acceptanceDeadline) {
        std::this_thread::yield();
    }
    CHECK(runtime.acceptedCommandCount() == acceptedBeforePause + 1);

    const auto atAcceptance = runtime.ledger();
    const auto lastSequenceAtAcceptance =
        atAcceptance.empty() ? 0 : atAcceptance.back().sequence;
    const auto pauseStatus = pause.get();
    checkRuntimeOK(pauseStatus);

    unsigned slicesAfterObservedAcceptance = 0;
    bool foundPause = false;
    for (const auto& entry : runtime.ledger()) {
        if (entry.event == beeb::LedgerEventKind::command &&
            entry.command == beeb::RuntimeCommandKind::pause &&
            entry.acceptanceSequence == pauseStatus.acceptanceSequence) {
            foundPause = true;
            break;
        }
        if (entry.sequence > lastSequenceAtAcceptance &&
            entry.event == beeb::LedgerEventKind::executionSlice) {
            ++slicesAfterObservedAcceptance;
        }
    }
    CHECK(foundPause);
    CHECK(slicesAfterObservedAcceptance <= 1);
    CHECK(runtimeValue(runtime.state()) == beeb::RuntimeState::paused);
}

void testC1RaceMixedCommands() {
    beeb::MachineRuntime runtime({.enableLedger = true});
    loadNOPFixture(runtime);
    constexpr unsigned threadCount = 8;
    constexpr unsigned operationsPerThread = 1'250;
    std::atomic<unsigned> completed{0};
    std::atomic<unsigned> failed{0};
    std::vector<std::thread> workers;
    workers.reserve(threadCount);

    for (unsigned thread = 0; thread < threadCount; ++thread) {
        workers.emplace_back([&, thread] {
            for (unsigned operation = 0; operation < operationsPerThread; ++operation) {
                beeb::RuntimeStatus status;
                switch ((thread + operation) % 5) {
                case 0: status = runtime.start(); break;
                case 1: status = runtime.pause(); break;
                case 2: status = runtime.reset(); break;
                case 3: status = runtime.setKey(
                    static_cast<std::uint8_t>(thread),
                    static_cast<std::uint8_t>(operation % 8),
                    (operation & 1) != 0); break;
                default: status = runtime.cpuState().status; break;
                }
                if (status.code == beeb::RuntimeStatusCode::ok) ++completed;
                else ++failed;
            }
        });
    }
    for (auto& worker : workers) worker.join();

    CHECK_EQ(completed.load() + failed.load(), threadCount * operationsPerThread);
    CHECK_EQ(failed.load(), 0);
    checkRuntimeOK(runtime.pause());
    CHECK(runtime.ledger().size() >= completed.load());
}

void testC1RaceShutdownDrainAndRejection() {
    beeb::MachineRuntime runtime;
    loadNOPFixture(runtime);
    std::vector<std::future<beeb::RuntimeStatus>> calls;
    calls.reserve(96);
    for (unsigned index = 0; index < 96; ++index) {
        calls.emplace_back(std::async(std::launch::async, [&runtime, index] {
            if ((index & 1) == 0) return runtime.cpuState().status;
            return runtime.setKey(
                static_cast<std::uint8_t>(index % 16),
                static_cast<std::uint8_t>((index / 16) % 16), true);
        }));
    }
    checkRuntimeOK(runtime.shutdown());
    for (auto& call : calls) {
        const auto status = call.get();
        CHECK(status.code == beeb::RuntimeStatusCode::ok ||
              status.code == beeb::RuntimeStatusCode::unavailable);
    }
    CHECK(runtime.start().code == beeb::RuntimeStatusCode::unavailable);
}

} // namespace

int main(int argc, char** argv) {
    bool quick = false;
    std::string filter;
    for (int index = 1; index < argc; ++index) {
        const std::string argument = argv[index];
        if (argument == "--quick") {
            quick = true;
        } else if (argument == "--filter" && index + 1 < argc) {
            filter = argv[++index];
        } else {
            std::cerr << "usage: beeb-tests [--quick] [--filter text]\n";
            return 2;
        }
    }
    const std::vector<Test> tests{
        {"reset and simple program", testResetAndSimpleProgram},
        {"address wrapping and page cycles", testAddressingWrapAndPageCycles},
        {"JMP indirect hardware bug", testJMPIndirectHardwareBug},
        {"subroutine and stack", testSubroutineAndStack},
        {"interrupt entry and RTI", testInterrupts},
        {"branch timing", testBranchTiming},
        {"binary ADC exhaustive", testBinaryADCExhaustive},
        {"binary SBC exhaustive", testBinarySBCExhaustive},
        {"valid BCD arithmetic exhaustive", testValidBCDArithmeticExhaustive},
        {"NMOS decimal flag vectors", testNMOSDecimalFlagVectors},
        {"all 151 official opcodes decode", testAllOfficialOpcodesDecode},
        {"illegal opcode trap", testIllegalOpcodeTraps},
        {"C API contains illegal opcode errors", testCAPIContainsIllegalOpcodeErrors},
        {"C API boundary failures are recoverable", testCAPIBoundaryFailuresAreRecoverable},
        {"VIA timer and interrupt enable", testVIATimerAndInterruptEnable},
        {"VIA data direction and CA1 edge", testVIADataDirectionsAndEdges},
        {"CRTC frame timing", testCRTCFrameTiming},
        {"CRTC one-line vertical adjustment", testCRTCOneLineVerticalAdjust},
        {"CRTC two-line vertical adjustment", testCRTCTwoLineVerticalAdjust},
        {"SN76489 register protocol and rendering", testSoundRegisterProtocolAndRendering},
        {"BBC memory map and ROM selection", testBBCMemoryMapAndROMSelection},
        {"BBC bitmap frame rendering", testBBCBitmapFrameRendering},
        {"SSD and DSD sector layout", testSSDAndDSDSectorLayout},
        {"8271 sector read protocol", test8271SectorReadProtocol},
        {"BBC 8271 memory map", testBBCFDCMemoryMap},
        {"clean-room teletext rendering", testCleanRoomTeletextRendering},
        {"teletext control cells use active background", testTeletextControlCellsUseActiveBackground},
        {"C1 contract: lifecycle command matrix", testC1RuntimeContractLifecycleMatrix},
        {"C1 contract: fault and recovery matrix", testC1RuntimeContractFaultAndRecoveryMatrix},
        {"C1 contract: structured status isolation", testC1RuntimeContractStructuredStatusIsolation},
        {"C1 replay: deterministic command and safe-point ledger", testC1ReplayDeterministicLedger},
        {"C1 replay: captured concurrent ledger replays exactly", testC1ReplayCapturedConcurrentLedgerExactly},
        {"C1 contract: fixed execution slices share ledger order", testC1RuntimeFixedExecutionSlices},
        {"C1 lifecycle: accepted pause completes within one slice", testC1PauseCompletesWithinOneAcceptedSlice},
        {"C1 race: 10000 mixed commands", testC1RaceMixedCommands},
        {"C1 race: shutdown drain and rejection", testC1RaceShutdownDrainAndRejection},
    };

    unsigned failed = 0;
    std::size_t executed = 0;
    for (const auto& [name, test] : tests) {
        if (quick && (name.ends_with("exhaustive") || name.starts_with("C1 race:"))) continue;
        if (!filter.empty() && name.find(filter) == std::string::npos) continue;
        ++executed;
        try {
            test();
            std::cout << "PASS  " << name << '\n';
        } catch (const std::exception& error) {
            ++failed;
            std::cerr << "FAIL  " << name << "\n      " << error.what() << '\n';
        }
    }
    if (executed == 0) {
        std::cerr << "no tests matched filter: " << filter << '\n';
        return 2;
    }
    std::cout << "\n" << (executed - failed) << '/' << executed << " tests passed\n";
    return failed == 0 ? 0 : 1;
}
