#pragma once

// C0-DOC-RATIONALE: docs/code/timing-model.md owns the bus timing boundary.

#include <cstdint>

namespace beeb {

/// Memory and elapsed-time boundary used by the 6502 processor.
///
/// Implementations own the addressed storage and devices. CPU6502 calls tick()
/// once after each complete instruction or interrupt sequence.
class Bus {
  public:
    /// Allows destruction through the interface.
    virtual ~Bus() = default;

    /// Reads one byte from the 16-bit processor address space.
    /// @param address Address presented by the processor.
    /// @return Byte visible at `address`, including device read side effects.
    virtual std::uint8_t read(std::uint16_t address) = 0;

    /// Writes one byte to the 16-bit processor address space.
    /// @param address Address presented by the processor.
    /// @param value Value written by the processor.
    virtual void write(std::uint16_t address, std::uint8_t value) = 0;

    /// Advances bus-owned devices after a complete CPU state transition.
    /// @param cpuCycles CPU cycles consumed by that instruction or interrupt.
    virtual void tick(std::uint32_t cpuCycles) { (void)cpuCycles; }
};

} // namespace beeb
