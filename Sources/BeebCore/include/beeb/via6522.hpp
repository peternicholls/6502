#pragma once

// C0-DOC-RATIONALE: docs/code/timing-model.md owns aggregate VIA timing.

#include <cstdint>
#include <functional>

namespace beeb {

/// Register, timer, edge, and interrupt model of the MOS 6522 VIA.
/// A machine owns and serializes each instance. Input/output callbacks are
/// owned by the VIA, invoked synchronously on the mutating caller's thread,
/// and callback exceptions propagate; no background thread invokes them.
class VIA6522 {
  public:
    /// Provider for the external logic level on an eight-bit port.
    using Input = std::function<std::uint8_t()>;
    /// Observer receiving the output register and data-direction mask.
    using Output = std::function<void(std::uint8_t, std::uint8_t)>;

    /// Restores registers, timers, interrupt flags, and control-line levels.
    void reset();
    /// Reads one of the 16 VIA registers and applies read side effects.
    /// @param reg Register selector; only the low four bits are used.
    /// @return Host-visible register value.
    std::uint8_t read(std::uint8_t reg);
    /// Writes one of the 16 VIA registers and applies write side effects.
    /// @param reg Register selector; only the low four bits are used.
    /// @param value Value written by the host.
    void write(std::uint8_t reg, std::uint8_t value);
    /// Advances timer state on the VIA clock timebase.
    /// @param viaCycles Elapsed VIA cycles.
    void tick(std::uint32_t viaCycles);

    /// Installs the borrowed-behavior provider for port A input pins.
    /// @param input Callback owned by this VIA, or empty for pulled-high input.
    void setPortAInput(Input input) { inputA_ = std::move(input); }
    /// Installs the provider for port B input pins.
    /// @param input Callback owned by this VIA, or empty for pulled-high input.
    void setPortBInput(Input input) { inputB_ = std::move(input); }
    /// Installs the observer for port A output changes.
    /// @param output Callback owned by this VIA, or empty for no observer.
    void setPortAOutput(Output output) { outputA_ = std::move(output); }
    /// Installs the observer for port B output changes.
    /// @param output Callback owned by this VIA, or empty for no observer.
    void setPortBOutput(Output output) { outputB_ = std::move(output); }
    /// Updates CA1 and latches a configured active edge.
    /// @param level New external logic level.
    void setCA1(bool level);
    /// Updates CB1 and latches a configured active edge.
    /// @param level New external logic level.
    void setCB1(bool level);

    /// Reports the aggregate enabled-interrupt output level.
    /// @return `true` when any enabled interrupt flag is set.
    [[nodiscard]] bool irq() const noexcept;
    /// Returns the port A output register without input merging.
    /// @return Port A output latch.
    [[nodiscard]] std::uint8_t outputA() const noexcept { return ora_; }
    /// Returns the port B output register without input merging.
    /// @return Port B output latch.
    [[nodiscard]] std::uint8_t outputB() const noexcept { return orb_; }
    /// Returns the port A data-direction register.
    /// @return Port A direction mask.
    [[nodiscard]] std::uint8_t ddra() const noexcept { return ddra_; }
    /// Returns the port B data-direction register.
    /// @return Port B direction mask.
    [[nodiscard]] std::uint8_t ddrb() const noexcept { return ddrb_; }

  private:
    friend class BBCMicro;

    std::uint8_t orb_ = 0;
    std::uint8_t ora_ = 0;
    std::uint8_t ddrb_ = 0;
    std::uint8_t ddra_ = 0;
    std::uint16_t t1Counter_ = 0xFFFF;
    std::uint16_t t1Latch_ = 0xFFFF;
    std::uint16_t t2Counter_ = 0xFFFF;
    std::uint8_t t2LatchLow_ = 0xFF;
    std::uint8_t shift_ = 0;
    std::uint8_t acr_ = 0;
    std::uint8_t pcr_ = 0;
    std::uint8_t ifr_ = 0;
    std::uint8_t ier_ = 0;
    bool t1Running_ = false;
    bool t2Running_ = false;
    bool ca1_ = true;
    bool cb1_ = true;
    Input inputA_;
    Input inputB_;
    Output outputA_;
    Output outputB_;

    std::uint8_t portAValue() const;
    std::uint8_t portBValue() const;
    void clearIFR(std::uint8_t mask) { ifr_ &= static_cast<std::uint8_t>(~mask); }
    void notifyA();
    void notifyB();
};

} // namespace beeb
