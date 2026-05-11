#ifndef RING_H
#define RING_H

#include "celestial.h"
#include "neopixel.h"
#include "weather_alerts.h"
#include <ctime>

// High-level controller: paints the NeoPixel ring based on time of day.
//
// Default behavior:
//   Daytime: one warm-colored LED at the sun's current position.
//   Nighttime: cool-blue arc proportional to moon illumination.
//
// Alert override:
//   If alerts are active, the ring temporarily flashes a color
//   corresponding to the highest-priority alert:
//     SevereWeather   -> red
//     HeavyRainComing -> blue
//     HeavySnowComing -> white
//     BigTempDrop     -> cyan
//     BigTempJump     -> orange
//   When alerts clear, the ring returns to normal celestial behavior.
class RingDisplay {
public:
    void configure(NeoPixel* ring, const celestial::Observer& obs);

    // Set or clear the active alert. Pass nullptr/empty to clear.
    void setAlerts(const weather_alerts::AlertSet& alerts);

    // Paint the ring for the given moment. Buffers + show().
    bool update(std::time_t now);

private:
    NeoPixel*                ring_ = nullptr;
    celestial::Observer      obs_  = {0, 0};
    weather_alerts::AlertSet alerts_;

    void paintDay(std::time_t now);
    void paintNight(std::time_t now);
    void paintAlert(std::time_t now);  // overrides day/night
};

#endif