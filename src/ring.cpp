#include "ring.h"
#include <cmath>

namespace {

// Warm sun palette, walked through as the sun moves.
// Morning warm yellow -> midday bright yellow -> afternoon orange -> evening red-orange.
struct RGB { uint8_t r, g, b; };

constexpr RGB SUN_PALETTE[] = {
    {255, 204,  60}, // sunrise warm yellow
    {255, 178,  40}, // mid-morning gold
    {255, 153,   0}, // late-morning amber
    {255, 178,  40}, // noon bright
    {255, 153,   0}, // early afternoon
    {255, 102,   0}, // mid afternoon orange
    {255,  85,   0}, // late afternoon
    {255,  69,   0}, // sunset red-orange
};
constexpr int SUN_PALETTE_SIZE = sizeof(SUN_PALETTE) / sizeof(SUN_PALETTE[0]);

constexpr RGB MOON_COLOR = {80, 80, 255};

} // namespace

void RingDisplay::configure(NeoPixel* ring, const celestial::Observer& obs) {
    ring_ = ring;
    obs_  = obs;
}

bool RingDisplay::update(std::time_t now) {
    if (!ring_) return false;
    ring_->clear();

    if (celestial::isDaytime(obs_, now)) {
        paintDay(now);
    } else {
        paintNight(now);
    }
    return ring_->show();
}

void RingDisplay::paintDay(std::time_t now) {
    double frac = celestial::sunFractionOfDay(obs_, now);
    if (frac < 0) return;            // shouldn't happen if isDaytime() returned true
    int n = ring_->count();
    if (n <= 0) return;

    // Sun's logical position around the ring. Logical 0 is "12 o'clock" by convention,
    // so we map sunrise -> ~9 o'clock (left), noon -> 12 o'clock (top), sunset -> ~3 o'clock.
    // Quarter turn before noon and after noon, total span = half the ring across the top.
    //
    // Mapping: sunrise (frac=0) -> logical (3*n/4), midday -> 0, sunset -> n/4.
    // Walking the top arc clockwise from 9 to 12 to 3.
    int idx;
    {
        // Position 0..n smoothly. Use logical 0 == top, increasing clockwise.
        // sunrise = 3n/4 (left), noon = 0 (top), sunset = n/4 (right)
        // i.e. logical_pos = (3n/4 + frac * n/2) mod n
        double pos = (3.0 * n / 4.0) + frac * (n / 2.0);
        // wrap into [0, n)
        while (pos >= n) pos -= n;
        idx = static_cast<int>(std::floor(pos)) % n;
        if (idx < 0) idx += n;
    }

    // Pick a palette color based on sun fraction.
    int paletteIdx = static_cast<int>(frac * SUN_PALETTE_SIZE);
    if (paletteIdx < 0) paletteIdx = 0;
    if (paletteIdx >= SUN_PALETTE_SIZE) paletteIdx = SUN_PALETTE_SIZE - 1;
    RGB c = SUN_PALETTE[paletteIdx];

    ring_->setPixel(idx, c.r, c.g, c.b);
}

void RingDisplay::paintNight(std::time_t now) {
    double illum = celestial::moonIllumination(now);  // 0..1
    bool   wax   = celestial::isMoonWaxing(now);
    int    n     = ring_->count();
    if (n <= 0) return;

    // How many LEDs to light, proportional to illumination.
    // At full moon (illum=1.0), light all n. At new moon, light none.
    int lit = static_cast<int>(std::round(illum * n));
    if (lit < 0) lit = 0;
    if (lit > n) lit = n;

    // Fill from one side based on direction.
    // Waxing: light "grows" clockwise from logical 0 (top), increasing index.
    // Waning: light "shrinks" counterclockwise (fills from the other side).
    for (int i = 0; i < lit; ++i) {
        int idx = wax ? i : (n - 1 - i);
        ring_->setPixel(idx, MOON_COLOR.r, MOON_COLOR.g, MOON_COLOR.b);
    }
}