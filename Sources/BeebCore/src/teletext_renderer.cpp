#include "beeb/teletext_renderer.hpp"

#include <algorithm>
#include <cctype>

namespace beeb {

std::array<std::uint8_t, 4> TeletextRenderer::colour(std::uint8_t c) {
    return {
        static_cast<std::uint8_t>((c & 1) ? 255 : 0),
        static_cast<std::uint8_t>((c & 2) ? 255 : 0),
        static_cast<std::uint8_t>((c & 4) ? 255 : 0),
        255
    };
}

void TeletextRenderer::pixel(TeletextBitmap& bitmap, unsigned x, unsigned y,
                             const std::array<std::uint8_t, 4>& rgba) {
    if (x >= bitmap.width || y >= bitmap.height) return;
    const auto offset = (static_cast<std::size_t>(y) * bitmap.width + x) * 4;
    std::copy(rgba.begin(), rgba.end(), bitmap.rgba.begin() + static_cast<std::ptrdiff_t>(offset));
}

std::array<std::uint8_t, 7> TeletextRenderer::glyph(char c) {
    if (c >= 'a' && c <= 'z') c = static_cast<char>(c - 'a' + 'A');
    switch (c) {
        case 'A': return {14,17,17,31,17,17,17}; case 'B': return {30,17,17,30,17,17,30};
        case 'C': return {14,17,16,16,16,17,14}; case 'D': return {30,17,17,17,17,17,30};
        case 'E': return {31,16,16,30,16,16,31}; case 'F': return {31,16,16,30,16,16,16};
        case 'G': return {14,17,16,23,17,17,15}; case 'H': return {17,17,17,31,17,17,17};
        case 'I': return {14,4,4,4,4,4,14};      case 'J': return {7,2,2,2,2,18,12};
        case 'K': return {17,18,20,24,20,18,17}; case 'L': return {16,16,16,16,16,16,31};
        case 'M': return {17,27,21,21,17,17,17}; case 'N': return {17,25,21,19,17,17,17};
        case 'O': return {14,17,17,17,17,17,14}; case 'P': return {30,17,17,30,16,16,16};
        case 'Q': return {14,17,17,17,21,18,13}; case 'R': return {30,17,17,30,20,18,17};
        case 'S': return {15,16,16,14,1,1,30};   case 'T': return {31,4,4,4,4,4,4};
        case 'U': return {17,17,17,17,17,17,14}; case 'V': return {17,17,17,17,17,10,4};
        case 'W': return {17,17,17,21,21,21,10}; case 'X': return {17,17,10,4,10,17,17};
        case 'Y': return {17,17,10,4,4,4,4};     case 'Z': return {31,1,2,4,8,16,31};
        case '0': return {14,17,19,21,25,17,14}; case '1': return {4,12,4,4,4,4,14};
        case '2': return {14,17,1,2,4,8,31};     case '3': return {30,1,1,14,1,1,30};
        case '4': return {2,6,10,18,31,2,2};     case '5': return {31,16,16,30,1,1,30};
        case '6': return {14,16,16,30,17,17,14}; case '7': return {31,1,2,4,8,8,8};
        case '8': return {14,17,17,14,17,17,14}; case '9': return {14,17,17,15,1,1,14};
        case '.': return {0,0,0,0,0,12,12};      case ',': return {0,0,0,0,0,12,8};
        case ':': return {0,12,12,0,12,12,0};    case ';': return {0,12,12,0,12,8,0};
        case '!': return {4,4,4,4,4,0,4};        case '?': return {14,17,1,2,4,0,4};
        case '-': return {0,0,0,31,0,0,0};       case '_': return {0,0,0,0,0,0,31};
        case '+': return {0,4,4,31,4,4,0};       case '=': return {0,0,31,0,31,0,0};
        case '/': return {1,2,2,4,8,8,16};       case '\\': return {16,8,8,4,2,2,1};
        case '(': return {2,4,8,8,8,4,2};        case ')': return {8,4,2,2,2,4,8};
        case '[': return {14,8,8,8,8,8,14};      case ']': return {14,2,2,2,2,2,14};
        case '<': return {2,4,8,16,8,4,2};       case '>': return {8,4,2,1,2,4,8};
        case '*': return {0,17,10,31,10,17,0};   case '#': return {10,10,31,10,31,10,10};
        case '$': return {4,15,20,14,5,30,4};    case '%': return {17,2,4,8,16,17,0};
        case '&': return {12,18,20,8,21,18,13};  case '@': return {14,17,23,21,23,16,14};
        case '\'': return {4,4,8,0,0,0,0};       case '"': return {10,10,20,0,0,0,0};
        case '^': return {4,10,17,0,0,0,0};      case '|': return {4,4,4,4,4,4,4};
        case '`': return {8,4,2,0,0,0,0};        case '~': return {0,0,9,22,0,0,0};
        default: return {0,0,0,0,0,0,0};
    }
}

TeletextBitmap TeletextRenderer::render(std::span<const std::uint8_t> ram,
                                        const CRTC6845& crtc,
                                        std::uint64_t frameNumber) const {
    constexpr unsigned cellWidth = 12;
    constexpr unsigned cellHeight = 20;
    const unsigned columns = std::clamp<unsigned>(crtc.reg(1), 1, 80);
    const unsigned rows = std::clamp<unsigned>(crtc.reg(6), 1, 32);
    TeletextBitmap bitmap;
    bitmap.width = columns * cellWidth;
    bitmap.height = rows * cellHeight;
    bitmap.rgba.assign(static_cast<std::size_t>(bitmap.width) * bitmap.height * 4, 0);
    for (std::size_t index = 3; index < bitmap.rgba.size(); index += 4) bitmap.rgba[index] = 255;

    const auto start = static_cast<unsigned>(crtc.displayStart() & 0x03FF);
    for (unsigned row = 0; row < rows; ++row) {
        std::uint8_t foreground = 7;
        std::uint8_t background = 0;
        bool graphics = false;
        bool separated = false;
        bool flash = false;

        for (unsigned columnIndex = 0; columnIndex < columns; ++columnIndex) {
            const auto address = 0x7C00u | ((start + row * columns + columnIndex) & 0x03FFu);
            const auto character = address < ram.size() ? ram[address] & 0x7F : 0x20;
            if (character < 0x20) {
                if (character >= 0x01 && character <= 0x07) { foreground = character; graphics = false; }
                else if (character == 0x08) flash = true;
                else if (character == 0x09) flash = false;
                else if (character >= 0x11 && character <= 0x17) { foreground = character - 0x10; graphics = true; }
                else if (character == 0x19) separated = false;
                else if (character == 0x1A) separated = true;
                else if (character == 0x1C) background = 0;
                else if (character == 0x1D) background = foreground;
                continue;
            }

            const auto fg = colour(foreground);
            const auto bg = colour(background);
            const bool visible = !flash || ((frameNumber / 25) & 1) == 0;
            const auto originX = columnIndex * cellWidth;
            const auto originY = row * cellHeight;
            for (unsigned y = 0; y < cellHeight; ++y) {
                for (unsigned x = 0; x < cellWidth; ++x) pixel(bitmap, originX + x, originY + y, bg);
            }
            if (!visible) continue;

            if (graphics) {
                const unsigned mosaic = character & 0x3F;
                for (unsigned blockY = 0; blockY < 3; ++blockY) {
                    for (unsigned blockX = 0; blockX < 2; ++blockX) {
                        const unsigned bit = blockY * 2 + blockX;
                        if ((mosaic & (1u << bit)) == 0) continue;
                        const unsigned gap = separated ? 1 : 0;
                        for (unsigned y = blockY * 6 + gap; y < (blockY + 1) * 6 - gap; ++y) {
                            for (unsigned x = blockX * 6 + gap; x < (blockX + 1) * 6 - gap; ++x) {
                                pixel(bitmap, originX + x, originY + y + 1, fg);
                            }
                        }
                    }
                }
            } else {
                const auto rows5x7 = glyph(static_cast<char>(character));
                for (unsigned glyphY = 0; glyphY < 7; ++glyphY) {
                    for (unsigned glyphX = 0; glyphX < 5; ++glyphX) {
                        if ((rows5x7[glyphY] & (1u << (4 - glyphX))) == 0) continue;
                        for (unsigned sy = 0; sy < 2; ++sy) {
                            for (unsigned sx = 0; sx < 2; ++sx) {
                                pixel(bitmap, originX + 1 + glyphX * 2 + sx,
                                      originY + 3 + glyphY * 2 + sy, fg);
                            }
                        }
                    }
                }
            }
        }
    }
    return bitmap;
}

} // namespace beeb
