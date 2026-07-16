#pragma once

#include "beeb/bus.hpp"

#include <cstdint>
#include <functional>
#include <string>

namespace beeb {

/// Serializable programmer-visible state for an NMOS 6502.
struct CPUState {
    std::uint8_t a = 0;       ///< Accumulator register.
    std::uint8_t x = 0;       ///< X index register.
    std::uint8_t y = 0;       ///< Y index register.
    std::uint8_t sp = 0;      ///< Stack pointer register.
    std::uint8_t p = 0x24;    ///< Processor status flags.
    std::uint16_t pc = 0;     ///< Program counter.
    std::uint64_t cycles = 0; ///< Total completed CPU cycles.

    /// Compares every programmer-visible register and the completed-cycle count.
    /// Documentation rationale: runtime replay compares owned CPU values without
    /// introducing access to the runtime-owned processor.
    friend bool operator==(const CPUState&, const CPUState&) = default;
};

/// Deterministic instruction-level NMOS 6502 processor.
///
/// The processor borrows its Bus for its entire lifetime. The class is not
/// internally synchronized; one caller must serialize stepping and mutation.
class CPU6502 {
  public:
    /// Bit masks for the processor status register.
    enum Flag : std::uint8_t {
        Carry = 0x01,            ///< Carry or borrow state.
        Zero = 0x02,             ///< Result was zero.
        InterruptDisable = 0x04, ///< Maskable interrupt disable.
        Decimal = 0x08,          ///< Binary-coded decimal arithmetic mode.
        Break = 0x10,            ///< Break marker used in stacked status.
        Unused = 0x20,           ///< Status bit held high by the processor.
        Overflow = 0x40,         ///< Signed arithmetic overflow.
        Negative = 0x80,         ///< Result sign bit.
    };

    /// Observer called before execution with the current state and opcode.
    using TraceCallback = std::function<void(const CPUState&, std::uint8_t)>;

    /// Creates a processor borrowing `bus` for all memory and timing access.
    /// @param bus Bus whose lifetime must exceed this processor's lifetime.
    explicit CPU6502(Bus& bus);

    /// Loads the reset vector and establishes the NMOS reset register state.
    void reset();

    /// Executes one instruction or pending interrupt as one atomic transition.
    /// @return CPU cycles consumed; the same count has been sent to Bus::tick().
    /// @throws std::runtime_error for an unsupported/illegal opcode.
    /// @throws Any exception raised by the trace observer after restoring the
    /// processor's pre-fetch state. Observer callbacks run synchronously on
    /// the stepping thread and must not mutate the processor or bus.
    std::uint32_t step();

    /// Sets the level-sensitive maskable interrupt input.
    /// @param asserted `true` while the IRQ line is asserted.
    void setIRQ(bool asserted) noexcept { irqLine_ = asserted; }

    /// Latches a non-maskable interrupt for the next step.
    void requestNMI() noexcept { nmiPending_ = true; }

    /// Returns a value snapshot of all programmer-visible state.
    /// @return Independent register and cycle-counter snapshot.
    [[nodiscard]] CPUState state() const noexcept;

    /// Replaces programmer-visible state without touching bus or line state.
    /// @param state State and cycle counter to install.
    void setState(const CPUState& state) noexcept;

    /// Tests one status-register flag.
    /// @param f Flag to inspect.
    /// @return Whether the flag is set.
    [[nodiscard]] bool flag(Flag f) const noexcept;

    /// Changes one status-register flag while preserving the unused bit rule.
    /// @param f Flag to change.
    /// @param value Whether the flag is set.
    void setFlag(Flag f, bool value) noexcept;

    /// Replaces the optional pre-instruction trace observer.
    /// @param callback Observer to own, or an empty function to disable tracing.
    void setTraceCallback(TraceCallback callback) { trace_ = std::move(callback); }

  private:
    // The bus is borrowed, never owned, and must outlive this processor. The
    // helpers below are grouped by the hardware boundary they preserve:
    // fetch/read/write are bus cycles; addressing helpers report page crossing
    // so dispatch can apply conditional penalties; push/pull and setNZ/compare
    // maintain stack/flag invariants; adc/sbc and shifts implement NMOS ALU
    // flags. branch and interrupt model extra bus cycles, while finish() is the
    // single timing boundary that advances the bus and committed cycle count.
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
