#pragma once

#include <cstdint>
#include <functional>

namespace beeb {

class VIA6522 {
public:
    using Input = std::function<std::uint8_t()>;
    using Output = std::function<void(std::uint8_t, std::uint8_t)>;

    void reset();
    std::uint8_t read(std::uint8_t reg);
    void write(std::uint8_t reg, std::uint8_t value);
    void tick(std::uint32_t viaCycles);

    void setPortAInput(Input input) { inputA_ = std::move(input); }
    void setPortBInput(Input input) { inputB_ = std::move(input); }
    void setPortAOutput(Output output) { outputA_ = std::move(output); }
    void setPortBOutput(Output output) { outputB_ = std::move(output); }
    void setCA1(bool level);
    void setCB1(bool level);

    [[nodiscard]] bool irq() const noexcept;
    [[nodiscard]] std::uint8_t outputA() const noexcept { return ora_; }
    [[nodiscard]] std::uint8_t outputB() const noexcept { return orb_; }
    [[nodiscard]] std::uint8_t ddra() const noexcept { return ddra_; }
    [[nodiscard]] std::uint8_t ddrb() const noexcept { return ddrb_; }

private:
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
