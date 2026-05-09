#ifndef RING_H
#define RING_H

#include "celestial.h"
#include "neopixel.h"
#include <ctime>

// High-level controller: paints the NeoPixel ring based on time of day.
//
// Daytime: one warm-colored LED at the sun's current position around the ring.
// Nighttime: a cool-blue arc representing the moon's illumination, filling
//            from one side based on waxing/waning direction.
//
// The ring's `setOrientation()` should already be configured by the caller.
class RingDisplay {
public:
    // Configure the controller. Does not own the ring.
    // The ring should already be `begin()`-ed and oriented.
    void configure(NeoPixel* ring, const celestial::Observer& obs);

    // Paint the ring for the given moment. Buffers + show().
    bool update(std::time_t now);

private:
    NeoPixel*            ring_ = nullptr;
    celestial::Observer  obs_  = {0, 0};

    void paintDay(std::time_t now);
    void paintNight(std::time_t now);
};

#endif