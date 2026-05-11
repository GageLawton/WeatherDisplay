#include "ring.h"
#include <cmath>

namespace {

struct RGB { uint8_t r, g, b; };

// Warm sun palette, walked through as the sun moves across the day.
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

// Alert colors -- one per alert kind. Pulse the whole ring in this color.
RGB colorForAlert(weather_alerts::Kind k) {
    using K = weather_alerts::Kind;
    switch (k) {
        case K::SevereWeather:    return {255,   0,   0}; // red
        case K::HeavyRainComing:  return {  0, 100, 255}; // blue
        case K::HeavySnowComing:  return {255, 255, 255}; // white
        case K::BigTempDrop:      return {  0, 200, 255}; // cyan
        case K::BigTempJump:      return {255, 140,   0}; // orange
    }
    return {255, 255, 255};
}

} // anonymous namespace

void RingDisplay::configure(NeoPixel* ring, const celestial::Observer& obs) {
    ring_ = ring;
    obs_  = obs;
}

void RingDisplay::setAlerts(const weather_alerts::AlertSet& alerts) {
    alerts_ = alerts;
}

bool RingDisplay::update(std::time_t now) {
    if (!ring_) return false;
    ring_->clear();

    if (alerts_.hasAny()) {
        paintAlert(now);
    } else if (celestial::isDaytime(obs_, now)) {
        paintDay(now);
    } else {
        paintNight(now);
    }

    return ring_->show();
}

void RingDisplay::paintDay(std::time_t now) {
    double frac = celestial::sunFractionOfDay(obs_, now);
    if (frac < 0) return;
    int n = ring_->count();
    if (n <= 0) return;

    int idx;
    {
        double pos = (3.0 * n / 4.0) + frac * (n / 2.0);
        while (pos >= n) pos -= n;
        idx = static_cast<int>(std::floor(pos)) % n;
        if (idx < 0) idx += n;
    }

    int paletteIdx = static_cast<int>(frac * SUN_PALETTE_SIZE);
    if (paletteIdx < 0) paletteIdx = 0;
    if (paletteIdx >= SUN_PALETTE_SIZE) paletteIdx = SUN_PALETTE_SIZE - 1;
    RGB c = SUN_PALETTE[paletteIdx];

    ring_->setPixel(idx, c.r, c.g, c.b);
}

void RingDisplay::paintNight(std::time_t now) {
    double illum = celestial::moonIllumination(now);
    bool   wax   = celestial::isMoonWaxing(now);
    int    n     = ring_->count();
    if (n <= 0) return;

    int lit = static_cast<int>(std::round(illum * n));
    if (lit < 0) lit = 0;
    if (lit > n) lit = n;

    for (int i = 0; i < lit; ++i) {
        int idx = wax ? i : (n - 1 - i);
        ring_->setPixel(idx, MOON_COLOR.r, MOON_COLOR.g, MOON_COLOR.b);
    }
}

void RingDisplay::paintAlert(std::time_t now) {
    const auto* a = alerts_.primary();
    if (!a) return;
    RGB c = colorForAlert(a->kind);
    int n = ring_->count();
    if (n <= 0) return;

    // Pulse: dim the whole ring in/out on a ~2-second cycle so it's clearly
    // attention-grabbing without strobing. now's second-level granularity
    // is fine; the ring updates once per minute anyway, so we mostly get
    // a steady brightness that drifts over time.
    constexpr double PERIOD_SEC = 2.0;
    constexpr double MIN_SCALE  = 0.35;
    constexpr double MAX_SCALE  = 1.0;
    double phase = (now % static_cast<std::time_t>(PERIOD_SEC * 100)) / (PERIOD_SEC * 100.0);
    double wave  = 0.5 * (1.0 - std::cos(2.0 * M_PI * phase));
    double scale = MIN_SCALE + (MAX_SCALE - MIN_SCALE) * wave;

    uint8_t r = static_cast<uint8_t>(c.r * scale);
    uint8_t g = static_cast<uint8_t>(c.g * scale);
    uint8_t b = static_cast<uint8_t>(c.b * scale);

    for (int i = 0; i < n; ++i) {
        ring_->setPixel(i, r, g, b);
    }
}