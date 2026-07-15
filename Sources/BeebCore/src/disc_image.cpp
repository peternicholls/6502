#include "beeb/disc_image.hpp"

#include <algorithm>
#include <limits>

namespace beeb {

bool DiscImage::load(std::span<const std::uint8_t> bytes, Layout layout, bool writable) {
    constexpr std::size_t trackSideBytes = 10 * 256;
    const unsigned sides = layout == Layout::DSD ? 2u : 1u;
    if (bytes.empty() || bytes.size() % (trackSideBytes * sides) != 0) return false;
    const auto tracks = bytes.size() / (trackSideBytes * sides);
    if (tracks == 0 || tracks > 80) return false;
    bytes_.assign(bytes.begin(), bytes.end());
    layout_ = layout;
    tracks_ = static_cast<unsigned>(tracks);
    sides_ = sides;
    writable_ = writable;
    return true;
}

std::size_t DiscImage::sectorOffset(unsigned track, unsigned side, unsigned sector) const {
    if (track >= tracks_ || side >= sides_ || sector >= 10) return std::numeric_limits<std::size_t>::max();
    // DSD files interleave side 0 and side 1 for each physical track.
    return ((static_cast<std::size_t>(track) * sides_ + side) * 10 + sector) * 256;
}

bool DiscImage::readSector(unsigned track, unsigned side, unsigned sector,
                           std::span<std::uint8_t> destination) const {
    const auto offset = sectorOffset(track, side, sector);
    if (offset == std::numeric_limits<std::size_t>::max() || destination.size() > 256) return false;
    std::copy_n(bytes_.begin() + static_cast<std::ptrdiff_t>(offset), destination.size(), destination.begin());
    return true;
}

bool DiscImage::writeSector(unsigned track, unsigned side, unsigned sector,
                            std::span<const std::uint8_t> source) {
    const auto offset = sectorOffset(track, side, sector);
    if (!writable_ || offset == std::numeric_limits<std::size_t>::max() || source.size() > 256) return false;
    std::copy(source.begin(), source.end(), bytes_.begin() + static_cast<std::ptrdiff_t>(offset));
    return true;
}

} // namespace beeb
