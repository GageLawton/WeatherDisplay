#include "oled.h"
#include "oled_font.h"
#include <cstring>

// SSD1306 commands
namespace {
    constexpr uint8_t SSD1306_DISPLAYOFF         = 0xAE;
    constexpr uint8_t SSD1306_DISPLAYON          = 0xAF;
    constexpr uint8_t SSD1306_SETDISPLAYCLOCKDIV = 0xD5;
    constexpr uint8_t SSD1306_SETMULTIPLEX       = 0xA8;
    constexpr uint8_t SSD1306_SETDISPLAYOFFSET   = 0xD3;
    constexpr uint8_t SSD1306_SETSTARTLINE       = 0x40;
    constexpr uint8_t SSD1306_CHARGEPUMP         = 0x8D;
    constexpr uint8_t SSD1306_MEMORYMODE         = 0x20;
    constexpr uint8_t SSD1306_SEGREMAP           = 0xA0;  // |0x01 to flip horiz
    constexpr uint8_t SSD1306_COMSCANDEC         = 0xC8;
    constexpr uint8_t SSD1306_SETCOMPINS         = 0xDA;
    constexpr uint8_t SSD1306_SETCONTRAST        = 0x81;
    constexpr uint8_t SSD1306_SETPRECHARGE       = 0xD9;
    constexpr uint8_t SSD1306_SETVCOMDETECT      = 0xDB;
    constexpr uint8_t SSD1306_DISPLAYALLON_RESUME= 0xA4;
    constexpr uint8_t SSD1306_NORMALDISPLAY      = 0xA6;
    constexpr uint8_t SSD1306_DEACTIVATE_SCROLL  = 0x2E;
    constexpr uint8_t SSD1306_COLUMNADDR         = 0x21;
    constexpr uint8_t SSD1306_PAGEADDR           = 0x22;
}

bool OLED::begin(int address) {
    if (!bus_.open(address)) return false;
    if (!sendInitSequence())  return false;
    clear();
    return show();
}

void OLED::end() { bus_.close(); }

bool OLED::sendInitSequence() {
    const uint8_t init[] = {
        SSD1306_DISPLAYOFF,
        SSD1306_SETDISPLAYCLOCKDIV, 0x80,
        SSD1306_SETMULTIPLEX,       0x3F,        // 64-1
        SSD1306_SETDISPLAYOFFSET,   0x00,
        SSD1306_SETSTARTLINE | 0x0,
        SSD1306_CHARGEPUMP,         0x14,        // enable internal charge pump
        SSD1306_MEMORYMODE,         0x00,        // horizontal addressing
        SSD1306_SEGREMAP | 0x1,                  // flip horizontally
        SSD1306_COMSCANDEC,                      // flip vertically
        SSD1306_SETCOMPINS,         0x12,        // 128x64 layout
        SSD1306_SETCONTRAST,        0xCF,
        SSD1306_SETPRECHARGE,       0xF1,
        SSD1306_SETVCOMDETECT,      0x40,
        SSD1306_DISPLAYALLON_RESUME,
        SSD1306_NORMALDISPLAY,
        SSD1306_DEACTIVATE_SCROLL,
        SSD1306_DISPLAYON,
    };
    for (uint8_t b : init) {
        if (!bus_.writeCommand(b)) return false;
    }
    return true;
}

void OLED::clear() { std::memset(fb_, 0, FB_SIZE); }

void OLED::setPixel(int x, int y, bool on) {
    if (x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT) return;
    const int  page = y / 8;
    const int  bit  = y % 8;
    const int  idx  = page * WIDTH + x;
    if (on) fb_[idx] |=  (uint8_t)(1u << bit);
    else    fb_[idx] &= (uint8_t)~(1u << bit);
}

bool OLED::getPixel(int x, int y) const {
    if (x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT) return false;
    const int idx = (y / 8) * WIDTH + x;
    return (fb_[idx] >> (y % 8)) & 0x1;
}

void OLED::drawText(int x, int y, const std::string& s, int scale, bool on) {
    if (scale < 1) scale = 1;
    int cx = x;
    for (char c : s) {
        const uint8_t* g = oled_font::glyphFor(c);
        for (int col = 0; col < oled_font::GLYPH_WIDTH; ++col) {
            uint8_t bits = g[col];
            for (int row = 0; row < oled_font::GLYPH_HEIGHT; ++row) {
                if (bits & (1u << row)) {
                    // Draw a `scale` x `scale` block per source pixel.
                    int px = cx + col * scale;
                    int py = y  + row * scale;
                    for (int dy = 0; dy < scale; ++dy)
                        for (int dx = 0; dx < scale; ++dx)
                            setPixel(px + dx, py + dy, on);
                }
            }
        }
        cx += (oled_font::GLYPH_WIDTH + 1) * scale;  // 1-col gap between glyphs
    }
}

int OLED::textWidth(const std::string& s, int scale) {
    if (scale < 1) scale = 1;
    if (s.empty()) return 0;
    // N glyphs * 5px + (N-1) gaps * 1px, all scaled.
    int n = (int)s.size();
    return (n * oled_font::GLYPH_WIDTH + (n - 1)) * scale;
}

int OLED::drawTextCenteredFit(int y, const std::string& s, int maxScale, int maxWidth) {
    if (maxScale < 1) maxScale = 1;
    if (maxWidth > WIDTH) maxWidth = WIDTH;
    for (int scale = maxScale; scale >= 1; --scale) {
        int w = textWidth(s, scale);
        if (w <= maxWidth) {
            int x = (WIDTH - w) / 2;
            drawText(x, y, s, scale, true);
            return scale;
        }
    }
    return 0; // didn't fit even at scale 1
}

bool OLED::show() {
    // Set column range 0..127
    if (!bus_.writeCommand(SSD1306_COLUMNADDR)) return false;
    if (!bus_.writeCommand(0))   return false;
    if (!bus_.writeCommand(127)) return false;
    // Set page range 0..7
    if (!bus_.writeCommand(SSD1306_PAGEADDR)) return false;
    if (!bus_.writeCommand(0)) return false;
    if (!bus_.writeCommand(7)) return false;
    // Send framebuffer in chunks (some I2C drivers don't like 1024-byte writes)
    constexpr size_t CHUNK = 16;
    for (size_t off = 0; off < FB_SIZE; off += CHUNK) {
        size_t n = (FB_SIZE - off) < CHUNK ? (FB_SIZE - off) : CHUNK;
        if (!bus_.writeData(fb_ + off, n)) return false;
    }
    return true;
}