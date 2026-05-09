#include "neopixel.h"

#include <cstdio>
#include <cstring>

#if defined(NEOPIXEL_REAL)
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
#endif

namespace {
#if defined(NEOPIXEL_REAL)
constexpr uint32_t SPI_HZ = 6400000; // 6.4 MHz -> 156.25 ns per SPI bit
constexpr uint8_t  SPI_MODE_VAL = 0;
constexpr uint8_t  SPI_BITS_PER_WORD = 8;
#endif

inline uint8_t encodeBit(bool one) {
    return one ? 0b11111100 : 0b11000000;
}

void encodeByte(uint8_t value, uint8_t* out8) {
    for (int i = 0; i < 8; ++i) {
        bool bit = (value >> (7 - i)) & 0x1;
        out8[i] = encodeBit(bit);
    }
}

inline uint8_t scale(uint8_t c, double b) {
    if (b <= 0) return 0;
    if (b >= 1) return c;
    int v = static_cast<int>(c * b + 0.5);
    if (v < 0) v = 0;
    if (v > 255) v = 255;
    return static_cast<uint8_t>(v);
}
} // anonymous namespace

bool NeoPixel::begin(int count, const std::string& device) {
    if (count <= 0) return false;
    count_ = count;
    rgb_.assign(count * 3, 0);

    constexpr int trailerBytes = 64;
    wire_.assign(count * 24 + trailerBytes, 0);

#if defined(NEOPIXEL_REAL)
    fd_ = ::open(device.c_str(), O_RDWR);
    if (fd_ < 0) {
        std::perror(device.c_str());
        return false;
    }
    uint8_t  mode  = SPI_MODE_VAL;
    uint8_t  bits  = SPI_BITS_PER_WORD;
    uint32_t speed = SPI_HZ;
    if (ioctl(fd_, SPI_IOC_WR_MODE,          &mode)  < 0) { std::perror("SPI_IOC_WR_MODE"); return false; }
    if (ioctl(fd_, SPI_IOC_WR_BITS_PER_WORD, &bits)  < 0) { std::perror("SPI_IOC_WR_BITS_PER_WORD"); return false; }
    if (ioctl(fd_, SPI_IOC_WR_MAX_SPEED_HZ,  &speed) < 0) { std::perror("SPI_IOC_WR_MAX_SPEED_HZ"); return false; }
    isOpen_ = true;
#else
    (void)device;
    fd_ = 1;
    isOpen_ = true;
#endif

    return true;
}

void NeoPixel::end() {
#if defined(NEOPIXEL_REAL)
    if (fd_ >= 0) ::close(fd_);
#endif
    fd_ = -1;
    isOpen_ = false;
}

void NeoPixel::setOrientation(int offset, bool clockwise) {
    // Normalize offset into [0, count_)
    if (count_ > 0) {
        offset_ = ((offset % count_) + count_) % count_;
    } else {
        offset_ = 0;
    }
    clockwise_ = clockwise;
}

int NeoPixel::logicalToPhysical(int logicalIndex) const {
    if (count_ <= 0) return 0;
    int idx = logicalIndex % count_;
    if (idx < 0) idx += count_;
    int phys = clockwise_
        ? (offset_ + idx) % count_
        : (offset_ - idx + count_ * 2) % count_; // +count_*2 keeps it non-negative for any reasonable idx
    return phys;
}

void NeoPixel::setPixel(int logicalIndex, uint8_t r, uint8_t g, uint8_t b) {
    if (logicalIndex < 0 || logicalIndex >= count_) return;
    rgb_[logicalIndex * 3 + 0] = r;
    rgb_[logicalIndex * 3 + 1] = g;
    rgb_[logicalIndex * 3 + 2] = b;
}

void NeoPixel::fill(uint8_t r, uint8_t g, uint8_t b) {
    for (int i = 0; i < count_; ++i) setPixel(i, r, g, b);
}

void NeoPixel::clear() {
    std::fill(rgb_.begin(), rgb_.end(), 0);
}

void NeoPixel::setBrightness(double b) {
    if (b < 0) b = 0;
    if (b > 1) b = 1;
    brightness_ = b;
}

void NeoPixel::getPixel(int logicalIndex, uint8_t& r, uint8_t& g, uint8_t& b) const {
    if (logicalIndex < 0 || logicalIndex >= count_) { r = g = b = 0; return; }
    r = rgb_[logicalIndex * 3 + 0];
    g = rgb_[logicalIndex * 3 + 1];
    b = rgb_[logicalIndex * 3 + 2];
}

void NeoPixel::getPhysicalPixel(int physicalIndex, uint8_t& r, uint8_t& g, uint8_t& b) const {
    // Find which logical index maps to this physical index.
    // For small counts (16), a linear search is fine.
    for (int i = 0; i < count_; ++i) {
        if (logicalToPhysical(i) == physicalIndex) {
            getPixel(i, r, g, b);
            return;
        }
    }
    r = g = b = 0;
}

bool NeoPixel::show() {
    if (!isOpen_) return false;

    // Encode each PHYSICAL LED slot in wire order, applying logical->physical mapping.
    // wire_[physical * 24 .. physical * 24 + 23] holds that physical LED's encoded bytes.
    for (int phys = 0; phys < count_; ++phys) {
        // Find the logical index whose color goes into this physical slot.
        // Inverse map: which logical index produces this physical?
        // Easier: invert the orientation transform.
        int logical;
        if (clockwise_) {
            logical = ((phys - offset_) % count_ + count_) % count_;
        } else {
            logical = ((offset_ - phys) % count_ + count_) % count_;
        }

        uint8_t r = scale(rgb_[logical * 3 + 0], brightness_);
        uint8_t g = scale(rgb_[logical * 3 + 1], brightness_);
        uint8_t b = scale(rgb_[logical * 3 + 2], brightness_);
        uint8_t* out = &wire_[phys * 24];
        encodeByte(g, out + 0);   // green first
        encodeByte(r, out + 8);   // red
        encodeByte(b, out + 16);  // blue
    }

    return sendWire();
}

#if defined(NEOPIXEL_REAL)
bool NeoPixel::sendWire() {
    spi_ioc_transfer xfer{};
    xfer.tx_buf        = reinterpret_cast<__u64>(wire_.data());
    xfer.rx_buf        = 0;
    xfer.len           = wire_.size();
    xfer.speed_hz      = SPI_HZ;
    xfer.bits_per_word = SPI_BITS_PER_WORD;
    xfer.delay_usecs   = 0;
    int ret = ioctl(fd_, SPI_IOC_MESSAGE(1), &xfer);
    if (ret < 0) {
        std::perror("SPI_IOC_MESSAGE");
        return false;
    }
    return true;
}
#else
namespace { size_t g_lastShowBytes = 0; }
bool NeoPixel::sendWire() {
    g_lastShowBytes = wire_.size();
    return true;
}
size_t NeoPixel::mockLastShowBytes() { return g_lastShowBytes; }
#endif