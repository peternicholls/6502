#pragma once

#include "beeb/bus.hpp"

#include <cstdint>
#include <functional>
#include <string>

namespace beeb {

struct CPUState {
    std::uint8_t a = 0;
    std::uint8_t x = 0;
    std::uint8_t y = 0;
    std::uint8_t sp = 0;
    std::uint8_t p = 0x24;
    std::uint16_t pc = 0;
    std::uint64_t cycles = 0;
};

class CPU6502 {
public:
    enum Flag : std::uint8_t {
        Carry = 0x01,
        Zero = 0x02,
        InterruptDisable = 0x04,
        Decimal = 0x08,
        Break = 0x10,
        Unused = 0x20,
        Overflow = 0x40,
        Negative = 0x80,
    };

    using TraceCallback = std::function<void(const CPUState&, std::uint8_t)>;

    explicit CPU6502(Bus& bus);

    void reset();
    std::uint32_t step();
    void setIRQ(bool asserted) noexcept { irqLine_ = asserted; }
    void requestNMI() noexcept { nmiPending_ = true; }

    [[nodiscard]] CPUState state() const noexcept;
    void setState(const CPUState& state) noexcept;
    [[nodiscard]] bool flag(Flag f) const noexcept;
    void setFlag(Flag f, bool value) noexcept;
    void setTraceCallback(TraceCallback callback) { trace_ = std::move(callback); }

private:
    Bus& bus_;
    std::uint8_t a_ = 0;
    std::uint8_t x_ = 0;
    std::uint8_t y_ = 0;
    std::uint8_t sp_ = 0;
    std::uint8_t p_ = Unused | InterruptDisable;
    std::uint16_t pc_ = 0;
    std::uint64_t cycles_ = 0;
    bool irqLine_ = false;
    bool nmiPending_ = false;
    TraceCallback trace_;

    std::uint8_t read(std::uint16_t address) { return bus_.read(address); }
    void write(std::uint16_t address, std::uint8_t value) { bus_.write(address, value); }
    std::uint8_t fetch8();
    std::uint16_t fetch16();
    std::uint16_t read16(std::uint16_t address);
    std::uint16_t read16ZeroPage(std::uint8_t address);
    std::uint16_t read16JMPBug(std::uint16_t address);

    std::uint16_t zp();
    std::uint16_t zpx();
    std::uint16_t zpy();
    std::uint16_t abs();
    std::uint16_t absx(bool& pageCrossed);
    std::uint16_t absy(bool& pageCrossed);
    std::uint16_t indx();
    std::uint16_t indy(bool& pageCrossed);

    void push(std::uint8_t value);
    std::uint8_t pull();
    void setNZ(std::uint8_t value);
    void compare(std::uint8_t lhs, std::uint8_t rhs);
    void adc(std::uint8_t value);
    void sbc(std::uint8_t value);
    std::uint8_t asl(std::uint8_t value);
    std::uint8_t lsr(std::uint8_t value);
    std::uint8_t rol(std::uint8_t value);
    std::uint8_t ror(std::uint8_t value);
    std::uint32_t branch(bool condition);
    std::uint32_t interrupt(std::uint16_t vector);
    std::uint32_t finish(std::uint32_t cycles);
    [[noreturn]] void illegal(std::uint8_t opcode, std::uint16_t address) const;
};

} // namespace beeb
