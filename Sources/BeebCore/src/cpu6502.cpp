#include "beeb/cpu6502.hpp"

#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace beeb {

CPU6502::CPU6502(Bus& bus) : bus_(bus) {}

CPUState CPU6502::state() const noexcept {
    return {a_, x_, y_, sp_, static_cast<std::uint8_t>(p_ | Unused), pc_, cycles_};
}

void CPU6502::setState(const CPUState& s) noexcept {
    a_ = s.a;
    x_ = s.x;
    y_ = s.y;
    sp_ = s.sp;
    p_ = static_cast<std::uint8_t>((s.p | Unused) & ~Break);
    pc_ = s.pc;
    cycles_ = s.cycles;
}

bool CPU6502::flag(Flag f) const noexcept {
    return (p_ & f) != 0;
}

void CPU6502::setFlag(Flag f, bool value) noexcept {
    if (value) p_ |= f;
    else p_ &= static_cast<std::uint8_t>(~f);
    p_ |= Unused;
}

std::uint8_t CPU6502::fetch8() { return read(pc_++); }

std::uint16_t CPU6502::fetch16() {
    const auto lo = fetch8();
    const auto hi = fetch8();
    return static_cast<std::uint16_t>(lo | (static_cast<std::uint16_t>(hi) << 8));
}

std::uint16_t CPU6502::read16(std::uint16_t address) {
    const auto lo = read(address);
    const auto hi = read(static_cast<std::uint16_t>(address + 1));
    return static_cast<std::uint16_t>(lo | (static_cast<std::uint16_t>(hi) << 8));
}

std::uint16_t CPU6502::read16ZeroPage(std::uint8_t address) {
    const auto lo = read(address);
    const auto hi = read(static_cast<std::uint8_t>(address + 1));
    return static_cast<std::uint16_t>(lo | (static_cast<std::uint16_t>(hi) << 8));
}

std::uint16_t CPU6502::read16JMPBug(std::uint16_t address) {
    const auto lo = read(address);
    const auto next = static_cast<std::uint16_t>((address & 0xFF00) | ((address + 1) & 0x00FF));
    const auto hi = read(next);
    return static_cast<std::uint16_t>(lo | (static_cast<std::uint16_t>(hi) << 8));
}

std::uint16_t CPU6502::zp() { return fetch8(); }
std::uint16_t CPU6502::zpx() { return static_cast<std::uint8_t>(fetch8() + x_); }
std::uint16_t CPU6502::zpy() { return static_cast<std::uint8_t>(fetch8() + y_); }
std::uint16_t CPU6502::abs() { return fetch16(); }

std::uint16_t CPU6502::absx(bool& pageCrossed) {
    const auto base = fetch16();
    const auto address = static_cast<std::uint16_t>(base + x_);
    pageCrossed = (base & 0xFF00) != (address & 0xFF00);
    return address;
}

std::uint16_t CPU6502::absy(bool& pageCrossed) {
    const auto base = fetch16();
    const auto address = static_cast<std::uint16_t>(base + y_);
    pageCrossed = (base & 0xFF00) != (address & 0xFF00);
    return address;
}

std::uint16_t CPU6502::indx() {
    return read16ZeroPage(static_cast<std::uint8_t>(fetch8() + x_));
}

std::uint16_t CPU6502::indy(bool& pageCrossed) {
    const auto base = read16ZeroPage(fetch8());
    const auto address = static_cast<std::uint16_t>(base + y_);
    pageCrossed = (base & 0xFF00) != (address & 0xFF00);
    return address;
}

void CPU6502::push(std::uint8_t value) {
    write(static_cast<std::uint16_t>(0x0100 | sp_), value);
    --sp_;
}

std::uint8_t CPU6502::pull() {
    ++sp_;
    return read(static_cast<std::uint16_t>(0x0100 | sp_));
}

void CPU6502::setNZ(std::uint8_t value) {
    setFlag(Zero, value == 0);
    setFlag(Negative, (value & 0x80) != 0);
}

