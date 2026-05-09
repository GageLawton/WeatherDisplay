#ifndef OLED_H
#define OLED_H

#include "i2c_bus.h"
#include <cstdint>
#include <string>

class OLED {
public:
    static constexpr int WIDTH  = 128;
    static constexpr int HEIGHT = 64;
    static constexpr int PAGES  = HEIGHT / 8;       // 8
    static constexpr int FB_SIZE = WIDTH * PAGES;   // 1024 bytes

    // Initialize SSD1306 at given I2C address (typically 0x3C).
    bool begin(int address = 0x3C);
    void end();

    // Framebuffer ops (do not touch the panel until show() is called).
    void clear();
    void setPixel(int x, int y, bool on);
    bool getPixel(int x, int y) const;

    // Draw a string at (x,y) using the 5x7 font, scaled by `scale`.
    // scale=1 -> 5x7,  scale=2 -> 10x14,  scale=3 -> 15x21,  scale=4 -> 20x28.
    // Each glyph also has 1px right padding (so step = (5+1)*scale).
    void drawText(int x, int y, const std::string& s, int scale = 1, bool on = true);

    // Returns the rendered width in pixels for `drawText` with the given scale.
    static int textWidth(const std::string& s, int scale = 1);
    static int textHeight(int scale = 1) { return 7 * scale; }

    // Center a string horizontally on a given y, picking the largest scale
    // that fits within `maxWidth` (default = full width).
    // Returns the scale that was used (1..maxScale), or 0 if even scale=1 doesn't fit.
    int drawTextCenteredFit(int y, const std::string& s,
                            int maxScale = 4, int maxWidth = WIDTH);

    // Push the current framebuffer to the display.
    bool show();

    // Direct access for testing/inspection.
    const uint8_t* framebuffer() const { return fb_; }
    uint8_t*       framebuffer()       { return fb_; }

private:
    bool sendInitSequence();

    I2CBus  bus_;
    uint8_t fb_[FB_SIZE] = {0};
};

#endif