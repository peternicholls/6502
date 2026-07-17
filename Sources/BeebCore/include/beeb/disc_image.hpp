#pragma once

// C0-DOC-RATIONALE: docs/code/evidence-and-testing.md owns fixture provenance.

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace beeb {

/// Validated, memory-owned DFS sector image.
///
/// Loading copies the supplied bytes. Returned spans borrow this object's
/// storage and remain valid until the next successful load or destruction.
/// The image is not synchronized; one owner must serialize loads and sector
/// access (readers may reuse it only when no load is in flight).
class DiscImage {
  public:
    /// On-disk ordering of the image's sides.
    enum class Layout {
        SSD, ///< One side stored track by track.
        DSD, ///< Two sides interleaved for each physical track.
    };

    /// Validates and copies an image containing 1...80 DFS tracks.
    /// @param bytes Complete image bytes; the span is not retained.
    /// @param layout Single- or double-sided byte ordering.
    /// @param writable Whether writeSector() may mutate the private copy.
    /// @return `true` when the size/layout is valid and the copy was installed.
    bool load(std::span<const std::uint8_t> bytes, Layout layout, bool writable = false);
    /// Reports whether a valid image is loaded.
    /// @return `true` when image storage is present.
    [[nodiscard]] bool present() const noexcept { return !bytes_.empty(); }
    /// Reports whether sector writes are permitted.
    /// @return Current write-permission state.
    [[nodiscard]] bool writable() const noexcept { return writable_; }
    /// Returns the validated physical track count.
    /// @return Track count, or zero with no image.
    [[nodiscard]] unsigned tracks() const noexcept { return tracks_; }
    /// Returns one for SSD and two for DSD.
    /// @return Side count for the current image layout.
    [[nodiscard]] unsigned sides() const noexcept { return sides_; }
    /// Returns the DFS geometry constant of ten sectors per track/side.
    /// @return Ten.
    [[nodiscard]] unsigned sectorsPerTrack() const noexcept { return 10; }

    /// Copies up to one sector from the image.
    /// @param track Zero-based physical track.
    /// @param side Zero-based side.
    /// @param sector Zero-based sector in the range 0...9.
    /// @param destination Writable destination of at most 256 bytes.
    /// @return `false` for invalid coordinates or an oversized destination.
    bool readSector(unsigned track, unsigned side, unsigned sector,
                    std::span<std::uint8_t> destination) const;
    /// Copies up to one sector into a writable image.
    /// @param track Zero-based physical track.
    /// @param side Zero-based side.
    /// @param sector Zero-based sector in the range 0...9.
    /// @param source Source of at most 256 bytes; the span is not retained.
    /// @return `false` when read-only, out of range, or oversized.
    bool writeSector(unsigned track, unsigned side, unsigned sector,
                     std::span<const std::uint8_t> source);
    /// Borrows the complete current image bytes.
    /// @return Read-only span valid until successful load or destruction.
    [[nodiscard]] std::span<const std::uint8_t> bytes() const noexcept { return bytes_; }

  private:
    friend class BBCMicro;

    std::vector<std::uint8_t> bytes_;
    Layout layout_ = Layout::SSD;
    unsigned tracks_ = 0;
    unsigned sides_ = 1;
    bool writable_ = false;

    [[nodiscard]] std::size_t sectorOffset(unsigned track, unsigned side, unsigned sector) const;
};

} // namespace beeb