void CPU6502::compare(std::uint8_t lhs, std::uint8_t rhs) {
    const auto result = static_cast<std::uint8_t>(lhs - rhs);
    setFlag(Carry, lhs >= rhs);
    setNZ(result);
}

void CPU6502::adc(std::uint8_t value) {
    const auto carryIn = flag(Carry) ? 1u : 0u;
    if (!flag(Decimal)) {
        const auto sum = static_cast<unsigned>(a_) + value + carryIn;
        const auto result = static_cast<std::uint8_t>(sum);
        setFlag(Carry, sum > 0xFF);
        setFlag(Overflow, ((~(a_ ^ value) & (a_ ^ result)) & 0x80) != 0);
        a_ = result;
        setNZ(a_);
        return;
    }

    // NMOS 6502 decimal datapath. N, V and Z intentionally reflect internal
    // binary/intermediate values rather than the final BCD-adjusted result.
    auto low = static_cast<unsigned>(a_ & 0x0F) + (value & 0x0F) + carryIn;
    if (low >= 0x0A) low = ((low + 0x06) & 0x0F) + 0x10;
    auto sum = static_cast<unsigned>(a_ & 0xF0) + (value & 0xF0) + low;
    const auto binary = static_cast<unsigned>(a_) + value + carryIn;
    setFlag(Zero, (binary & 0xFF) == 0);
    setFlag(Negative, (sum & 0x80) != 0);
    setFlag(Overflow, ((~(a_ ^ value) & (a_ ^ sum)) & 0x80) != 0);
    if (sum >= 0xA0) sum += 0x60;
    setFlag(Carry, sum >= 0x100);
    a_ = static_cast<std::uint8_t>(sum);
}

void CPU6502::sbc(std::uint8_t value) {
    const auto borrow = flag(Carry) ? 0u : 1u;
    const auto binaryWide = static_cast<unsigned>(a_) - value - borrow;
    const auto binary = static_cast<std::uint8_t>(binaryWide);
    setFlag(Carry, binaryWide < 0x100);
    setFlag(Overflow, (((a_ ^ value) & (a_ ^ binary)) & 0x80) != 0);
    setNZ(binary);

    if (!flag(Decimal)) {
        a_ = binary;
        return;
    }

    auto low = static_cast<int>(a_ & 0x0F) - static_cast<int>(value & 0x0F) - static_cast<int>(borrow);
    if (low < 0) low -= 6;
    auto result = static_cast<int>(a_ & 0xF0) - static_cast<int>(value & 0xF0) + low;
    if (result < 0) result -= 0x60;
    a_ = static_cast<std::uint8_t>(result);
}

std::uint8_t CPU6502::asl(std::uint8_t value) {
    setFlag(Carry, (value & 0x80) != 0);
    value = static_cast<std::uint8_t>(value << 1);
    setNZ(value);
    return value;
}

std::uint8_t CPU6502::lsr(std::uint8_t value) {
    setFlag(Carry, (value & 0x01) != 0);
    value = static_cast<std::uint8_t>(value >> 1);
    setNZ(value);
    return value;
}

std::uint8_t CPU6502::rol(std::uint8_t value) {
    const auto carryIn = flag(Carry) ? 1u : 0u;
    setFlag(Carry, (value & 0x80) != 0);
    value = static_cast<std::uint8_t>((value << 1) | carryIn);
    setNZ(value);
    return value;
}

std::uint8_t CPU6502::ror(std::uint8_t value) {
    const auto carryIn = flag(Carry) ? 0x80u : 0u;
    setFlag(Carry, (value & 0x01) != 0);
    value = static_cast<std::uint8_t>((value >> 1) | carryIn);
    setNZ(value);
    return value;
}

std::uint32_t CPU6502::branch(bool condition) {
    const auto offset = static_cast<std::int8_t>(fetch8());
    if (!condition) return 2;
    const auto oldPC = pc_;
    pc_ = static_cast<std::uint16_t>(pc_ + offset);
    return 3 + (((oldPC ^ pc_) & 0xFF00) != 0 ? 1 : 0);
}

