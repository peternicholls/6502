#include "beeb/cpu6502.hpp"
#include "beeb/crtc6845.hpp"
#include "beeb/disc_image.hpp"
#include "beeb/intel8271.hpp"
#include "beeb/machine.hpp"
#include "beeb/profile.hpp"
#include "beeb/runtime.hpp"
#include "beeb/sn76489.hpp"
#include "beeb/via6522.hpp"
#include "beeb_c.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdlib>
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

beeb_status beeb_test_create_with_allocation_failure(beeb_machine**,
                                                     beeb::RuntimeAllocationFailurePoint);
beeb_status beeb_test_hold_admitted_call(beeb_machine*, std::latch&, std::latch&);

// The C profile tests are committed red before their public implementation.
// Weak failing definitions let independently ordered core tasks link; the real
// strong C boundary replaces them when its later task is implemented.
extern "C" __attribute__((weak)) beeb_status beeb_create_with_profile(const beeb_machine_profile*,
                                                                      beeb_machine**) {
    beeb_status result{};
    result.code = BEEB_STATUS_INTERNAL_FAILURE;
    return result;
}
extern "C" __attribute__((weak)) beeb_status beeb_get_machine_profile(beeb_machine*,
                                                                      beeb_machine_profile*) {
    beeb_status result{};
    result.code = BEEB_STATUS_INTERNAL_FAILURE;
    return result;
}

namespace {

// C0-DOC-RATIONALE: docs/code/evidence-and-testing.md owns why these fixtures
// retain exact hardware, boundary, replay, and concurrency observations.

/// Deterministic fake bus that captures device ticks and every write for assertions
/// about NMOS bus-visible behavior; reads come from a complete 64 KiB backing store.
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

/// Assertion boundary used by the registry runner to report one failing fixture cleanly.
struct TestFailure : std::runtime_error {
    using std::runtime_error::runtime_error;
};

#define CHECK(expr)                                                                                \
    do {                                                                                           \
        if (!(expr)) {                                                                             \
            std::ostringstream os;                                                                 \
            os << __FILE__ << ':' << __LINE__ << ": CHECK(" #expr ") failed";                      \
            throw TestFailure(os.str());                                                           \
        }                                                                                          \
    } while (false)

#define CHECK_EQ(actual, expected)                                                                 \
    do {                                                                                           \
        const auto a_ = (actual);                                                                  \
        const auto e_ = (expected);                                                                \
        if (a_ != e_) {                                                                            \
            std::ostringstream os;                                                                 \
            os << __FILE__ << ':' << __LINE__ << ": expected " << +e_ << ", got " << +a_;          \
            throw TestFailure(os.str());                                                           \
        }                                                                                          \
    } while (false)

/// Name/callable pair forming the test registry consumed by the command-line runner.
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
                s.p = static_cast<std::uint8_t>(beeb::CPU6502::Unused |
                                                (carry ? beeb::CPU6502::Carry : 0));
                s.pc = 0x0200;
                cpu.setState(s);
                cpu.step();
                const auto result = a + operand + carry;
                const auto byte = static_cast<std::uint8_t>(result);
                CHECK_EQ(cpu.state().a, byte);
                CHECK_EQ(cpu.flag(beeb::CPU6502::Carry), result > 255);
                CHECK_EQ(cpu.flag(beeb::CPU6502::Zero), byte == 0);
                CHECK_EQ(cpu.flag(beeb::CPU6502::Negative), (byte & 0x80) != 0);
                CHECK_EQ(cpu.flag(beeb::CPU6502::Overflow),
                         ((~(a ^ operand) & (a ^ byte)) & 0x80) != 0);
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
                s.p = static_cast<std::uint8_t>(beeb::CPU6502::Unused |
                                                (carry ? beeb::CPU6502::Carry : 0));
                s.pc = 0x0200;
                cpu.setState(s);
                cpu.step();
                const int signedResult =
                    static_cast<int>(a) - static_cast<int>(operand) - (carry ? 0 : 1);
                const auto byte = static_cast<std::uint8_t>(signedResult);
                CHECK_EQ(cpu.state().a, byte);
                CHECK_EQ(cpu.flag(beeb::CPU6502::Carry), signedResult >= 0);
                CHECK_EQ(cpu.flag(beeb::CPU6502::Zero), byte == 0);
                CHECK_EQ(cpu.flag(beeb::CPU6502::Negative), (byte & 0x80) != 0);
                CHECK_EQ(cpu.flag(beeb::CPU6502::Overflow),
                         (((a ^ operand) & (a ^ byte)) & 0x80) != 0);
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
                s.p = static_cast<std::uint8_t>(beeb::CPU6502::Unused | beeb::CPU6502::Decimal |
                                                (carry ? beeb::CPU6502::Carry : 0));
                s.pc = 0x0200;
                cpu.setState(s);
                cpu.step();
                const auto sum = a + operand + carry;
                CHECK_EQ(cpu.state().a, bcd(sum % 100));
                CHECK_EQ(cpu.flag(beeb::CPU6502::Carry), sum >= 100);

                bus.memory[0x0200] = 0xE9;
                s.a = bcd(a);
                s.p = static_cast<std::uint8_t>(beeb::CPU6502::Unused | beeb::CPU6502::Decimal |
                                                (carry ? beeb::CPU6502::Carry : 0));
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
    // These vectors preserve the difficult NMOS decimal datapath provenance: unlike
    // CMOS BCD expectations, N/V/Z reflect internal binary/intermediate values.
    /// One NMOS decimal ADC/SBC input and its expected accumulator/flag outcome.
    struct Vector {
        std::uint8_t opcode, a, value;
        bool carryIn;
        std::uint8_t result;
        bool n, v, z, c;
    };
    const std::array vectors{
        Vector{0x69, 0x00, 0x00, false, 0x00, false, false, true, false},
        Vector{0x69, 0x79, 0x00, true, 0x80, true, true, false, false},
        Vector{0x69, 0x24, 0x56, false, 0x80, true, true, false, false},
        Vector{0x69, 0x93, 0x82, false, 0x75, false, true, false, true},
        Vector{0x69, 0x89, 0x76, true, 0x66, false, false, true, true},
        Vector{0x69, 0x80, 0xF0, false, 0xD0, false, true, false, true},
        Vector{0x69, 0x80, 0xFA, false, 0xE0, true, false, false, true},
        Vector{0xE9, 0x00, 0x00, false, 0x99, true, false, false, false},
        Vector{0xE9, 0x00, 0x00, true, 0x00, false, false, true, true},
        Vector{0xE9, 0x00, 0x01, true, 0x99, true, false, false, false},
        Vector{0xE9, 0x0B, 0x00, false, 0x0A, false, false, false, true},
        Vector{0xE9, 0x9B, 0x00, false, 0x9A, true, false, false, true},
    };

    RAMBus bus;
    beeb::CPU6502 cpu(bus);
    for (const auto& vector : vectors) {
        bus.memory[0x0200] = vector.opcode;
        bus.memory[0x0201] = vector.value;
        beeb::CPUState s;
        s.a = vector.a;
        s.pc = 0x0200;
        s.p = static_cast<std::uint8_t>(beeb::CPU6502::Unused | beeb::CPU6502::Decimal |
                                        (vector.carryIn ? beeb::CPU6502::Carry : 0));
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
        0x00, 0x01, 0x05, 0x06, 0x08, 0x09, 0x0A, 0x0D, 0x0E, 0x10, 0x11, 0x15, 0x16, 0x18,
        0x19, 0x1D, 0x1E, 0x20, 0x21, 0x24, 0x25, 0x26, 0x28, 0x29, 0x2A, 0x2C, 0x2D, 0x2E,
        0x30, 0x31, 0x35, 0x36, 0x38, 0x39, 0x3D, 0x3E, 0x40, 0x41, 0x45, 0x46, 0x48, 0x49,
        0x4A, 0x4C, 0x4D, 0x4E, 0x50, 0x51, 0x55, 0x56, 0x58, 0x59, 0x5D, 0x5E, 0x60, 0x61,
        0x65, 0x66, 0x68, 0x69, 0x6A, 0x6C, 0x6D, 0x6E, 0x70, 0x71, 0x75, 0x76, 0x78, 0x79,
        0x7D, 0x7E, 0x81, 0x84, 0x85, 0x86, 0x88, 0x8A, 0x8C, 0x8D, 0x8E, 0x90, 0x91, 0x94,
        0x95, 0x96, 0x98, 0x99, 0x9A, 0x9D, 0xA0, 0xA1, 0xA2, 0xA4, 0xA5, 0xA6, 0xA8, 0xA9,
        0xAA, 0xAC, 0xAD, 0xAE, 0xB0, 0xB1, 0xB4, 0xB5, 0xB6, 0xB8, 0xB9, 0xBA, 0xBC, 0xBD,
        0xBE, 0xC0, 0xC1, 0xC4, 0xC5, 0xC6, 0xC8, 0xC9, 0xCA, 0xCC, 0xCD, 0xCE, 0xD0, 0xD1,
        0xD5, 0xD6, 0xD8, 0xD9, 0xDD, 0xDE, 0xE0, 0xE1, 0xE4, 0xE5, 0xE6, 0xE8, 0xE9, 0xEA,
        0xEC, 0xED, 0xEE, 0xF0, 0xF1, 0xF5, 0xF6, 0xF8, 0xF9, 0xFD, 0xFE,
    };

    for (const auto opcode : official) {
        RAMBus bus;
        bus.memory.fill(0);
        bus.memory[0x0200] = opcode;
        bus.memory[0xFFFE] = 0;
        bus.memory[0xFFFF] = 2;
        auto cpu = makeCPU(bus);
        try {
            cpu.step();
        } catch (const std::exception& e) {
            throw TestFailure(std::string("official opcode rejected: ") + e.what());
        }
    }
}

void testIllegalOpcodeTraps() {
    RAMBus bus;
    auto cpu = makeCPU(bus);
    bus.memory[0x0200] = 0x02;
    bool threw = false;
    try {
        cpu.step();
    } catch (const std::runtime_error&) {
        threw = true;
    }
    CHECK(threw);
}

void testTraceObserverFailureIsAtomic() {
    RAMBus bus;
    auto cpu = makeCPU(bus);
    bus.memory[0x0200] = 0xEA; // NOP
    beeb::CPUState observed{};
    std::uint8_t observedOpcode = 0;
    cpu.setTraceCallback([&](const beeb::CPUState& state, std::uint8_t opcode) {
        observed = state;
        observedOpcode = opcode;
    });
    const auto tracedBefore = cpu.state();
    CHECK_EQ(cpu.step(), 2);
    auto expectedObserved = tracedBefore;
    ++expectedObserved.pc;
    CHECK(observed == expectedObserved);
    CHECK_EQ(observedOpcode, 0xEA);

    bus.memory[cpu.state().pc] = 0xEA;
    const auto before = cpu.state();
    cpu.setTraceCallback(
        [](const beeb::CPUState&, std::uint8_t) { throw std::runtime_error("trace failed"); });
    bool threw = false;
    try {
        cpu.step();
    } catch (const std::runtime_error&) {
        threw = true;
    }
    CHECK(threw);
    CHECK(cpu.state() == before);
    CHECK_EQ(bus.ticks, 2);
}

void checkCStatus(const beeb_status& status, beeb_status_code expected) {
    if (status.code != expected) {
        std::ostringstream message;
        message << "C status expected " << static_cast<int>(expected) << ", got "
                << static_cast<int>(status.code) << ": " << status.message;
        throw TestFailure(message.str());
    }
    CHECK(status.message[BEEB_STATUS_MESSAGE_CAPACITY - 1] == '\0');
    if (expected == BEEB_STATUS_OK) CHECK(status.message[0] == '\0');
}

beeb_machine* createCMachine() {
    beeb_machine* machine = nullptr;
    checkCStatus(beeb_create(&machine), BEEB_STATUS_OK);
    CHECK(machine != nullptr);
    return machine;
}

