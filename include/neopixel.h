#ifndef NEOPIXEL_H
#define NEOPIXEL_H

#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>

// WS2812 / WS2812B / SK6812 (RGB) NeoPixel driver via Linux SPI.
//
// The WS2812 uses a 1-wire protocol with strict ~0.4us / ~0.85us bit timing.
// Linux on a Pi 5 cannot bit-bang that reliably from userspace, but we can
// trick it into outputting the correct waveform by clocking carefully chosen
// SPI bytes at 6.4 MHz, where each WS2812 "bit" is encoded as 8 SPI bits:
//
//   WS2812 '0' -> 0b11000000  (one high pulse ~0.31us, then low ~1.09us)
//   WS2812 '1' -> 0b11111100  (one high pulse ~0.94us, then low ~0.31us)
//
// 24 bits per LED * 8 SPI bits per WS2812 bit = 24 bytes per LED on the wire.
//
// Build with -DNEOPIXEL_REAL on the Pi (uses /dev/spidev0.0).
// Without it, the mock backend just records the pixel state for tests.

class NeoPixel {
public:
    // Open the SPI device and prepare to drive `count` LEDs.
    // On Mac/mock, the device path is ignored.
    bool begin(int count, const std::string& device = "/dev/spidev0.0");

    void end();

    // ---- Orientation configuration ----------------------------------------
    //
    // Logical-to-physical pixel mapping. Lets us put "logical 0" anywhere
    // on the ring and choose direction without rewiring or re-soldering.
    //
    //   offset:    which physical pixel corresponds to logical pixel 0.
    //              e.g. if the ring's hardware pixel 0 is at 6 o'clock and
    //              we want logical 0 at 12 o'clock on a 16-LED ring, set
    //              offset = 8.
    //
    //   clockwise: true (default) means logical indices increase clockwise
    //              when viewed from the front. false means counterclockwise.
    //
    // setPixel/getPixel accept LOGICAL indices. The mapping is applied
    // inside show() when the framebuffer is encoded for the wire.
    void setOrientation(int offset, bool clockwise);

    int  offset()    const { return offset_; }
    bool clockwise() const { return clockwise_; }

    // ---- Pixel buffer (logical indices) -----------------------------------
    void setPixel(int logicalIndex, uint8_t r, uint8_t g, uint8_t b);
    void fill(uint8_t r, uint8_t g, uint8_t b);
    void clear();

    // Apply a global brightness scale [0, 1] to the next show() call.
    void setBrightness(double b);

    // Push the current framebuffer to the LEDs.
    bool show();

    // ---- Inspection -------------------------------------------------------
    int    count()       const { return count_; }
    double brightness()  const { return brightness_; }

    // Returns the color at a LOGICAL index (post-clear, pre-brightness).
    void getPixel(int logicalIndex, uint8_t& r, uint8_t& g, uint8_t& b) const;

    // Returns the color at a PHYSICAL index. Useful for the visualizer
    // (which renders what the actual hardware ring would look like).
    void getPhysicalPixel(int physicalIndex, uint8_t& r, uint8_t& g, uint8_t& b) const;

    // Map a logical index to its physical index (handy for tests).
    int logicalToPhysical(int logicalIndex) const;

#ifndef NEOPIXEL_REAL
    static size_t mockLastShowBytes();
#endif

private:
    bool    isOpen_     = false;
    int     fd_         = -1;
    int     count_      = 0;
    double  brightness_ = 1.0;
    int     offset_     = 0;
    bool    clockwise_  = true;

    // Framebuffer of (r, g, b) per LOGICAL pixel.
    std::vector<uint8_t> rgb_;

    // Wire buffer of 24 SPI bytes per LED (encoded bits) + a small reset trailer.
    std::vector<uint8_t> wire_;

    bool sendWire();
};

#endif