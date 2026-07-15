#pragma once

#include <cstdint>

namespace beeb {

class Bus {
public:
    virtual ~Bus() = default;
    virtual std::uint8_t read(std::uint16_t address) = 0;
    virtual void write(std::uint16_t address, std::uint8_t value) = 0;

    // Called after an instruction or interrupt sequence. Implementations use
    // this to advance peripherals on the same 2 MHz CPU timebase.
    virtual void tick(std::uint32_t cpuCycles) { (void)cpuCycles; }
};

} // namespace beeb
