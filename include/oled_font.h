#ifndef OLED_FONT_H
#define OLED_FONT_H

#include <cstdint>

// 5x7 ASCII font. One glyph = 5 columns, one byte per column.
// LSB of each byte = top pixel. Bit 6 = bottom pixel of 7-row glyph.
// Covers 0x20 (space) through 0x7E (~). 95 glyphs * 5 bytes = 475 bytes.
namespace oled_font {

constexpr int GLYPH_WIDTH  = 5;
constexpr int GLYPH_HEIGHT = 7;
constexpr int FIRST_CHAR   = 0x20;
constexpr int LAST_CHAR    = 0x7E;

extern const uint8_t glyphs[(LAST_CHAR - FIRST_CHAR + 1) * GLYPH_WIDTH];

inline const uint8_t* glyphFor(char c) {
    if (c < FIRST_CHAR || c > LAST_CHAR) c = '?';
    return &glyphs[(c - FIRST_CHAR) * GLYPH_WIDTH];
}

} // namespace oled_font

#endif