void testCAPI02StatusOutParametersAndNullability() {
    // Declaration-driven C 0.2 matrix: keep one applicable success and one
    // output-preserving validation failure for every fallible header family.
    CHECK(std::string(beeb_version_string()) == "0.3.0");
    checkCStatus(beeb_create(nullptr), BEEB_STATUS_INVALID_ARGUMENT);
    checkCStatus(beeb_destroy(nullptr), BEEB_STATUS_INVALID_ARGUMENT);

    beeb_runtime_state runtimeState = BEEB_RUNTIME_STATE_FAULTED;
    checkCStatus(beeb_get_runtime_state(nullptr, &runtimeState), BEEB_STATUS_INVALID_ARGUMENT);
    CHECK(runtimeState == BEEB_RUNTIME_STATE_FAULTED);
    checkCStatus(beeb_get_runtime_state(nullptr, nullptr), BEEB_STATUS_INVALID_ARGUMENT);
    checkCStatus(beeb_start(nullptr), BEEB_STATUS_INVALID_ARGUMENT);
    checkCStatus(beeb_pause(nullptr), BEEB_STATUS_INVALID_ARGUMENT);
    checkCStatus(beeb_reset(nullptr), BEEB_STATUS_INVALID_ARGUMENT);

    std::uint64_t actualCycles = 77;
    checkCStatus(beeb_run_cycles(nullptr, 1, &actualCycles), BEEB_STATUS_INVALID_ARGUMENT);
    CHECK_EQ(actualCycles, 77);
    checkCStatus(beeb_run_cycles(nullptr, 1, nullptr), BEEB_STATUS_INVALID_ARGUMENT);
    int completedFrame = 7;
    checkCStatus(beeb_run_until_frame(nullptr, 1, &completedFrame), BEEB_STATUS_INVALID_ARGUMENT);
    CHECK_EQ(completedFrame, 7);
    checkCStatus(beeb_run_until_frame(nullptr, 1, nullptr), BEEB_STATUS_INVALID_ARGUMENT);

    beeb_cpu_state untouchedState{};
    untouchedState.pc = 0x1234;
    untouchedState.cycles = 99;
    checkCStatus(beeb_get_cpu_state(nullptr, &untouchedState), BEEB_STATUS_INVALID_ARGUMENT);
    CHECK_EQ(untouchedState.pc, 0x1234);
    CHECK_EQ(untouchedState.cycles, 99);
    checkCStatus(beeb_get_cpu_state(nullptr, nullptr), BEEB_STATUS_INVALID_ARGUMENT);

    beeb_safe_point untouchedPoint{};
    untouchedPoint.cpu_cycles = 123;
    checkCStatus(beeb_get_safe_point(nullptr, &untouchedPoint), BEEB_STATUS_INVALID_ARGUMENT);
    CHECK_EQ(untouchedPoint.cpu_cycles, 123);
    checkCStatus(beeb_get_safe_point(nullptr, nullptr), BEEB_STATUS_INVALID_ARGUMENT);
    beeb_fault_detail untouchedFault{};
    untouchedFault.available = 1;
    checkCStatus(beeb_get_fault(nullptr, &untouchedFault), BEEB_STATUS_INVALID_ARGUMENT);
    CHECK_EQ(untouchedFault.available, 1);
    checkCStatus(beeb_get_fault(nullptr, nullptr), BEEB_STATUS_INVALID_ARGUMENT);

    beeb_frame untouchedFrame{};
    untouchedFrame.available = 1;
    untouchedFrame.width = 99;
    checkCStatus(beeb_get_frame(nullptr, &untouchedFrame), BEEB_STATUS_INVALID_ARGUMENT);
    CHECK_EQ(untouchedFrame.available, 1);
    CHECK_EQ(untouchedFrame.width, 99);
    checkCStatus(beeb_frame_release(nullptr), BEEB_STATUS_INVALID_ARGUMENT);

    float sample = 42.0f;
    checkCStatus(beeb_render_audio(nullptr, &sample, 1, 48'000.0), BEEB_STATUS_INVALID_ARGUMENT);
    CHECK(sample == 42.0f);
    checkCStatus(beeb_set_key(nullptr, 0, 0, 1), BEEB_STATUS_INVALID_ARGUMENT);
    checkCStatus(beeb_set_break(nullptr, 1), BEEB_STATUS_INVALID_ARGUMENT);
    checkCStatus(beeb_load_os_rom(nullptr, nullptr, 0), BEEB_STATUS_INVALID_ARGUMENT);
    checkCStatus(beeb_load_sideways_rom(nullptr, 0, nullptr, 0), BEEB_STATUS_INVALID_ARGUMENT);
    checkCStatus(beeb_mount_disc(nullptr, 0, nullptr, 0, 0, 0), BEEB_STATUS_INVALID_ARGUMENT);

    auto* machine = createCMachine();
    const auto missingROM = beeb_load_os_rom(machine, nullptr, 0);
    checkCStatus(missingROM, BEEB_STATUS_INVALID_ARGUMENT);
    CHECK(std::string(missingROM.message) == "OS ROM data is null");
    const auto staleStatus = missingROM;

    std::array<std::uint8_t, 0x4000> os{};
    os.fill(0xEA);
    os[0x3FFC] = 0x00;
    os[0x3FFD] = 0xC0;
    checkCStatus(beeb_load_os_rom(machine, os.data(), os.size()), BEEB_STATUS_OK);
    CHECK(staleStatus.code == BEEB_STATUS_INVALID_ARGUMENT);
    CHECK(std::string(staleStatus.message) == "OS ROM data is null");
    checkCStatus(beeb_reset(machine), BEEB_STATUS_OK);
    checkCStatus(beeb_start(machine), BEEB_STATUS_OK);
    checkCStatus(beeb_get_runtime_state(machine, &runtimeState), BEEB_STATUS_OK);
    CHECK(runtimeState == BEEB_RUNTIME_STATE_RUNNING);
    checkCStatus(beeb_pause(machine), BEEB_STATUS_OK);
    checkCStatus(beeb_get_runtime_state(machine, &runtimeState), BEEB_STATUS_OK);
    CHECK(runtimeState == BEEB_RUNTIME_STATE_PAUSED);

    completedFrame = 7;
    checkCStatus(beeb_run_until_frame(machine, 0, &completedFrame), BEEB_STATUS_OK);
    CHECK_EQ(completedFrame, 0);

    actualCycles = 0;
    checkCStatus(beeb_run_cycles(machine, 1, &actualCycles), BEEB_STATUS_OK);
    CHECK(actualCycles >= 1);
    beeb_cpu_state state{};
    checkCStatus(beeb_get_cpu_state(machine, &state), BEEB_STATUS_OK);
    CHECK(state.pc >= 0xC001);
    checkCStatus(beeb_get_cpu_state(machine, nullptr), BEEB_STATUS_INVALID_ARGUMENT);
    beeb_safe_point point{};
    checkCStatus(beeb_get_safe_point(machine, &point), BEEB_STATUS_OK);
    beeb_fault_detail fault{};
    checkCStatus(beeb_get_fault(machine, &fault), BEEB_STATUS_OK);
    CHECK_EQ(fault.available, 0);

    const std::array<std::uint8_t, 1> tinySideways{0x42};
    checkCStatus(beeb_load_sideways_rom(machine, 0, tinySideways.data(), tinySideways.size()),
                 BEEB_STATUS_OK);
    checkCStatus(beeb_load_sideways_rom(machine, 0, tinySideways.data(), 0),
                 BEEB_STATUS_INVALID_ARGUMENT);
    std::vector<std::uint8_t> oversizedSideways(0x4001, 0);
    checkCStatus(
        beeb_load_sideways_rom(machine, 0, oversizedSideways.data(), oversizedSideways.size()),
        BEEB_STATUS_INVALID_ARGUMENT);
    const std::vector<std::uint8_t> disc(40 * 10 * 256, 0);
    checkCStatus(beeb_mount_disc(machine, 0, disc.data(), disc.size(), 0, 0), BEEB_STATUS_OK);

    beeb_frame frame{};
    checkCStatus(beeb_get_frame(machine, &frame), BEEB_STATUS_OK);
    CHECK_EQ(frame.available, 0);
    CHECK(frame.rgba == nullptr);
    CHECK_EQ(frame.rgba_size, 0);
    checkCStatus(beeb_frame_release(&frame), BEEB_STATUS_OK);

    sample = 42.0f;
    checkCStatus(beeb_render_audio(machine, nullptr, 1, 48'000.0), BEEB_STATUS_INVALID_ARGUMENT);
    checkCStatus(beeb_render_audio(machine, &sample, 1, 0.0), BEEB_STATUS_INVALID_ARGUMENT);
    CHECK(sample == 42.0f);
    checkCStatus(beeb_render_audio(machine, &sample, 1, 48'000.0), BEEB_STATUS_OK);
    checkCStatus(beeb_set_key(machine, 16, 0, 1), BEEB_STATUS_INVALID_ARGUMENT);
    checkCStatus(beeb_set_key(machine, 1, 2, 1), BEEB_STATUS_OK);
    checkCStatus(beeb_set_break(machine, 1), BEEB_STATUS_OK);
    checkCStatus(beeb_set_break(machine, 0), BEEB_STATUS_OK);
    checkCStatus(beeb_destroy(machine), BEEB_STATUS_OK);
    checkCStatus(beeb_start(machine), BEEB_STATUS_INVALID_ARGUMENT);

    beeb_machine* allocationMachine = nullptr;
    checkCStatus(beeb_test_create_with_allocation_failure(
                     &allocationMachine, beeb::RuntimeAllocationFailurePoint::frame),
                 BEEB_STATUS_OK);
    beeb_frame untouchedAllocationFrame{};
    untouchedAllocationFrame.available = 1;
    untouchedAllocationFrame.width = 37;
    checkCStatus(beeb_get_frame(allocationMachine, &untouchedAllocationFrame),
                 BEEB_STATUS_RESOURCE_EXHAUSTED);
    CHECK_EQ(untouchedAllocationFrame.available, 1);
    CHECK_EQ(untouchedAllocationFrame.width, 37);
    beeb_frame recoveredFrame{};
    checkCStatus(beeb_get_frame(allocationMachine, &recoveredFrame), BEEB_STATUS_OK);
    checkCStatus(beeb_frame_release(&recoveredFrame), BEEB_STATUS_OK);
    checkCStatus(beeb_destroy(allocationMachine), BEEB_STATUS_OK);
}

void testCAPI02FaultAndRecovery() {
    auto* machine = createCMachine();
    std::array<std::uint8_t, 0x4000> os{};
    os.fill(0xEA);
    os[0] = 0x02;
    os[0x3FFC] = 0x00;
    os[0x3FFD] = 0xC0;
    checkCStatus(beeb_load_os_rom(machine, os.data(), os.size()), BEEB_STATUS_OK);
    checkCStatus(beeb_reset(machine), BEEB_STATUS_OK);

    std::uint64_t actualCycles = 99;
    const auto execution = beeb_run_cycles(machine, 1, &actualCycles);
    checkCStatus(execution, BEEB_STATUS_EXECUTION_FAILED);
    CHECK_EQ(actualCycles, 99);
    CHECK(std::string(execution.message).find("unsupported NMOS 6502 opcode $02") !=
          std::string::npos);

    beeb_runtime_state state = BEEB_RUNTIME_STATE_PAUSED;
    checkCStatus(beeb_get_runtime_state(machine, &state), BEEB_STATUS_OK);
    CHECK(state == BEEB_RUNTIME_STATE_FAULTED);
    beeb_fault_detail fault{};
    checkCStatus(beeb_get_fault(machine, &fault), BEEB_STATUS_OK);
    CHECK_EQ(fault.available, 1);
    CHECK(std::string(fault.message) == std::string(execution.message));
    CHECK(fault.safe_point.state == BEEB_RUNTIME_STATE_FAULTED);
    checkCStatus(beeb_start(machine), BEEB_STATUS_INVALID_STATE);

    beeb_cpu_state cpu{};
    checkCStatus(beeb_get_cpu_state(machine, &cpu), BEEB_STATUS_OK);
    checkCStatus(beeb_reset(machine), BEEB_STATUS_OK);
    checkCStatus(beeb_get_runtime_state(machine, &state), BEEB_STATUS_OK);
    CHECK(state == BEEB_RUNTIME_STATE_PAUSED);
    checkCStatus(beeb_get_fault(machine, &fault), BEEB_STATUS_OK);
    CHECK_EQ(fault.available, 0);
    CHECK(fault.message[0] == '\0');
    checkCStatus(beeb_destroy(machine), BEEB_STATUS_OK);
}

void testCAPI02DestroyWaitsForCallsAlreadyInside() {
    auto* machine = createCMachine();
    std::array<std::uint8_t, 0x4000> os{};
    os.fill(0xEA);
    os[0] = 0x4C;
    os[1] = 0x00;
    os[2] = 0xC0;
    os[0x3FFC] = 0x00;
    os[0x3FFD] = 0xC0;
    checkCStatus(beeb_load_os_rom(machine, os.data(), os.size()), BEEB_STATUS_OK);
    checkCStatus(beeb_reset(machine), BEEB_STATUS_OK);

    std::latch admitted(1);
    std::latch release(1);
    auto heldCall = std::async(std::launch::async, [&] {
        return beeb_test_hold_admitted_call(machine, admitted, release);
    });
    admitted.wait();
    auto destroy = std::async(std::launch::async, [&] { return beeb_destroy(machine); });
    beeb_runtime_state unavailableState = BEEB_RUNTIME_STATE_FAULTED;
    beeb_status unavailable{};
    const auto unavailableDeadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    do {
        unavailableState = BEEB_RUNTIME_STATE_FAULTED;
        unavailable = beeb_get_runtime_state(machine, &unavailableState);
        if (unavailable.code == BEEB_STATUS_UNAVAILABLE) break;
        std::this_thread::yield();
    } while (std::chrono::steady_clock::now() < unavailableDeadline);
    checkCStatus(unavailable, BEEB_STATUS_UNAVAILABLE);
    CHECK(unavailableState == BEEB_RUNTIME_STATE_FAULTED);
    release.count_down();
    const auto heldStatus = heldCall.get();
    const auto destroyed = destroy.get();
    checkCStatus(heldStatus, BEEB_STATUS_OK);
    checkCStatus(destroyed, BEEB_STATUS_OK);
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
    via.write(0xE, 0x82);          // enable CA1
    via.setCA1(false);             // default PCR selects falling edge
    CHECK(via.irq());
    (void)via.read(0x1);
    CHECK(!via.irq());
}

void testCRTCFrameTiming() {
    beeb::CRTC6845 crtc;
    crtc.reset();
    const auto set = [&](std::uint8_t reg, std::uint8_t value) {
        crtc.select(reg);
        crtc.write(value);
    };
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
    const auto set = [&](std::uint8_t reg, std::uint8_t value) {
        crtc.select(reg);
        crtc.write(value);
    };
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
    for (const auto sample : samples)
        nonZero = nonZero || sample != 0.0f;
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
    setCRTC(1, 1); // eight output pixels
    setCRTC(6, 1); // one character row
    setCRTC(9, 0); // one raster line
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
    for (unsigned byte = 0; byte < 256; ++byte)
        ssd[(1 * 10 + 2) * 256 + byte] = static_cast<std::uint8_t>(byte);
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
    crtc.select(1);
    crtc.write(2);
    crtc.select(6);
    crtc.write(1);
    crtc.select(12);
    crtc.write(0);
    crtc.select(13);
    crtc.write(0);
    std::array<std::uint8_t, 0x8000> ram{};
    ram[0x7C00] = 0x01; // alpha red
    ram[0x7C01] = 'A';
    beeb::TeletextRenderer renderer;
    const auto bitmap = renderer.render(ram, crtc, 0);
    CHECK_EQ(bitmap.width, 24);
    CHECK_EQ(bitmap.height, 20);
    unsigned redPixels = 0;
    for (std::size_t offset = 0; offset < bitmap.rgba.size(); offset += 4) {
        if (bitmap.rgba[offset] == 255 && bitmap.rgba[offset + 1] == 0 &&
            bitmap.rgba[offset + 2] == 0)
            ++redPixels;
    }
    CHECK(redPixels > 0);
}

void testTeletextControlCellsUseActiveBackground() {
    beeb::CRTC6845 crtc;
    crtc.reset();
    crtc.select(1);
    crtc.write(4);
    crtc.select(6);
    crtc.write(1);
    crtc.select(12);
    crtc.write(0);
    crtc.select(13);
    crtc.write(0);
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

std::array<std::uint8_t, 0x4000> makeLoopingOSROM() {
    auto os = makeNOPOSROM();
    os[0] = 0x4C;
    os[1] = 0x00;
    os[2] = 0xC0;
    return os;
}

std::array<std::uint8_t, 0x4000> makeOutputOSROM() {
    auto os = makeNOPOSROM();
    std::size_t cursor = 0;
    const auto emit = [&](std::uint8_t byte) { os[cursor++] = byte; };
    const auto loadImmediate = [&](std::uint8_t value) {
        emit(0xA9);
        emit(value);
    };
    const auto storeAbsolute = [&](std::uint16_t address) {
        emit(0x8D);
        emit(static_cast<std::uint8_t>(address));
        emit(static_cast<std::uint8_t>(address >> 8));
    };
    const auto setCRTC = [&](std::uint8_t reg, std::uint8_t value) {
        loadImmediate(reg);
        storeAbsolute(0xFE00);
        loadImmediate(value);
        storeAbsolute(0xFE01);
    };
    setCRTC(1, 1);
    setCRTC(6, 1);
    setCRTC(9, 0);
    setCRTC(12, 0);
    setCRTC(13, 0);
    loadImmediate(0x1C);
    storeAbsolute(0xFE20);
    const auto idle = static_cast<std::uint16_t>(0xC000 + cursor);
    emit(0x4C);
    emit(static_cast<std::uint8_t>(idle));
    emit(static_cast<std::uint8_t>(idle >> 8));
    return os;
}

std::array<std::uint8_t, 0x4000> makeLateFaultOSROM() {
    auto os = makeNOPOSROM();
    const std::array<std::uint8_t, 11> program{
        0xA9, 0x2A,       // LDA #$2A
        0x8D, 0x00, 0x00, // STA $0000
        0xA9, 0x0F,       // LDA #$0F
        0x8D, 0x00, 0xFE, // STA $FE00 (CRTC register select)
        0x02,             // unsupported opcode after RAM and device mutation
    };
    std::copy(program.begin(), program.end(), os.begin());
    return os;
}

void checkRuntimeOK(const beeb::RuntimeStatus& status) {
    CHECK(status.code == beeb::RuntimeStatusCode::ok);
    CHECK(status.message.empty());
}

template <typename T> const T& runtimeValue(const beeb::RuntimeResult<T>& result) {
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

/// Status/output tuple used by the exhaustive lifecycle matrix independent of
/// each command's concrete C++ result type.
struct C1MatrixObservation {
    beeb::RuntimeStatus status;
    bool outputPresent = false;
};

C1MatrixObservation invokeC1MatrixCommand(beeb::MachineRuntime& runtime,
                                          beeb::RuntimeCommandKind command) {
    const auto result = [](const auto& value) {
        return C1MatrixObservation{value.status, value.value.has_value()};
    };
    const auto outputResult = [](const auto& output, bool present) {
        beeb::RuntimeStatusCode code = beeb::RuntimeStatusCode::internalFailure;
        switch (output.status.code) {
        case beeb::OutputStatusCode::ok:
        case beeb::OutputStatusCode::empty:
        case beeb::OutputStatusCode::underrun:
        case beeb::OutputStatusCode::overrun:
            code = beeb::RuntimeStatusCode::ok;
            break;
        case beeb::OutputStatusCode::capacityExceeded:
        case beeb::OutputStatusCode::resourceExhausted:
            code = beeb::RuntimeStatusCode::resourceExhausted;
            break;
        case beeb::OutputStatusCode::invalidArgument:
            code = beeb::RuntimeStatusCode::invalidArgument;
            break;
        case beeb::OutputStatusCode::invalidState:
            code = beeb::RuntimeStatusCode::invalidState;
            break;
        case beeb::OutputStatusCode::unavailable:
            code = beeb::RuntimeStatusCode::unavailable;
            break;
        case beeb::OutputStatusCode::productionFailed:
            code = beeb::RuntimeStatusCode::executionFailed;
            break;
        case beeb::OutputStatusCode::internalFailure:
            break;
        }
        return C1MatrixObservation{{code, output.status.message, 0}, present};
    };
    switch (command) {
    case beeb::RuntimeCommandKind::start:
        return {runtime.start(), false};
    case beeb::RuntimeCommandKind::pause:
        return {runtime.pause(), false};
    case beeb::RuntimeCommandKind::reset:
        return {runtime.reset(), false};
    case beeb::RuntimeCommandKind::runCycles:
        return result(runtime.runFor(0));
    case beeb::RuntimeCommandKind::runUntilFrame:
        return result(runtime.runUntilFrame(0));
    case beeb::RuntimeCommandKind::loadOSROM:
        return {runtime.loadOSROM(makeLoopingOSROM()), false};
    case beeb::RuntimeCommandKind::loadSidewaysROM: {
        const std::array<std::uint8_t, 1> rom{0x42};
        return {runtime.loadSidewaysROM(0, rom), false};
    }
    case beeb::RuntimeCommandKind::mountDisc: {
        const std::vector<std::uint8_t> disc(40 * 10 * 256, 0);
        return {runtime.mountDisc(0, disc, beeb::DiscImage::Layout::SSD), false};
    }
    case beeb::RuntimeCommandKind::setKey:
        return {runtime.setKey(0, 0, true), false};
    case beeb::RuntimeCommandKind::setBreak:
        return {runtime.setBreak(false), false};
    case beeb::RuntimeCommandKind::runtimeState:
        return result(runtime.state());
    case beeb::RuntimeCommandKind::profile:
        return result(runtime.profile());
    case beeb::RuntimeCommandKind::safePoint:
        return result(runtime.safePoint());
    case beeb::RuntimeCommandKind::fault:
        return result(runtime.fault());
    case beeb::RuntimeCommandKind::cpuState:
        return result(runtime.cpuState());
    case beeb::RuntimeCommandKind::frame:
        return result(runtime.frame());
    case beeb::RuntimeCommandKind::renderAudio:
        return result(runtime.renderAudio(1, 48'000));
    case beeb::RuntimeCommandKind::dequeueFrame: {
        const auto output = runtime.dequeueFrame();
        return outputResult(output, output.frame.has_value());
    }
    case beeb::RuntimeCommandKind::drainAudio: {
        const auto output = runtime.drainAudio(0);
        return outputResult(output, true);
    }
    case beeb::RuntimeCommandKind::outputDiagnostics:
        return result(runtime.outputDiagnostics());
    case beeb::RuntimeCommandKind::shutdown:
        return {runtime.shutdown(), false};
    }
    throw TestFailure("unhandled C1 matrix command");
}

void testC1RuntimeCompleteCommandMatrix() {
    using Kind = beeb::RuntimeCommandKind;
    using Code = beeb::RuntimeStatusCode;
    using State = beeb::RuntimeState;
    constexpr std::array commands{Kind::start,           Kind::pause,         Kind::reset,
                                  Kind::runCycles,       Kind::runUntilFrame, Kind::loadOSROM,
                                  Kind::loadSidewaysROM, Kind::mountDisc,     Kind::setKey,
                                  Kind::setBreak,        Kind::runtimeState,  Kind::profile,
                                  Kind::safePoint,       Kind::fault,         Kind::cpuState,
                                  Kind::frame,           Kind::renderAudio,   Kind::shutdown};
    constexpr std::array states{State::paused, State::running, State::faulted, State::shuttingDown};
    const auto isQuery = [](Kind command) {
        return command == Kind::runCycles || command == Kind::runUntilFrame ||
               command == Kind::runtimeState || command == Kind::profile ||
               command == Kind::safePoint || command == Kind::fault || command == Kind::cpuState ||
               command == Kind::frame || command == Kind::renderAudio;
    };
    const auto expectedCode = [](State state, Kind command) {
        if (state == State::shuttingDown)
            return command == Kind::shutdown ? Code::ok : Code::unavailable;
        if (state == State::faulted) {
            switch (command) {
            case Kind::reset:
            case Kind::runtimeState:
            case Kind::profile:
            case Kind::safePoint:
            case Kind::fault:
            case Kind::cpuState:
            case Kind::frame:
            case Kind::shutdown:
                return Code::ok;
            default:
                return Code::invalidState;
            }
        }
        if (state == State::running &&
            (command == Kind::runCycles || command == Kind::runUntilFrame))
            return Code::invalidState;
        return Code::ok;
    };

    for (const auto initial : states) {
        for (const auto command : commands) {
            beeb::MachineRuntime runtime({.enableLedger = true});
            checkRuntimeOK(runtime.loadOSROM(initial == State::faulted ? makeLateFaultOSROM()
                                                                       : makeLoopingOSROM()));
            checkRuntimeOK(runtime.reset());
            if (initial == State::running) checkRuntimeOK(runtime.start());
            if (initial == State::faulted) {
                CHECK(runtime.runFor(100).status.code == Code::executionFailed);
            }
            if (initial == State::shuttingDown) checkRuntimeOK(runtime.shutdown());

            const auto observation = invokeC1MatrixCommand(runtime, command);
            const auto expected = expectedCode(initial, command);
            CHECK(observation.status.code == expected);
            CHECK_EQ(observation.outputPresent, expected == Code::ok && isQuery(command));
            if (initial == State::shuttingDown && command != Kind::shutdown) {
                CHECK_EQ(observation.status.acceptanceSequence, 0);
                continue;
            }
            if (initial == State::shuttingDown && command == Kind::shutdown)
                CHECK_EQ(observation.status.acceptanceSequence, 0);
            else
                CHECK(observation.status.acceptanceSequence != 0);
            const auto ledger = runtime.ledger();
            const auto entry = std::find_if(ledger.rbegin(), ledger.rend(), [&](const auto& item) {
                return item.event == beeb::LedgerEventKind::command && item.command == command;
            });
            CHECK(entry != ledger.rend());
            CHECK(entry->status == expected);

            if (command == Kind::shutdown) continue;
            const auto state = runtimeValue(runtime.state());
            if (expected != Code::ok) {
                CHECK(state == initial);
            } else if (command == Kind::start) {
                CHECK(state == State::running);
            } else if (command == Kind::pause || command == Kind::reset) {
                CHECK(state == State::paused);
            } else {
                CHECK(state == initial);
            }
        }
    }
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

void testC1LateFaultRetainsLastCompletedBoundary() {
    beeb::BBCMicro expected;
    CHECK(expected.loadOSROM(makeLateFaultOSROM()));
    expected.reset();
    const auto expectedStartCycles = expected.cpu().state().cycles;
    try {
        (void)expected.runFor(100);
        CHECK(false);
    } catch (const std::runtime_error&) {
    }
    const auto expectedCPU = expected.cpu().state();
    const auto expectedDigest = beeb::BBCMicroTestAccess::digest(expected);
    const auto expectedRetainedCycles = expectedCPU.cycles - expectedStartCycles;
    CHECK(expectedRetainedCycles > 0);

    const auto exercisePaused = [&](bool untilFrame) {
        beeb::MachineRuntime runtime({.enableLedger = true});
        checkRuntimeOK(runtime.loadOSROM(makeLateFaultOSROM()));
        checkRuntimeOK(runtime.reset());
        const auto result =
            untilFrame ? runtime.runUntilFrame(100).status : runtime.runFor(100).status;
        CHECK(result.code == beeb::RuntimeStatusCode::executionFailed);
        CHECK(runtimeValue(runtime.cpuState()) == expectedCPU);
        const auto fault = runtimeValue(runtime.fault());
        CHECK(fault.available);
        CHECK_EQ(fault.safePoint.cpuCycles, expectedCPU.cycles);
        CHECK(fault.safePoint.state == beeb::RuntimeState::faulted);
        (void)runtimeValue(runtime.safePoint());
        const auto ledger = runtime.ledger();
        CHECK(!ledger.empty());
        const auto failed = std::find_if(ledger.rbegin(), ledger.rend(), [](const auto& entry) {
            return entry.status == beeb::RuntimeStatusCode::executionFailed;
        });
        CHECK(failed != ledger.rend());
        CHECK_EQ(failed->actualCycles, expectedRetainedCycles);
        CHECK_EQ(ledger.back().resultDigest, expectedDigest);
        checkRuntimeOK(runtime.reset());
        CHECK(runtimeValue(runtime.state()) == beeb::RuntimeState::paused);
    };

    exercisePaused(false);
    exercisePaused(true);

    beeb::MachineRuntime sustained({.enableLedger = true});
    checkRuntimeOK(sustained.loadOSROM(makeLateFaultOSROM()));
    checkRuntimeOK(sustained.reset());
    checkRuntimeOK(sustained.start());
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    while (runtimeValue(sustained.state()) != beeb::RuntimeState::faulted &&
           std::chrono::steady_clock::now() < deadline)
        std::this_thread::yield();
    CHECK(runtimeValue(sustained.state()) == beeb::RuntimeState::faulted);
    CHECK(runtimeValue(sustained.cpuState()) == expectedCPU);
    (void)runtimeValue(sustained.safePoint());
    const auto ledger = sustained.ledger();
    const auto failedSlice = std::find_if(ledger.rbegin(), ledger.rend(), [](const auto& entry) {
        return entry.event == beeb::LedgerEventKind::executionSlice &&
               entry.status == beeb::RuntimeStatusCode::executionFailed;
    });
    CHECK(failedSlice != ledger.rend());
    CHECK_EQ(failedSlice->actualCycles, expectedRetainedCycles);
    CHECK_EQ(ledger.back().resultDigest, expectedDigest);
    checkRuntimeOK(sustained.reset());
}

void testC1ClosedLoopSustainedLifecycleRepeats() {
    for (unsigned repetition = 0; repetition < 50; ++repetition) {
        beeb::MachineRuntime runtime({.enableLedger = true});
        checkRuntimeOK(runtime.loadOSROM(makeLoopingOSROM()));
        checkRuntimeOK(runtime.reset());
        checkRuntimeOK(runtime.start());
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
        bool observedExecutionSlice = false;
        while (!observedExecutionSlice && std::chrono::steady_clock::now() < deadline) {
            const auto runningLedger = runtime.ledger();
            observedExecutionSlice =
                std::any_of(runningLedger.begin(), runningLedger.end(), [](const auto& entry) {
                    return entry.event == beeb::LedgerEventKind::executionSlice;
                });
            std::this_thread::yield();
        }
        CHECK(observedExecutionSlice);
        checkRuntimeOK(runtime.pause());
        CHECK(runtimeValue(runtime.state()) == beeb::RuntimeState::paused);
        CHECK(!runtimeValue(runtime.fault()).available);
        const auto ledger = runtime.ledger();
        CHECK(std::any_of(ledger.begin(), ledger.end(), [](const auto& entry) {
            return entry.event == beeb::LedgerEventKind::executionSlice;
        }));
    }
}

void testC1MachineDigestCoversRollbackState() {
    beeb::BBCMicro machine;
    const auto initial = beeb::BBCMicroTestAccess::digest(machine);
    auto checkpoint = machine.checkpoint();
    const std::array<std::uint8_t, 1> byte{0x2A};
    CHECK(machine.loadRAM(0, byte));
    machine.crtc().select(15);
    machine.cpu().setIRQ(true);
    machine.cpu().requestNMI();
    CHECK(beeb::BBCMicroTestAccess::digest(machine) != initial);
    machine.restore(std::move(checkpoint));
    CHECK_EQ(beeb::BBCMicroTestAccess::digest(machine), initial);

    machine.systemVIA().write(4, 4);
    machine.systemVIA().write(5, 0);
    const auto timerInitial = beeb::BBCMicroTestAccess::digest(machine);
    checkpoint = machine.checkpoint();
    machine.systemVIA().tick(1);
    CHECK(beeb::BBCMicroTestAccess::digest(machine) != timerInitial);
    machine.restore(std::move(checkpoint));
    CHECK_EQ(beeb::BBCMicroTestAccess::digest(machine), timerInitial);
    machine.reset();
    CHECK_EQ(beeb::BBCMicroTestAccess::digest(machine), initial);

    checkpoint = machine.checkpoint();
    float sample = 0;
    machine.sound().render(&sample, 1, 48'000);
    CHECK(beeb::BBCMicroTestAccess::digest(machine) != initial);
    machine.restore(std::move(checkpoint));
    CHECK_EQ(beeb::BBCMicroTestAccess::digest(machine), initial);

    checkpoint = machine.checkpoint();
    machine.discController().write(0, 0x13);
    machine.discController().tick(1);
    CHECK(beeb::BBCMicroTestAccess::digest(machine) != initial);
    machine.restore(std::move(checkpoint));
    CHECK_EQ(beeb::BBCMicroTestAccess::digest(machine), initial);
}

void testC1AllocationFailuresRemainRecoverable() {
    using Point = beeb::RuntimeAllocationFailurePoint;
    using Code = beeb::RuntimeStatusCode;
    const auto runtimeWithFailure = [](Point point) {
        return std::make_unique<beeb::MachineRuntime>(
            beeb::MachineRuntimeOptions{.enableLedger = true, .failAllocationAt = point});
    };

    for (const auto point : {Point::request, Point::queue, Point::ledger}) {
        auto runtime = runtimeWithFailure(point);
        const auto failed = runtime->state();
        CHECK(failed.status.code == Code::resourceExhausted);
        CHECK(!failed.value.has_value());
        CHECK(runtimeValue(runtime->state()) == beeb::RuntimeState::paused);
    }

    {
        auto runtime = runtimeWithFailure(Point::frame);
        const auto failed = runtime->frame();
        CHECK(failed.status.code == Code::resourceExhausted);
        CHECK(!failed.value.has_value());
        CHECK(runtime->cpuState().status.code == Code::ok);
    }
    {
        auto runtime = runtimeWithFailure(Point::audio);
        const auto failed = runtime->renderAudio(1, 48'000);
        CHECK(failed.status.code == Code::resourceExhausted);
        CHECK(!failed.value.has_value());
        CHECK(runtime->cpuState().status.code == Code::ok);
        const auto oversized = runtime->renderAudio(std::vector<float>{}.max_size(), 48'000);
        CHECK(oversized.status.code == Code::invalidArgument);
        CHECK(!oversized.value.has_value());
    }
    {
        auto runtime = runtimeWithFailure(Point::faultResult);
        const auto failed = runtime->fault();
        CHECK(failed.status.code == Code::resourceExhausted);
        CHECK(!failed.value.has_value());
        CHECK(runtimeValue(runtime->state()) == beeb::RuntimeState::paused);
    }
    {
        auto runtime = runtimeWithFailure(Point::shutdownResult);
        CHECK(runtime->shutdown().code == Code::resourceExhausted);
        CHECK(runtimeValue(runtime->state()) == beeb::RuntimeState::paused);
        checkRuntimeOK(runtime->shutdown());
    }
    {
        auto runtime = runtimeWithFailure(Point::boundedExecution);
        checkRuntimeOK(runtime->loadOSROM(makeLoopingOSROM()));
        checkRuntimeOK(runtime->reset());
        const auto before = runtimeValue(runtime->cpuState());
        const auto failed = runtime->runFor(10);
        CHECK(failed.status.code == Code::resourceExhausted);
        CHECK(!failed.value.has_value());
        CHECK(runtimeValue(runtime->cpuState()) == before);
        CHECK(runtimeValue(runtime->state()) == beeb::RuntimeState::paused);
    }
    {
        auto runtime = runtimeWithFailure(Point::sustainedExecution);
        checkRuntimeOK(runtime->loadOSROM(makeLoopingOSROM()));
        checkRuntimeOK(runtime->reset());
        checkRuntimeOK(runtime->start());
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
        while (runtimeValue(runtime->state()) != beeb::RuntimeState::faulted &&
               std::chrono::steady_clock::now() < deadline)
            std::this_thread::yield();
        CHECK(runtimeValue(runtime->state()) == beeb::RuntimeState::faulted);
        const auto ledger = runtime->ledger();
        CHECK(std::any_of(ledger.begin(), ledger.end(), [](const auto& entry) {
            return entry.event == beeb::LedgerEventKind::executionSlice &&
                   entry.status == beeb::RuntimeStatusCode::resourceExhausted;
        }));
        checkRuntimeOK(runtime->reset());
        CHECK(runtime->cpuState().status.code == Code::ok);
    }
}

void testC1OwnerReentrantSubmissionIsRejected() {
    beeb::MachineRuntime runtime({.enableLedger = true, .testReentrantSubmission = true});
    const auto rejected = runtime.state();
    CHECK(rejected.status.code == beeb::RuntimeStatusCode::reentrantCall);
    CHECK(!rejected.value.has_value());
    CHECK(rejected.status.acceptanceSequence != 0);
    CHECK(runtimeValue(runtime.state()) == beeb::RuntimeState::paused);
}

/// Owned signature from one deterministic command scenario: CPU, safe point,
/// exact ledger, and whole-machine digest are compared across fresh runtimes.
struct C1ReplaySignature {
    beeb::CPUState cpu;
    beeb::SafePoint safePoint;
    std::vector<beeb::LedgerEntry> ledger;
    std::uint64_t machineDigest = 0;
};

C1ReplaySignature runC1ReplayScenario() {
    beeb::MachineRuntime runtime({.enableLedger = true});
    loadNOPFixture(runtime);
    CHECK(runtimeValue(runtime.runFor(17)) >= 17);
    checkRuntimeOK(runtime.setKey(1, 2, true));
    CHECK(runtimeValue(runtime.runFor(31)) >= 31);
    checkRuntimeOK(runtime.setKey(1, 2, false));
    const auto cpu = runtimeValue(runtime.cpuState());
    const auto safePoint = runtimeValue(runtime.safePoint());
    const auto ledger = runtime.ledger();
    CHECK(!ledger.empty());
    return {cpu, safePoint, ledger, ledger.back().resultDigest};
}

void testC1ReplayDeterministicLedger() {
    const auto expected = runC1ReplayScenario();
    CHECK(!expected.ledger.empty());
    for (unsigned repetition = 1; repetition < 10; ++repetition) {
        const auto actual = runC1ReplayScenario();
        CHECK(actual.cpu == expected.cpu);
        CHECK(actual.safePoint == expected.safePoint);
        CHECK(actual.ledger == expected.ledger);
        CHECK_EQ(actual.machineDigest, expected.machineDigest);
    }
}

/// Owned capture after concurrent command admission; its accepted ledger drives
/// sequential replay while CPU, safe point, and machine digest define the target.
struct C1CapturedReplay {
    beeb::CPUState cpu;
    beeb::SafePoint safePoint;
    std::vector<beeb::LedgerEntry> ledger;
    std::uint64_t machineDigest = 0;
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
            case 0:
                return runtime.start();
            case 1:
                return runtime.pause();
            default:
                return runtime.state().status;
            }
        }));
    }
    ready.wait();
    release.count_down();
    for (auto& call : calls)
        checkRuntimeOK(call.get());
    checkRuntimeOK(runtime.pause());

    const auto cpu = runtimeValue(runtime.cpuState());
    const auto safePoint = runtimeValue(runtime.safePoint());
    const auto ledger = runtime.ledger();
    CHECK(!ledger.empty());
    return {cpu, safePoint, ledger, ledger.back().resultDigest};
}

/// Final CPU, safe-point, and machine-digest outcome from one sequential replay.
struct C1ReplayOutcome {
    beeb::CPUState cpu;
    beeb::SafePoint safePoint;
    std::uint64_t machineDigest = 0;

    friend bool operator==(const C1ReplayOutcome&, const C1ReplayOutcome&) = default;
};

C1ReplayOutcome replayC1Ledger(const std::vector<beeb::LedgerEntry>& ledger) {
    beeb::BBCMicro machine;
    auto state = beeb::RuntimeState::paused;
    auto previousSequence = std::uint64_t{0};
    auto previousAcceptance = std::uint64_t{0};
    beeb::SafePoint finalSafePoint{};
    const auto os = makeNOPOSROM();
    std::uint64_t audioRemainder = 0;
    const auto publishAudio = [&](std::uint64_t cycles) {
        const auto fractional = audioRemainder + (cycles % 125) * 3;
        std::uint64_t remaining = (cycles / 125) * 3 + fractional / 125;
        audioRemainder = fractional % 125;
        std::array<float, beeb::audioSampleCapacity> samples{};
        while (remaining != 0) {
            const auto count =
                static_cast<std::size_t>(std::min<std::uint64_t>(remaining, samples.size()));
            machine.sound().render(samples.data(), count,
                                   static_cast<double>(beeb::audioSampleRate));
            remaining -= count;
        }
    };

    for (const auto& entry : ledger) {
        CHECK(entry.sequence > previousSequence);
        previousSequence = entry.sequence;
        CHECK(entry.safePoint.ledgerSequence == entry.sequence);
        CHECK(entry.status == beeb::RuntimeStatusCode::ok);

        if (entry.event == beeb::LedgerEventKind::executionSlice) {
            CHECK(state == beeb::RuntimeState::running);
            CHECK_EQ(entry.requestedCycles, beeb::MachineRuntime::executionSliceCycles);
            CHECK_EQ(machine.runFor(entry.requestedCycles), entry.actualCycles);
            publishAudio(entry.actualCycles);
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
    return {machine.cpu().state(), finalSafePoint, beeb::BBCMicroTestAccess::digest(machine)};
}

void writeC1ReplayEvidence(const C1CapturedReplay& capture) {
    const auto* directory = std::getenv("BEEB_C1_EVIDENCE_DIR");
    if (!directory || *directory == '\0') return;
    std::filesystem::create_directories(directory);
    std::ofstream output(std::filesystem::path(directory) / "accepted-ledger.txt");
    CHECK(output.good());
    output << "sequence event command acceptance requested actual payload result status "
              "cpu_cycles frame state\n";
    output << "# safe_point_machine_digest=" << capture.machineDigest << '\n';
    for (const auto& entry : capture.ledger) {
        output << entry.sequence << ' ' << static_cast<unsigned>(entry.event) << ' '
               << static_cast<unsigned>(entry.command) << ' ' << entry.acceptanceSequence << ' '
               << entry.requestedCycles << ' ' << entry.actualCycles << ' ' << entry.payloadDigest
               << ' ' << entry.resultDigest << ' ' << static_cast<unsigned>(entry.status) << ' '
               << entry.safePoint.cpuCycles << ' ' << entry.safePoint.frameNumber << ' '
               << static_cast<unsigned>(entry.safePoint.state) << '\n';
    }
}

void testC1ReplayCapturedConcurrentLedgerExactly() {
    const auto capture = captureC1ConcurrentLedger();
    CHECK(!capture.ledger.empty());
    writeC1ReplayEvidence(capture);

    const C1ReplayOutcome expected{capture.cpu, capture.safePoint, capture.machineDigest};
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
            }))
            break;
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
    const auto lastSequenceAtAcceptance = atAcceptance.empty() ? 0 : atAcceptance.back().sequence;
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

void waitForC1Acceptance(beeb::MachineRuntime& runtime, std::uint64_t expected) {
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    while (runtime.acceptedCommandCount() < expected &&
           std::chrono::steady_clock::now() < deadline) {
        std::this_thread::yield();
    }
    CHECK_EQ(runtime.acceptedCommandCount(), expected);
}

void testC1TransactionsFIFOAndNoAutoResume() {
    beeb::MachineRuntime runtime({.enableLedger = true});
    const auto os = makeNOPOSROM();
    loadNOPFixture(runtime);
    checkRuntimeOK(runtime.start());

    auto nextAcceptance = runtime.acceptedCommandCount() + 1;
    auto reset = std::async(std::launch::async, [&runtime] { return runtime.reset(); });
    waitForC1Acceptance(runtime, nextAcceptance++);
    auto load = std::async(std::launch::async, [&runtime, &os] { return runtime.loadOSROM(os); });
    waitForC1Acceptance(runtime, nextAcceptance++);
    auto input = std::async(std::launch::async, [&runtime] { return runtime.setKey(1, 2, true); });
    waitForC1Acceptance(runtime, nextAcceptance++);
    auto stateBetween = std::async(std::launch::async, [&runtime] { return runtime.state(); });
    waitForC1Acceptance(runtime, nextAcceptance++);
    auto start = std::async(std::launch::async, [&runtime] { return runtime.start(); });
    waitForC1Acceptance(runtime, nextAcceptance);

    const auto resetStatus = reset.get();
    const auto loadStatus = load.get();
    const auto inputStatus = input.get();
    const auto betweenResult = stateBetween.get();
    const auto startStatus = start.get();
    checkRuntimeOK(resetStatus);
    checkRuntimeOK(loadStatus);
    checkRuntimeOK(inputStatus);
    CHECK(runtimeValue(betweenResult) == beeb::RuntimeState::paused);
    checkRuntimeOK(startStatus);
    CHECK(runtimeValue(runtime.state()) == beeb::RuntimeState::running);
    checkRuntimeOK(runtime.pause());

    const auto ledger = runtime.ledger();
    const std::array acceptanceOrder{
        resetStatus.acceptanceSequence, loadStatus.acceptanceSequence,
        inputStatus.acceptanceSequence, betweenResult.status.acceptanceSequence,
        startStatus.acceptanceSequence,
    };
    std::vector<beeb::RuntimeCommandKind> acceptedCommands;
    for (const auto& entry : ledger) {
        if (entry.event != beeb::LedgerEventKind::command) continue;
        if (std::find(acceptanceOrder.begin(), acceptanceOrder.end(), entry.acceptanceSequence) !=
            acceptanceOrder.end()) {
            acceptedCommands.push_back(entry.command);
        }
    }
    const std::vector expectedCommands{
        beeb::RuntimeCommandKind::reset,  beeb::RuntimeCommandKind::loadOSROM,
        beeb::RuntimeCommandKind::setKey, beeb::RuntimeCommandKind::runtimeState,
        beeb::RuntimeCommandKind::start,
    };
    CHECK(acceptedCommands == expectedCommands);
}

void testC1TransactionsRejectAtomicallyAndCopyInput() {
    beeb::MachineRuntime runtime;
    auto source = makeNOPOSROM();
    checkRuntimeOK(runtime.loadOSROM(source));
    checkRuntimeOK(runtime.reset());

    std::vector<std::uint8_t> invalid(source.begin(), source.end() - 1);
    invalid[0] = 0x02;
    const auto rejected = runtime.loadOSROM(invalid);
    CHECK(rejected.code == beeb::RuntimeStatusCode::invalidArgument);
    checkRuntimeOK(runtime.reset());
    CHECK(runtimeValue(runtime.runFor(2)) >= 2);

    const auto beforeCopy = runtime.acceptedCommandCount();
    auto copiedLoad =
        std::async(std::launch::async, [&runtime, &source] { return runtime.loadOSROM(source); });
    waitForC1Acceptance(runtime, beforeCopy + 1);
    source[0] = 0x02;
    checkRuntimeOK(copiedLoad.get());
    checkRuntimeOK(runtime.reset());
    CHECK(runtimeValue(runtime.runFor(2)) >= 2);

    checkRuntimeOK(runtime.start());
    CHECK(runtime.setKey(16, 0, true).code == beeb::RuntimeStatusCode::invalidArgument);
    CHECK(runtimeValue(runtime.state()) == beeb::RuntimeState::running);
    checkRuntimeOK(runtime.pause());
}

std::uint64_t c1PayloadDigest(const beeb::MachineRuntime& runtime,
                              std::uint64_t acceptanceSequence) {
    const auto ledger = runtime.ledger();
    const auto entry = std::find_if(ledger.begin(), ledger.end(), [&](const auto& candidate) {
        return candidate.event == beeb::LedgerEventKind::command &&
               candidate.acceptanceSequence == acceptanceSequence;
    });
    CHECK(entry != ledger.end());
    return entry->payloadDigest;
}

void testC1MediaTransactionsCopyAndRejectAtomically() {
    beeb::MachineRuntime runtime({.enableLedger = true});
    auto os = makeNOPOSROM();
    os[0] = 0xA9;
    os[1] = 0x03; // LDA #3
    os[2] = 0x8D;
    os[3] = 0x30;
    os[4] = 0xFE; // STA $FE30
    os[5] = 0xAD;
    os[6] = 0x00;
    os[7] = 0x80; // LDA $8000
    checkRuntimeOK(runtime.loadOSROM(os));

    std::vector<std::uint8_t> sideways(0x4000, 0x42);
    const auto sidewaysAcceptance = runtime.acceptedCommandCount() + 1;
    auto sidewaysLoad =
        std::async(std::launch::async, [&] { return runtime.loadSidewaysROM(3, sideways); });
    waitForC1Acceptance(runtime, sidewaysAcceptance);
    sideways[0] = 0x99;
    const auto sidewaysStatus = sidewaysLoad.get();
    checkRuntimeOK(sidewaysStatus);

    std::vector<std::uint8_t> oversizedSideways(0x4001, 0x11);
    CHECK(runtime.loadSidewaysROM(3, oversizedSideways).code ==
          beeb::RuntimeStatusCode::invalidArgument);
    checkRuntimeOK(runtime.reset());
    CHECK(runtimeValue(runtime.runFor(10)) >= 10);
    CHECK_EQ(runtimeValue(runtime.cpuState()).a, 0x42);

    std::vector<std::uint8_t> expectedDisc(40 * 10 * 256, 0);
    expectedDisc[0] = 0xA7;
    auto disc = expectedDisc;
    const auto firstDiscAcceptance = runtime.acceptedCommandCount() + 1;
    auto discMount = std::async(std::launch::async, [&] {
        return runtime.mountDisc(0, disc, beeb::DiscImage::Layout::SSD);
    });
    waitForC1Acceptance(runtime, firstDiscAcceptance);
    disc[0] = 0xB8;
    const auto firstDiscStatus = discMount.get();
    checkRuntimeOK(firstDiscStatus);

    const std::array<std::uint8_t, 1> invalidDisc{0};
    CHECK(runtime.mountDisc(0, invalidDisc, beeb::DiscImage::Layout::SSD).code ==
          beeb::RuntimeStatusCode::invalidArgument);
    const auto secondDiscStatus = runtime.mountDisc(0, expectedDisc, beeb::DiscImage::Layout::SSD);
    checkRuntimeOK(secondDiscStatus);
    CHECK_EQ(c1PayloadDigest(runtime, firstDiscStatus.acceptanceSequence),
             c1PayloadDigest(runtime, secondDiscStatus.acceptanceSequence));
}

void testC1InputAndBreakSerializeWithReset() {
    beeb::MachineRuntime runtime({.enableLedger = true});
    auto os = makeNOPOSROM();
    os[0] = 0xA9;
    os[1] = 0x7F; // LDA #$7F
    os[2] = 0x8D;
    os[3] = 0x43;
    os[4] = 0xFE; // STA $FE43 (VIA DDRA)
    os[5] = 0xA9;
    os[6] = 0x21; // column 1, row 2
    os[7] = 0x8D;
    os[8] = 0x41;
    os[9] = 0xFE; // STA $FE41 (VIA ORA)
    os[10] = 0xAD;
    os[11] = 0x41;
    os[12] = 0xFE; // LDA $FE41
    checkRuntimeOK(runtime.loadOSROM(os));
    checkRuntimeOK(runtime.reset());

    const auto keyDown = runtime.setKey(1, 2, true);
    checkRuntimeOK(keyDown);
    CHECK(runtimeValue(runtime.runFor(16)) >= 16);
    CHECK_EQ(runtimeValue(runtime.cpuState()).a, 0xA1);

    const auto breakDown = runtime.setBreak(true);
    checkRuntimeOK(breakDown);
    const auto afterBreak = runtimeValue(runtime.cpuState());
    CHECK_EQ(afterBreak.pc, 0xC000);
    CHECK_EQ(afterBreak.cycles, 7);
    CHECK(runtimeValue(runtime.state()) == beeb::RuntimeState::paused);

    checkRuntimeOK(runtime.setBreak(false));
    checkRuntimeOK(runtime.setKey(1, 2, false));
    CHECK(runtimeValue(runtime.runFor(16)) >= 16);
    CHECK_EQ(runtimeValue(runtime.cpuState()).a, 0x21);

    const std::array expectedKinds{
        beeb::RuntimeCommandKind::setKey,
        beeb::RuntimeCommandKind::setBreak,
        beeb::RuntimeCommandKind::setBreak,
        beeb::RuntimeCommandKind::setKey,
    };
    std::vector<beeb::RuntimeCommandKind> observedKinds;
    for (const auto& entry : runtime.ledger()) {
        if (entry.event == beeb::LedgerEventKind::command &&
            (entry.command == beeb::RuntimeCommandKind::setKey ||
             entry.command == beeb::RuntimeCommandKind::setBreak)) {
            observedKinds.push_back(entry.command);
        }
    }
    CHECK_EQ(observedKinds.size(), expectedKinds.size());
    CHECK(std::equal(expectedKinds.begin(), expectedKinds.end(), observedKinds.begin()));
}

void testC1ObservationsReturnConsistentOwnedValues() {
    beeb::MachineRuntime runtime;
    auto os = makeNOPOSROM();
    std::size_t cursor = 0;
    const auto emit = [&](std::uint8_t byte) { os[cursor++] = byte; };
    const auto loadImmediate = [&](std::uint8_t value) {
        emit(0xA9);
        emit(value);
    };
    const auto storeAbsolute = [&](std::uint16_t address) {
        emit(0x8D);
        emit(static_cast<std::uint8_t>(address));
        emit(static_cast<std::uint8_t>(address >> 8));
    };
    const auto setCRTC = [&](std::uint8_t reg, std::uint8_t value) {
        loadImmediate(reg);
        storeAbsolute(0xFE00);
        loadImmediate(value);
        storeAbsolute(0xFE01);
    };
    setCRTC(1, 1);
    setCRTC(6, 1);
    setCRTC(9, 0);
    setCRTC(12, 0);
    setCRTC(13, 0);
    loadImmediate(0x1C);
    storeAbsolute(0xFE20);
    loadImmediate(0xA0);
    storeAbsolute(0x0000);
    const auto idle = static_cast<std::uint16_t>(0xC000 + cursor);
    emit(0x4C);
    emit(static_cast<std::uint8_t>(idle));
    emit(static_cast<std::uint8_t>(idle >> 8));

    checkRuntimeOK(runtime.loadOSROM(os));
    checkRuntimeOK(runtime.reset());
    CHECK(runtimeValue(runtime.runUntilFrame(10'000)));

    const auto cpuResult = runtime.cpuState();
    const auto frameResult = runtime.frame();
    const auto pointResult = runtime.safePoint();
    const auto cpu = runtimeValue(cpuResult);
    const auto firstFrame = runtimeValue(frameResult);
    const auto point = runtimeValue(pointResult);
    CHECK(firstFrame.available);
    CHECK_EQ(firstFrame.width, 8);
    CHECK_EQ(firstFrame.height, 1);
    CHECK_EQ(firstFrame.rgba.size(), 32);
    CHECK_EQ(cpu.cycles, point.cpuCycles);
    CHECK_EQ(firstFrame.number, point.frameNumber);

    CHECK(runtimeValue(runtime.runUntilFrame(10'000)));
    auto secondFrameResult = runtime.frame();
    auto secondFrame = runtimeValue(secondFrameResult);
    CHECK(firstFrame.rgba.data() != secondFrame.rgba.data());
    const auto firstPixel = firstFrame.rgba.front();
    secondFrame.rgba.front() ^= 0xFF;
    CHECK_EQ(firstFrame.rgba.front(), firstPixel);

    const auto firstAudioResult = runtime.renderAudio(32, 48'000.0);
    const auto secondAudioResult = runtime.renderAudio(32, 48'000.0);
    const auto firstAudio = runtimeValue(firstAudioResult);
    const auto secondAudio = runtimeValue(secondAudioResult);
    CHECK_EQ(firstAudio.size(), 32);
    CHECK_EQ(secondAudio.size(), 32);
    CHECK(firstAudio.data() != secondAudio.data());
    const auto invalidAudio = runtime.renderAudio(1, 0.0);
    CHECK(invalidAudio.status.code == beeb::RuntimeStatusCode::invalidArgument);
    CHECK(!invalidAudio.value.has_value());
}

void testC1RaceMixedCommands() {
    beeb::MachineRuntime runtime({.enableLedger = true});
    const auto loopingOS = makeLoopingOSROM();
    checkRuntimeOK(runtime.loadOSROM(loopingOS));
    checkRuntimeOK(runtime.reset());
    const std::vector<std::uint8_t> sideways(0x4000, 0x42);
    const std::vector<std::uint8_t> disc(40 * 10 * 256, 0);
    constexpr unsigned threadCount = 8;
    constexpr unsigned operationsPerThread = 1'250;
    std::atomic<unsigned> accounted{0};
    std::atomic<unsigned> accepted{0};
    std::atomic<unsigned> failed{0};
    std::vector<std::thread> workers;
    workers.reserve(threadCount);

    for (unsigned thread = 0; thread < threadCount; ++thread) {
        workers.emplace_back([&, thread] {
            for (unsigned operation = 0; operation < operationsPerThread; ++operation) {
                beeb::RuntimeStatus status;
                switch ((thread + operation) % 14) {
                case 0:
                    status = runtime.start();
                    break;
                case 1:
                    status = runtime.pause();
                    break;
                case 2:
                    status = runtime.reset();
                    break;
                case 3:
                    status = runtime.loadOSROM(loopingOS);
                    break;
                case 4:
                    status = runtime.loadSidewaysROM(3, sideways);
                    break;
                case 5:
                    status = runtime.mountDisc(0, disc, beeb::DiscImage::Layout::SSD);
                    break;
                case 6:
                    status = runtime.setKey(static_cast<std::uint8_t>(thread),
                                            static_cast<std::uint8_t>(operation % 8),
                                            (operation & 1) != 0);
                    break;
                case 7:
                    status = runtime.setBreak(false);
                    break;
                case 8:
                    status = runtime.state().status;
                    break;
                case 9:
                    status = runtime.cpuState().status;
                    break;
                case 10:
                    status = runtime.frame().status;
                    break;
                case 11:
                    status = runtime.renderAudio(1, 48'000).status;
                    break;
                case 12:
                    status = runtime.safePoint().status;
                    break;
                default:
                    status = runtime.fault().status;
                    break;
                }
                ++accounted;
                if (status.code == beeb::RuntimeStatusCode::ok && status.acceptanceSequence != 0)
                    ++accepted;
                else
                    ++failed;
            }
        });
    }
    for (auto& worker : workers)
        worker.join();

    CHECK_EQ(accounted.load(), threadCount * operationsPerThread);
    CHECK_EQ(accepted.load() + failed.load(), accounted.load());
    CHECK_EQ(failed.load(), 0);
    checkRuntimeOK(runtime.pause());
    CHECK(runtime.ledger().size() >= accepted.load());

    checkRuntimeOK(runtime.loadOSROM(makeLateFaultOSROM()));
    checkRuntimeOK(runtime.reset());
    constexpr unsigned recoveryObservers = 4;
    constexpr unsigned recoveryQueriesPerObserver = 100;
    std::latch executionFaulted(1);
    std::latch faultObserved(recoveryObservers);
    std::latch continueQueries(1);
    std::atomic<unsigned> recoveryFailures{0};
    std::vector<std::thread> recoveryWorkers;
    recoveryWorkers.reserve(recoveryObservers);
    for (unsigned observer = 0; observer < recoveryObservers; ++observer) {
        recoveryWorkers.emplace_back([&, observer] {
            executionFaulted.wait();
            const auto observed = runtime.fault();
            ++accounted;
            if (!observed.status.isOK() || !observed.value || !observed.value->available)
                ++recoveryFailures;
            faultObserved.count_down();
            continueQueries.wait();
            for (unsigned query = 0; query < recoveryQueriesPerObserver; ++query) {
                beeb::RuntimeStatus status;
                switch ((observer + query) % 4) {
                case 0:
                    status = runtime.state().status;
                    break;
                case 1:
                    status = runtime.cpuState().status;
                    break;
                case 2:
                    status = runtime.safePoint().status;
                    break;
                default:
                    status = runtime.fault().status;
                    break;
                }
                ++accounted;
                if (!status.isOK()) ++recoveryFailures;
            }
        });
    }
    const auto execution = runtime.runFor(100);
    ++accounted;
    CHECK(execution.status.code == beeb::RuntimeStatusCode::executionFailed);
    executionFaulted.count_down();
    faultObserved.wait();
    continueQueries.count_down();
    checkRuntimeOK(runtime.reset());
    ++accounted;
    checkRuntimeOK(runtime.loadOSROM(loopingOS));
    ++accounted;
    for (auto& worker : recoveryWorkers)
        worker.join();
    CHECK_EQ(recoveryFailures.load(), 0);
    CHECK_EQ(accounted.load(), threadCount * operationsPerThread + 1 + recoveryObservers +
                                   recoveryObservers * recoveryQueriesPerObserver + 2);
    CHECK(runtimeValue(runtime.state()) == beeb::RuntimeState::paused);
}

void testC1RaceShutdownDrainAndRejection() {
    beeb::MachineRuntime runtime;
    auto loopingOS = makeNOPOSROM();
    loopingOS[0] = 0x4C;
    loopingOS[1] = 0x00;
    loopingOS[2] = 0xC0;
    checkRuntimeOK(runtime.loadOSROM(loopingOS));
    checkRuntimeOK(runtime.reset());
    const auto baselineAcceptance = runtime.acceptedCommandCount();
    constexpr unsigned callerCount = 96;
    std::latch ready(callerCount);
    std::latch release(1);
    std::vector<std::future<beeb::RuntimeStatus>> calls;
    calls.reserve(callerCount);
    for (unsigned index = 0; index < callerCount; ++index) {
        calls.emplace_back(std::async(std::launch::async, [&, index] {
            ready.count_down();
            release.wait();
            if ((index & 1) == 0) return runtime.cpuState().status;
            return runtime.setKey(static_cast<std::uint8_t>(index % 16),
                                  static_cast<std::uint8_t>((index / 16) % 16), true);
        }));
    }
    ready.wait();
    auto longRun =
        std::async(std::launch::async, [&runtime] { return runtime.runFor(50'000'000); });
    waitForC1Acceptance(runtime, baselineAcceptance + 1);
    release.count_down();
    waitForC1Acceptance(runtime, baselineAcceptance + beeb::MachineRuntime::commandCapacity);

    constexpr unsigned shutdownCallerCount = 4;
    std::vector<std::future<beeb::RuntimeStatus>> shutdowns;
    shutdowns.reserve(shutdownCallerCount);
    for (unsigned index = 0; index < shutdownCallerCount; ++index) {
        shutdowns.emplace_back(
            std::async(std::launch::async, [&runtime] { return runtime.shutdown(); }));
    }

    const auto waiterDeadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    bool rejectedWaiterReady = false;
    while (!rejectedWaiterReady && std::chrono::steady_clock::now() < waiterDeadline) {
        rejectedWaiterReady = std::any_of(calls.begin(), calls.end(), [](auto& call) {
            return call.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
        });
        if (!rejectedWaiterReady) std::this_thread::yield();
    }
    CHECK(rejectedWaiterReady);
    CHECK(runtime.start().code == beeb::RuntimeStatusCode::unavailable);

    const auto longRunResult = longRun.get();
    checkRuntimeOK(longRunResult.status);
    std::uint64_t maximumAcceptedSequence = longRunResult.status.acceptanceSequence;
    unsigned accepted = 0;
    unsigned unavailable = 0;
    for (auto& call : calls) {
        const auto status = call.get();
        if (status.acceptanceSequence != 0) {
            checkRuntimeOK(status);
            maximumAcceptedSequence = std::max(maximumAcceptedSequence, status.acceptanceSequence);
            ++accepted;
        } else {
            CHECK(status.code == beeb::RuntimeStatusCode::unavailable);
            ++unavailable;
        }
    }
    CHECK(accepted > 0);
    CHECK(unavailable > 0);

    std::uint64_t shutdownAcceptance = 0;
    for (auto& shutdown : shutdowns) {
        CHECK(shutdown.wait_for(std::chrono::seconds(15)) == std::future_status::ready);
        const auto status = shutdown.get();
        checkRuntimeOK(status);
        shutdownAcceptance = std::max(shutdownAcceptance, status.acceptanceSequence);
    }
    CHECK(shutdownAcceptance > maximumAcceptedSequence);
    checkRuntimeOK(runtime.shutdown());
}

beeb::CompletedFrame c2Frame(std::uint64_t number) {
    beeb::CompletedFrame frame;
    frame.number = number;
    frame.width = 1;
    frame.height = 1;
    frame.rgba = {static_cast<std::uint8_t>(number), 0x22, 0x33, 0xFF};
    return frame;
}

void testC2CompletedFrameFIFOAndAccounting() {
    beeb::CompletedFrameQueue queue;

    const auto empty = queue.dequeue();
    CHECK(empty.status.code == beeb::OutputStatusCode::empty);
    CHECK(!empty.frame.has_value());

    CHECK(queue.publish(c2Frame(10)).code == beeb::OutputStatusCode::ok);
    CHECK(queue.publish(c2Frame(11)).code == beeb::OutputStatusCode::ok);
    CHECK(queue.publish(c2Frame(12)).code == beeb::OutputStatusCode::ok);
    CHECK_EQ(queue.depth(), beeb::completedFrameCapacity);

    auto retained = queue.dequeue();
    CHECK(retained.status.code == beeb::OutputStatusCode::ok);
    CHECK(retained.frame.has_value());
    CHECK_EQ(retained.frame->number, 10U);
    const auto retainedPixels = retained.frame->rgba;

    CHECK(queue.publish(c2Frame(13)).code == beeb::OutputStatusCode::ok);
    CHECK(queue.publish(c2Frame(14)).code == beeb::OutputStatusCode::overrun);
    CHECK_EQ(queue.depth(), beeb::completedFrameCapacity);
    CHECK(retained.frame->rgba == retainedPixels);

    const auto beforeInvalid = queue.counters();
    CHECK(queue.publish(c2Frame(14)).code == beeb::OutputStatusCode::invalidArgument);
    CHECK_EQ(queue.depth(), beeb::completedFrameCapacity);
    CHECK(queue.counters() == beforeInvalid);

    for (const auto expected : {12U, 13U, 14U}) {
        const auto result = queue.dequeue();
        CHECK(result.status.code == beeb::OutputStatusCode::ok);
        CHECK(result.frame.has_value());
        CHECK_EQ(result.frame->number, expected);
    }

    const auto counters = queue.counters();
    CHECK_EQ(queue.depth(), 0U);
    CHECK_EQ(counters.framesProduced, 5U);
    CHECK_EQ(counters.framesConsumed, 4U);
    CHECK_EQ(counters.framesDropped, 1U);
    CHECK_EQ(counters.framesProduced,
             counters.framesConsumed + counters.framesDropped + queue.depth());
}

void testC2CFrameReleaseRequiresOwnershipToken() {
    beeb_frame empty{};
    checkCStatus(beeb_frame_release(&empty), BEEB_STATUS_OK);

    std::uint8_t foreignByte = 0;
    beeb_frame foreign{};
    foreign.available = 1;
    foreign.rgba = &foreignByte;
    foreign.rgba_size = 1;
    checkCStatus(beeb_frame_release(&foreign), BEEB_STATUS_INVALID_ARGUMENT);
    CHECK(foreign.rgba == &foreignByte);
    CHECK_EQ(foreign.rgba_size, 1U);
    CHECK(foreign.release_context == nullptr);
}

void testC2AudioFIFOPressureDemandAndAccounting() {
    beeb::AudioSampleQueue queue;
    CHECK_EQ(queue.capacity(), 4'096U);
    CHECK_EQ(queue.demand(), 2'048U);

    const auto empty = queue.drain(4);
    CHECK(empty.status.code == beeb::OutputStatusCode::underrun);
    CHECK_EQ(empty.chunk.requested, 4U);
    CHECK_EQ(empty.chunk.shortfall, 4U);
    CHECK(empty.chunk.samples.empty());

    const std::array initial{1.0F, 2.0F, 3.0F};
    CHECK(queue.publish(initial).code == beeb::OutputStatusCode::ok);
    auto first = queue.drain(2);
    CHECK(first.status.code == beeb::OutputStatusCode::ok);
    CHECK(first.chunk.samples == std::vector<float>({1.0F, 2.0F}));
    CHECK_EQ(first.chunk.shortfall, 0U);
    CHECK_EQ(queue.depth(), 1U);
    CHECK_EQ(queue.demand(), 2'047U);

    std::vector<float> pressure(beeb::audioSampleCapacity);
    for (std::size_t index = 0; index < pressure.size(); ++index)
        pressure[index] = static_cast<float>(index);
    CHECK(queue.publish(pressure).code == beeb::OutputStatusCode::overrun);
    CHECK_EQ(queue.depth(), beeb::audioSampleCapacity);
    CHECK_EQ(queue.demand(), 0U);

    const auto full = queue.drain(beeb::audioSampleCapacity);
    CHECK(full.status.code == beeb::OutputStatusCode::ok);
    CHECK_EQ(full.chunk.samples.size(), beeb::audioSampleCapacity);
    CHECK_EQ(full.chunk.samples.front(), 0.0F);
    CHECK_EQ(full.chunk.samples.back(), 4'095.0F);

    const auto finalUnderrun = queue.drain(5);
    CHECK(finalUnderrun.status.code == beeb::OutputStatusCode::underrun);
    CHECK_EQ(finalUnderrun.chunk.shortfall, 5U);

    const auto counters = queue.counters();
    CHECK_EQ(counters.audioSamplesProduced, 4'099U);
    CHECK_EQ(counters.audioSamplesConsumed, 4'098U);
    CHECK_EQ(counters.audioSamplesOverrun, 1U);
    CHECK_EQ(counters.audioSamplesUnderrun, 9U);
    CHECK_EQ(counters.audioSamplesProduced,
             counters.audioSamplesConsumed + counters.audioSamplesOverrun + queue.depth());
}

void testC2DiagnosticsAreConsistentAndObservational() {
    beeb::MachineRuntime runtime;
    checkRuntimeOK(runtime.loadOSROM(makeLoopingOSROM()));
    checkRuntimeOK(runtime.reset());

    const auto initial = runtimeValue(runtime.outputDiagnostics());
    const auto initialCPU = runtimeValue(runtime.cpuState());
    CHECK_EQ(initial.totalCycles, initialCPU.cycles);
    CHECK_EQ(initial.latestFrameNumber, 0U);
    CHECK_EQ(initial.frameDepth, 0U);
    CHECK_EQ(initial.frameCapacity, beeb::completedFrameCapacity);
    CHECK_EQ(initial.audioDepth, 0U);
    CHECK_EQ(initial.audioCapacity, beeb::audioSampleCapacity);
    CHECK_EQ(initial.audioDemand, beeb::audioTargetDepth);
    CHECK(initial.counters == beeb::OutputCounters{});
    CHECK(initial.lastStatus == beeb::OutputStatusCode::ok);

    const auto actualCycles = runtimeValue(runtime.runFor(250));
    CHECK(actualCycles >= 250);
    const auto produced = runtimeValue(runtime.outputDiagnostics());
    CHECK_EQ(produced.totalCycles, initial.totalCycles + actualCycles);
    CHECK_EQ(produced.audioDepth, produced.counters.audioSamplesProduced);
    CHECK_EQ(produced.audioDemand, beeb::audioTargetDepth - produced.audioDepth);
    CHECK_EQ(produced.counters.audioSamplesProduced, produced.counters.audioSamplesConsumed +
                                                         produced.counters.audioSamplesOverrun +
                                                         produced.audioDepth);
    CHECK_EQ(produced.counters.framesProduced, produced.counters.framesConsumed +
                                                   produced.counters.framesDropped +
                                                   produced.frameDepth);

    const auto partial = runtime.drainAudio(produced.audioDepth + 5);
    CHECK(partial.status.code == beeb::OutputStatusCode::underrun);
    CHECK_EQ(partial.chunk.samples.size(), produced.audioDepth);
    CHECK_EQ(partial.chunk.shortfall, 5U);

    const auto pressured = runtimeValue(runtime.outputDiagnostics());
    CHECK_EQ(pressured.totalCycles, produced.totalCycles);
    CHECK_EQ(pressured.audioDepth, 0U);
    CHECK_EQ(pressured.audioDemand, beeb::audioTargetDepth);
    CHECK_EQ(pressured.counters.audioSamplesConsumed, produced.audioDepth);
    CHECK_EQ(pressured.counters.audioSamplesUnderrun, 5U);
    CHECK(pressured.lastStatus == beeb::OutputStatusCode::underrun);

    const auto repeated = runtimeValue(runtime.outputDiagnostics());
    CHECK(repeated == pressured);
    CHECK_EQ(runtimeValue(runtime.cpuState()).cycles, pressured.totalCycles);
}

void testC2ResetDiscardsRetainedOutputWithoutBreakingAccounting() {
    beeb::MachineRuntime runtime;
    checkRuntimeOK(runtime.loadOSROM(makeOutputOSROM()));
    checkRuntimeOK(runtime.reset());
    CHECK(runtimeValue(runtime.runUntilFrame(200'000)));
    CHECK(runtimeValue(runtime.runFor(2'000'000)) >= 2'000'000);

    const auto before = runtimeValue(runtime.outputDiagnostics());
    CHECK(before.frameDepth > 0);
    CHECK(before.audioDepth > 0);
    checkRuntimeOK(runtime.reset());

    const auto after = runtimeValue(runtime.outputDiagnostics());
    CHECK_EQ(after.frameDepth, 0U);
    CHECK_EQ(after.audioDepth, 0U);
    CHECK_EQ(after.audioDemand, beeb::audioTargetDepth);
    CHECK_EQ(after.latestFrameNumber, before.latestFrameNumber);
    CHECK_EQ(after.counters.framesProduced, before.counters.framesProduced);
    CHECK_EQ(after.counters.framesConsumed, before.counters.framesConsumed);
    CHECK_EQ(after.counters.framesDropped, before.counters.framesDropped + before.frameDepth);
    CHECK_EQ(after.counters.audioSamplesProduced, before.counters.audioSamplesProduced);
    CHECK_EQ(after.counters.audioSamplesConsumed, before.counters.audioSamplesConsumed);
    CHECK_EQ(after.counters.audioSamplesOverrun,
             before.counters.audioSamplesOverrun + before.audioDepth);
    CHECK(after.lastStatus == beeb::OutputStatusCode::ok);
    CHECK(after.counters.framesProduced ==
          after.counters.framesConsumed + after.counters.framesDropped + after.frameDepth);
    CHECK(after.counters.audioSamplesProduced == after.counters.audioSamplesConsumed +
                                                     after.counters.audioSamplesOverrun +
                                                     after.audioDepth);
    CHECK(runtime.dequeueFrame().status.code == beeb::OutputStatusCode::empty);
    const auto audio = runtime.drainAudio(1);
    CHECK(audio.status.code == beeb::OutputStatusCode::underrun);
    CHECK(audio.chunk.samples.empty());

    const auto postResetCycles = runtimeValue(runtime.runFor(122));
    const auto resumed = runtimeValue(runtime.outputDiagnostics());
    beeb::MachineRuntime fresh;
    checkRuntimeOK(fresh.loadOSROM(makeOutputOSROM()));
    checkRuntimeOK(fresh.reset());
    const auto freshCycles = runtimeValue(fresh.runFor(122));
    const auto freshOutput = runtimeValue(fresh.outputDiagnostics());
    CHECK_EQ(postResetCycles, freshCycles);
    CHECK_EQ(resumed.counters.audioSamplesProduced - after.counters.audioSamplesProduced,
             freshOutput.counters.audioSamplesProduced);
    const auto resumedAudio = runtime.drainAudio(resumed.audioDepth);
    const auto freshAudio = fresh.drainAudio(freshOutput.audioDepth);
    CHECK(resumedAudio.chunk.samples == freshAudio.chunk.samples);
}

void testTargetProfileModelBCoreRetentionAndQuery() {
    const auto canonical = beeb::MachineTargetProfile::modelB();
    const auto validation = beeb::validateMachineTargetProfile(canonical);
    CHECK(validation.support == beeb::ProfileSupport::supported);
    CHECK(validation.message.empty());

    beeb::BBCMicro machine(canonical);
    const auto machineDigest = beeb::BBCMicroTestAccess::digest(machine);
    CHECK(machine.profile() == canonical);
    CHECK_EQ(beeb::BBCMicroTestAccess::digest(machine), machineDigest);

    beeb::MachineRuntime runtime(canonical, {.enableLedger = true});
    const auto beforePoint = runtimeValue(runtime.safePoint());
    const auto beforeCPU = runtimeValue(runtime.cpuState());
    const auto first = runtimeValue(runtime.profile());
    const auto second = runtimeValue(runtime.profile());
    const auto afterCPU = runtimeValue(runtime.cpuState());
    const auto afterPoint = runtimeValue(runtime.safePoint());

    CHECK(first == canonical);
    CHECK(second == canonical);
    CHECK(beforeCPU == afterCPU);
    CHECK_EQ(beforePoint.cpuCycles, afterPoint.cpuCycles);
    CHECK_EQ(beforePoint.frameNumber, afterPoint.frameNumber);
    CHECK(beforePoint.state == afterPoint.state);

    const auto ledger = runtime.ledger();
    const auto profileQueries =
        std::count_if(ledger.begin(), ledger.end(), [](const beeb::LedgerEntry& entry) {
            return entry.command == beeb::RuntimeCommandKind::profile;
        });
    CHECK_EQ(profileQueries, 2U);
    for (const auto& entry : ledger) {
        if (entry.command != beeb::RuntimeCommandKind::profile) continue;
        CHECK_EQ(entry.safePoint.cpuCycles, beforePoint.cpuCycles);
        CHECK_EQ(entry.safePoint.frameNumber, beforePoint.frameNumber);
        CHECK(entry.safePoint.state == beforePoint.state);
    }
}

void testTargetProfileModelBCBoundaryCreationAndQuery() {
    const auto modelB = beeb_machine_profile_model_b();
    auto* const handleCanary = reinterpret_cast<beeb_machine*>(static_cast<std::uintptr_t>(1));

    beeb_machine* untouchedHandle = handleCanary;
    auto status = beeb_create_with_profile(nullptr, &untouchedHandle);
    CHECK(status.code == BEEB_STATUS_INVALID_ARGUMENT);
    CHECK(untouchedHandle == handleCanary);
    status = beeb_create_with_profile(&modelB, nullptr);
    CHECK(status.code == BEEB_STATUS_INVALID_ARGUMENT);

    beeb_machine* explicitMachine = nullptr;
    status = beeb_create_with_profile(&modelB, &explicitMachine);
    CHECK(status.code == BEEB_STATUS_OK);
    CHECK(explicitMachine != nullptr);

    beeb_machine_profile canary{};
    canary.schema_version = 77;
    canary.base.identifier = UINT32_C(0xf0000001);
    const auto originalCanary = canary;
    status = beeb_get_machine_profile(nullptr, &canary);
    CHECK(status.code == BEEB_STATUS_INVALID_ARGUMENT);
    CHECK(beeb_machine_profile_equal(&canary, &originalCanary));
    status = beeb_get_machine_profile(explicitMachine, nullptr);
    CHECK(status.code == BEEB_STATUS_INVALID_ARGUMENT);

    beeb_machine_profile first{};
    beeb_machine_profile second{};
    CHECK(beeb_get_machine_profile(explicitMachine, &first).code == BEEB_STATUS_OK);
    first.base.identifier = UINT32_C(0xf0000001);
    CHECK(beeb_get_machine_profile(explicitMachine, &second).code == BEEB_STATUS_OK);
    CHECK(beeb_machine_profile_equal(&second, &modelB));
    CHECK(!beeb_machine_profile_equal(&first, &second));
    CHECK(beeb_destroy(explicitMachine).code == BEEB_STATUS_OK);

    beeb_machine* legacyMachine = nullptr;
    CHECK(beeb_create(&legacyMachine).code == BEEB_STATUS_OK);
    beeb_machine_profile legacyProfile{};
    CHECK(beeb_get_machine_profile(legacyMachine, &legacyProfile).code == BEEB_STATUS_OK);
    CHECK(beeb_machine_profile_equal(&legacyProfile, &modelB));
    CHECK(beeb_destroy(legacyMachine).code == BEEB_STATUS_OK);
}

void testTargetProfileModelBPlusRecognitionAndRejection() {
    const auto modelBPlus = beeb::MachineTargetProfile::modelBPlus64K();
    const auto validation = beeb::validateMachineTargetProfile(modelBPlus);
    CHECK(validation.support == beeb::ProfileSupport::recognisedUnavailable);
    CHECK(validation.message.find("Model B+ 64K") != std::string::npos);
    CHECK(validation.message.find("unavailable") != std::string::npos);

    bool machineRejected = false;
    try {
        const beeb::BBCMicro machine(modelBPlus);
    } catch (const std::invalid_argument&) {
        machineRejected = true;
    }
    CHECK(machineRejected);

    bool runtimeRejected = false;
    try {
        const beeb::MachineRuntime runtime(modelBPlus);
    } catch (const std::invalid_argument&) {
        runtimeRejected = true;
    }
    CHECK(runtimeRejected);

    const auto cModelB = beeb_machine_profile_model_b();
    const auto cModelBPlus = beeb_machine_profile_model_b_plus_64k();
    CHECK(!beeb_machine_profile_equal(&cModelBPlus, &cModelB));

    auto status = beeb_create_with_profile(&cModelBPlus, nullptr);
    CHECK(status.code == BEEB_STATUS_INVALID_ARGUMENT);

    beeb_machine* activeModelB = nullptr;
    CHECK(beeb_create_with_profile(&cModelB, &activeModelB).code == BEEB_STATUS_OK);
    CHECK(activeModelB != nullptr);
    beeb_machine_profile beforeProfile{};
    CHECK(beeb_get_machine_profile(activeModelB, &beforeProfile).code == BEEB_STATUS_OK);

    auto* const handleCanary = reinterpret_cast<beeb_machine*>(static_cast<std::uintptr_t>(1));
    beeb_machine* rejectedOutput = handleCanary;
    status = beeb_create_with_profile(&cModelBPlus, &rejectedOutput);
    CHECK(status.code == BEEB_STATUS_UNAVAILABLE);
    CHECK(std::string{status.message}.find("Model B+ 64K") != std::string::npos);
    CHECK(std::string{status.message}.find("unavailable") != std::string::npos);
    CHECK(rejectedOutput == handleCanary);
    CHECK(beeb_destroy(rejectedOutput).code == BEEB_STATUS_INVALID_ARGUMENT);

    beeb_machine_profile afterProfile{};
    CHECK(beeb_get_machine_profile(activeModelB, &afterProfile).code == BEEB_STATUS_OK);
    CHECK(beeb_machine_profile_equal(&beforeProfile, &afterProfile));
    CHECK(beeb_machine_profile_equal(&afterProfile, &cModelB));
    CHECK(beeb_destroy(activeModelB).code == BEEB_STATUS_OK);
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
        {"trace observer failure is atomic", testTraceObserverFailureIsAtomic},
        {"C 0.2: status out-parameters and nullability",
         testCAPI02StatusOutParametersAndNullability},
        {"C 0.2: fault detail and reset recovery", testCAPI02FaultAndRecovery},
        {"C 0.2: destroy waits for calls already inside",
         testCAPI02DestroyWaitsForCallsAlreadyInside},
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
        {"teletext control cells use active background",
         testTeletextControlCellsUseActiveBackground},
        {"C1 contract: lifecycle command matrix", testC1RuntimeContractLifecycleMatrix},
        {"C1 contract: complete 18-command lifecycle matrix", testC1RuntimeCompleteCommandMatrix},
        {"C1 contract: fault and recovery matrix", testC1RuntimeContractFaultAndRecoveryMatrix},
        {"C1 contract: structured status isolation",
         testC1RuntimeContractStructuredStatusIsolation},
        {"C1 faults: late mutations retain the last completed boundary",
         testC1LateFaultRetainsLastCompletedBoundary},
        {"C1 lifecycle: closed sustained fixture repeats 50 times",
         testC1ClosedLoopSustainedLifecycleRepeats},
        {"C1 replay: machine digest covers rollback state", testC1MachineDigestCoversRollbackState},
        {"C1 allocation: failures remain recoverable", testC1AllocationFailuresRemainRecoverable},
        {"C1 contract: owner reentrant submission is rejected",
         testC1OwnerReentrantSubmissionIsRejected},
        {"C1 replay: deterministic command and safe-point ledger", testC1ReplayDeterministicLedger},
        {"C1 replay: captured concurrent ledger replays exactly",
         testC1ReplayCapturedConcurrentLedgerExactly},
        {"C1 contract: fixed execution slices share ledger order",
         testC1RuntimeFixedExecutionSlices},
        {"C1 lifecycle: accepted pause completes within one slice",
         testC1PauseCompletesWithinOneAcceptedSlice},
        {"C1 transactions: FIFO reset load query and explicit resume",
         testC1TransactionsFIFOAndNoAutoResume},
        {"C1 transactions: invalid input is atomic and payload is copied",
         testC1TransactionsRejectAtomicallyAndCopyInput},
        {"C1 transactions: media payloads are copied and reject atomically",
         testC1MediaTransactionsCopyAndRejectAtomically},
        {"C1 input: keyboard and BREAK serialize with reset",
         testC1InputAndBreakSerializeWithReset},
        {"C1 observations: CPU frame and audio values are owned",
         testC1ObservationsReturnConsistentOwnedValues},
        {"C1 race: 10000 mixed commands", testC1RaceMixedCommands},
        {"C1 race: shutdown drain and rejection", testC1RaceShutdownDrainAndRejection},
        {"C2 frames: FIFO overflow ownership and accounting",
         testC2CompletedFrameFIFOAndAccounting},
        {"C2 frames: C release requires the matching ownership token",
         testC2CFrameReleaseRequiresOwnershipToken},
        {"C2 audio: FIFO pressure demand and accounting",
         testC2AudioFIFOPressureDemandAndAccounting},
        {"C2 diagnostics: consistent observational pressure snapshot",
         testC2DiagnosticsAreConsistentAndObservational},
        {"C2 reset: retained output is discarded with exact accounting",
         testC2ResetDiscardsRetainedOutputWithoutBreakingAccounting},
        {"Target profile: Model B core retention and query",
         testTargetProfileModelBCoreRetentionAndQuery},
        {"Target profile: Model B C creation and owned query",
         testTargetProfileModelBCBoundaryCreationAndQuery},
        {"Target profile: Model B+ recognition and rejection",
         testTargetProfileModelBPlusRecognitionAndRejection},
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
