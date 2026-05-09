#ifndef CELESTIAL_H
#define CELESTIAL_H

#include <ctime>

// Pure-math sunrise/sunset/moon calculations.
// No external libraries needed.
//
// Sunrise/sunset use the standard NOAA algorithm and are accurate to
// within a minute or so for typical latitudes (-66.5 to +66.5 degrees).
//
// Moon phase uses a simple synodic-month approximation. Accurate to ±1 day
// for any phase, which is more than enough to choose how many LEDs to light.

namespace celestial {

// A point in time and space.
struct Observer {
    double latitude;     // degrees, north positive
    double longitude;    // degrees, east positive (Westmont = -87.97...)
};

// Times are returned as Unix epoch seconds (UTC).
// std::localtime() can convert them back to your local zone.
struct SunTimes {
    std::time_t sunrise;
    std::time_t sunset;
    bool        valid;   // false if the sun never rises/sets that day (polar latitudes)
};

// Compute today's sunrise and sunset for the given observer.
// `dayUtc` is any timestamp on the date of interest (UTC midnight is fine).
SunTimes sunTimesForDate(const Observer& obs, std::time_t dayUtc);

// Returns true if `now` falls between sunrise and sunset.
// At polar latitudes where the sun never rises, returns false.
// At polar latitudes where the sun never sets, returns true.
bool isDaytime(const Observer& obs, std::time_t now);

// Sun's progress through the day, 0.0 at sunrise to 1.0 at sunset.
// Returns -1.0 outside daylight hours (or if invalid).
double sunFractionOfDay(const Observer& obs, std::time_t now);

// Moon phase as a fraction of the synodic month [0, 1).
// 0.0 = new moon, 0.5 = full moon, 1.0 = next new moon.
double moonPhaseFraction(std::time_t now);

// Moon illumination [0, 1].
// 0.0 = new (dark), 1.0 = full (bright). Roughly cos-shaped.
double moonIllumination(std::time_t now);

// True if the moon is waxing (growing toward full), false if waning.
bool isMoonWaxing(std::time_t now);

} // namespace celestial

#endif