#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace beeb {

class DiscImage {
public:
    enum class Layout { SSD, DSD };

    bool load(std::span<const std::uint8_t> bytes, Layout layout, bool writable = false);
    [[nodiscard]] bool present() const noexcept { return !bytes_.empty(); }
    [[nodiscard]] bool writable() const noexcept { return writable_; }
    [[nodiscard]] unsigned tracks() const noexcept { return tracks_; }
    [[nodiscard]] unsigned sides() const noexcept { return sides_; }
    [[nodiscard]] unsigned sectorsPerTrack() const noexcept { return 10; }

    bool readSector(unsigned track, unsigned side, unsigned sector, std::span<std::uint8_t> destination) const;
    bool writeSector(unsigned track, unsigned side, unsigned sector, std::span<const std::uint8_t> source);
    [[nodiscard]] std::span<const std::uint8_t> bytes() const noexcept { return bytes_; }

private:
    std::vector<std::uint8_t> bytes_;
    Layout layout_ = Layout::SSD;
    unsigned tracks_ = 0;
    unsigned sides_ = 1;
    bool writable_ = false;

    [[nodiscard]] std::size_t sectorOffset(unsigned track, unsigned side, unsigned sector) const;
};

} // namespace beeb