std::uint32_t CPU6502::interrupt(std::uint16_t vector) {
    push(static_cast<std::uint8_t>(pc_ >> 8));
    push(static_cast<std::uint8_t>(pc_));
    push(static_cast<std::uint8_t>((p_ | Unused) & ~Break));
    setFlag(InterruptDisable, true);
    pc_ = read16(vector);
    return finish(7);
}

std::uint32_t CPU6502::finish(std::uint32_t cycles) {
    cycles_ += cycles;
    bus_.tick(cycles);
    return cycles;
}

void CPU6502::reset() {
    sp_ = static_cast<std::uint8_t>(sp_ - 3);
    setFlag(InterruptDisable, true);
    pc_ = read16(0xFFFC);
    nmiPending_ = false;
    finish(7);
}

[[noreturn]] void CPU6502::illegal(std::uint8_t opcode, std::uint16_t address) const {
    std::ostringstream message;
    message << "unsupported NMOS 6502 opcode $" << std::hex << std::uppercase
            << std::setw(2) << std::setfill('0') << static_cast<unsigned>(opcode)
            << " at $" << std::setw(4) << address;
    throw std::runtime_error(message.str());
}

std::uint32_t CPU6502::step() {
    if (nmiPending_) {
        nmiPending_ = false;
        return interrupt(0xFFFA);
    }
    if (irqLine_ && !flag(InterruptDisable)) return interrupt(0xFFFE);

    const auto opcodeAddress = pc_;
    const auto opcode = fetch8();
    if (trace_) trace_(state(), opcode);
    bool crossed = false;
    std::uint16_t address = 0;
    std::uint8_t value = 0;

    const auto readOp = [&](std::uint16_t addr) { return read(addr); };
    const auto rmw = [&](std::uint16_t addr, auto operation) {
        const auto old = read(addr);
        write(addr, old); // NMOS read-modify-write dummy write
        write(addr, operation(old));
    };

    switch (opcode) {
        case 0x00: // BRK
            ++pc_;
            push(static_cast<std::uint8_t>(pc_ >> 8));
            push(static_cast<std::uint8_t>(pc_));
            push(static_cast<std::uint8_t>(p_ | Break | Unused));
            setFlag(InterruptDisable, true);
            pc_ = read16(0xFFFE);
            return finish(7);

        case 0x01: a_ |= readOp(indx()); setNZ(a_); return finish(6);
        case 0x05: a_ |= readOp(zp()); setNZ(a_); return finish(3);
        case 0x06: address = zp(); rmw(address, [&](auto v) { return asl(v); }); return finish(5);
        case 0x08: push(static_cast<std::uint8_t>(p_ | Break | Unused)); return finish(3);
        case 0x09: a_ |= fetch8(); setNZ(a_); return finish(2);
        case 0x0A: a_ = asl(a_); return finish(2);
        case 0x0D: a_ |= readOp(abs()); setNZ(a_); return finish(4);
        case 0x0E: address = abs(); rmw(address, [&](auto v) { return asl(v); }); return finish(6);
        case 0x10: return finish(branch(!flag(Negative)));
        case 0x11: address = indy(crossed); a_ |= readOp(address); setNZ(a_); return finish(5 + crossed);
        case 0x15: a_ |= readOp(zpx()); setNZ(a_); return finish(4);
        case 0x16: address = zpx(); rmw(address, [&](auto v) { return asl(v); }); return finish(6);
        case 0x18: setFlag(Carry, false); return finish(2);
        case 0x19: address = absy(crossed); a_ |= readOp(address); setNZ(a_); return finish(4 + crossed);
        case 0x1D: address = absx(crossed); a_ |= readOp(address); setNZ(a_); return finish(4 + crossed);
        case 0x1E: address = absx(crossed); rmw(address, [&](auto v) { return asl(v); }); return finish(7);

        case 0x20: {
            const auto target = fetch16();
            const auto returnAddress = static_cast<std::uint16_t>(pc_ - 1);
            push(static_cast<std::uint8_t>(returnAddress >> 8));
            push(static_cast<std::uint8_t>(returnAddress));
            pc_ = target;
            return finish(6);
        }
        case 0x21: a_ &= readOp(indx()); setNZ(a_); return finish(6);
        case 0x24: value = readOp(zp()); setFlag(Zero, (a_ & value) == 0); setFlag(Negative, value & 0x80); setFlag(Overflow, value & 0x40); return finish(3);
        case 0x25: a_ &= readOp(zp()); setNZ(a_); return finish(3);
        case 0x26: address = zp(); rmw(address, [&](auto v) { return rol(v); }); return finish(5);
        case 0x28: p_ = static_cast<std::uint8_t>((pull() | Unused) & ~Break); return finish(4);
        case 0x29: a_ &= fetch8(); setNZ(a_); return finish(2);
        case 0x2A: a_ = rol(a_); return finish(2);
        case 0x2C: value = readOp(abs()); setFlag(Zero, (a_ & value) == 0); setFlag(Negative, value & 0x80); setFlag(Overflow, value & 0x40); return finish(4);
        case 0x2D: a_ &= readOp(abs()); setNZ(a_); return finish(4);
        case 0x2E: address = abs(); rmw(address, [&](auto v) { return rol(v); }); return finish(6);
        case 0x30: return finish(branch(flag(Negative)));
        case 0x31: address = indy(crossed); a_ &= readOp(address); setNZ(a_); return finish(5 + crossed);
        case 0x35: a_ &= readOp(zpx()); setNZ(a_); return finish(4);
        case 0x36: address = zpx(); rmw(address, [&](auto v) { return rol(v); }); return finish(6);
        case 0x38: setFlag(Carry, true); return finish(2);
        case 0x39: address = absy(crossed); a_ &= readOp(address); setNZ(a_); return finish(4 + crossed);
        case 0x3D: address = absx(crossed); a_ &= readOp(address); setNZ(a_); return finish(4 + crossed);
        case 0x3E: address = absx(crossed); rmw(address, [&](auto v) { return rol(v); }); return finish(7);

        case 0x40:
            p_ = static_cast<std::uint8_t>((pull() | Unused) & ~Break);
            pc_ = pull(); pc_ |= static_cast<std::uint16_t>(pull()) << 8;
            return finish(6);
        case 0x41: a_ ^= readOp(indx()); setNZ(a_); return finish(6);
        case 0x45: a_ ^= readOp(zp()); setNZ(a_); return finish(3);
        case 0x46: address = zp(); rmw(address, [&](auto v) { return lsr(v); }); return finish(5);
        case 0x48: push(a_); return finish(3);
        case 0x49: a_ ^= fetch8(); setNZ(a_); return finish(2);
        case 0x4A: a_ = lsr(a_); return finish(2);
        case 0x4C: pc_ = abs(); return finish(3);
        case 0x4D: a_ ^= readOp(abs()); setNZ(a_); return finish(4);
        case 0x4E: address = abs(); rmw(address, [&](auto v) { return lsr(v); }); return finish(6);
        case 0x50: return finish(branch(!flag(Overflow)));
        case 0x51: address = indy(crossed); a_ ^= readOp(address); setNZ(a_); return finish(5 + crossed);
        case 0x55: a_ ^= readOp(zpx()); setNZ(a_); return finish(4);
        case 0x56: address = zpx(); rmw(address, [&](auto v) { return lsr(v); }); return finish(6);
        case 0x58: setFlag(InterruptDisable, false); return finish(2);
        case 0x59: address = absy(crossed); a_ ^= readOp(address); setNZ(a_); return finish(4 + crossed);
        case 0x5D: address = absx(crossed); a_ ^= readOp(address); setNZ(a_); return finish(4 + crossed);
        case 0x5E: address = absx(crossed); rmw(address, [&](auto v) { return lsr(v); }); return finish(7);

        case 0x60:
            pc_ = pull(); pc_ |= static_cast<std::uint16_t>(pull()) << 8; ++pc_;
            return finish(6);
        case 0x61: adc(readOp(indx())); return finish(6);
        case 0x65: adc(readOp(zp())); return finish(3);
        case 0x66: address = zp(); rmw(address, [&](auto v) { return ror(v); }); return finish(5);
        case 0x68: a_ = pull(); setNZ(a_); return finish(4);
        case 0x69: adc(fetch8()); return finish(2);
        case 0x6A: a_ = ror(a_); return finish(2);
        case 0x6C: pc_ = read16JMPBug(abs()); return finish(5);
        case 0x6D: adc(readOp(abs())); return finish(4);
        case 0x6E: address = abs(); rmw(address, [&](auto v) { return ror(v); }); return finish(6);
        case 0x70: return finish(branch(flag(Overflow)));
        case 0x71: address = indy(crossed); adc(readOp(address)); return finish(5 + crossed);
        case 0x75: adc(readOp(zpx())); return finish(4);
        case 0x76: address = zpx(); rmw(address, [&](auto v) { return ror(v); }); return finish(6);
        case 0x78: setFlag(InterruptDisable, true); return finish(2);
        case 0x79: address = absy(crossed); adc(readOp(address)); return finish(4 + crossed);
        case 0x7D: address = absx(crossed); adc(readOp(address)); return finish(4 + crossed);
        case 0x7E: address = absx(crossed); rmw(address, [&](auto v) { return ror(v); }); return finish(7);

        case 0x81: write(indx(), a_); return finish(6);
        case 0x84: write(zp(), y_); return finish(3);
        case 0x85: write(zp(), a_); return finish(3);
        case 0x86: write(zp(), x_); return finish(3);
        case 0x88: --y_; setNZ(y_); return finish(2);
        case 0x8A: a_ = x_; setNZ(a_); return finish(2);
        case 0x8C: write(abs(), y_); return finish(4);
        case 0x8D: write(abs(), a_); return finish(4);
        case 0x8E: write(abs(), x_); return finish(4);
        case 0x90: return finish(branch(!flag(Carry)));
        case 0x91: address = indy(crossed); write(address, a_); return finish(6);
        case 0x94: write(zpx(), y_); return finish(4);
        case 0x95: write(zpx(), a_); return finish(4);
        case 0x96: write(zpy(), x_); return finish(4);
        case 0x98: a_ = y_; setNZ(a_); return finish(2);
        case 0x99: address = absy(crossed); write(address, a_); return finish(5);
        case 0x9A: sp_ = x_; return finish(2);
        case 0x9D: address = absx(crossed); write(address, a_); return finish(5);

        case 0xA0: y_ = fetch8(); setNZ(y_); return finish(2);
        case 0xA1: a_ = readOp(indx()); setNZ(a_); return finish(6);
        case 0xA2: x_ = fetch8(); setNZ(x_); return finish(2);
        case 0xA4: y_ = readOp(zp()); setNZ(y_); return finish(3);
        case 0xA5: a_ = readOp(zp()); setNZ(a_); return finish(3);
        case 0xA6: x_ = readOp(zp()); setNZ(x_); return finish(3);
        case 0xA8: y_ = a_; setNZ(y_); return finish(2);
        case 0xA9: a_ = fetch8(); setNZ(a_); return finish(2);
        case 0xAA: x_ = a_; setNZ(x_); return finish(2);
        case 0xAC: y_ = readOp(abs()); setNZ(y_); return finish(4);
        case 0xAD: a_ = readOp(abs()); setNZ(a_); return finish(4);
        case 0xAE: x_ = readOp(abs()); setNZ(x_); return finish(4);
        case 0xB0: return finish(branch(flag(Carry)));
        case 0xB1: address = indy(crossed); a_ = readOp(address); setNZ(a_); return finish(5 + crossed);
        case 0xB4: y_ = readOp(zpx()); setNZ(y_); return finish(4);
        case 0xB5: a_ = readOp(zpx()); setNZ(a_); return finish(4);
        case 0xB6: x_ = readOp(zpy()); setNZ(x_); return finish(4);
        case 0xB8: setFlag(Overflow, false); return finish(2);
        case 0xB9: address = absy(crossed); a_ = readOp(address); setNZ(a_); return finish(4 + crossed);
        case 0xBA: x_ = sp_; setNZ(x_); return finish(2);
        case 0xBC: address = absx(crossed); y_ = readOp(address); setNZ(y_); return finish(4 + crossed);
        case 0xBD: address = absx(crossed); a_ = readOp(address); setNZ(a_); return finish(4 + crossed);
        case 0xBE: address = absy(crossed); x_ = readOp(address); setNZ(x_); return finish(4 + crossed);

        case 0xC0: compare(y_, fetch8()); return finish(2);
        case 0xC1: compare(a_, readOp(indx())); return finish(6);
        case 0xC4: compare(y_, readOp(zp())); return finish(3);
        case 0xC5: compare(a_, readOp(zp())); return finish(3);
        case 0xC6: address = zp(); rmw(address, [&](auto v) { --v; setNZ(v); return v; }); return finish(5);
        case 0xC8: ++y_; setNZ(y_); return finish(2);
        case 0xC9: compare(a_, fetch8()); return finish(2);
        case 0xCA: --x_; setNZ(x_); return finish(2);
        case 0xCC: compare(y_, readOp(abs())); return finish(4);
        case 0xCD: compare(a_, readOp(abs())); return finish(4);
        case 0xCE: address = abs(); rmw(address, [&](auto v) { --v; setNZ(v); return v; }); return finish(6);
        case 0xD0: return finish(branch(!flag(Zero)));
        case 0xD1: address = indy(crossed); compare(a_, readOp(address)); return finish(5 + crossed);
        case 0xD5: compare(a_, readOp(zpx())); return finish(4);
        case 0xD6: address = zpx(); rmw(address, [&](auto v) { --v; setNZ(v); return v; }); return finish(6);
        case 0xD8: setFlag(Decimal, false); return finish(2);
        case 0xD9: address = absy(crossed); compare(a_, readOp(address)); return finish(4 + crossed);
        case 0xDD: address = absx(crossed); compare(a_, readOp(address)); return finish(4 + crossed);
        case 0xDE: address = absx(crossed); rmw(address, [&](auto v) { --v; setNZ(v); return v; }); return finish(7);

        case 0xE0: compare(x_, fetch8()); return finish(2);
        case 0xE1: sbc(readOp(indx())); return finish(6);
        case 0xE4: compare(x_, readOp(zp())); return finish(3);
        case 0xE5: sbc(readOp(zp())); return finish(3);
        case 0xE6: address = zp(); rmw(address, [&](auto v) { ++v; setNZ(v); return v; }); return finish(5);
        case 0xE8: ++x_; setNZ(x_); return finish(2);
        case 0xE9: sbc(fetch8()); return finish(2);
        case 0xEA: return finish(2);
        case 0xEC: compare(x_, readOp(abs())); return finish(4);
        case 0xED: sbc(readOp(abs())); return finish(4);
        case 0xEE: address = abs(); rmw(address, [&](auto v) { ++v; setNZ(v); return v; }); return finish(6);
        case 0xF0: return finish(branch(flag(Zero)));
        case 0xF1: address = indy(crossed); sbc(readOp(address)); return finish(5 + crossed);
        case 0xF5: sbc(readOp(zpx())); return finish(4);
        case 0xF6: address = zpx(); rmw(address, [&](auto v) { ++v; setNZ(v); return v; }); return finish(6);
        case 0xF8: setFlag(Decimal, true); return finish(2);
        case 0xF9: address = absy(crossed); sbc(readOp(address)); return finish(4 + crossed);
        case 0xFD: address = absx(crossed); sbc(readOp(address)); return finish(4 + crossed);
        case 0xFE: address = absx(crossed); rmw(address, [&](auto v) { ++v; setNZ(v); return v; }); return finish(7);

        default: illegal(opcode, opcodeAddress);
    }
}

} // namespace beeb
