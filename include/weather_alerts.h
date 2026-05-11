#ifndef WEATHER_ALERTS_H
#define WEATHER_ALERTS_H

#include "weather.h"
#include <string>
#include <vector>

namespace weather_alerts {

// Types of conditions worth flagging. Order matters for priority
// (used when picking the ring color and the interrupt-screen message).
enum class Kind {
    // Most severe first.
    SevereWeather,    // API-reported alert (tornado, severe storm, etc.)
    HeavyRainComing,  // Tomorrow's chance of rain >= heavyRainThreshold
    HeavySnowComing,  // Tomorrow has snow (or significant chance)
    BigTempDrop,      // Tomorrow's high is much lower than today's
    BigTempJump,      // Tomorrow's high is much higher than today's
};

struct Alert {
    Kind        kind;
    std::string headline;   // short, LCD-friendly: "Tornado Warning"
    std::string detail;     // longer, for email body
};

// The set of currently active conditions, plus a quick top-pick for UI.
struct AlertSet {
    std::vector<Alert> active;            // all active conditions
    bool               hasAny() const { return !active.empty(); }
    const Alert*       primary() const {  // highest priority alert, or nullptr
        return active.empty() ? nullptr : &active.front();
    }
};

// Thresholds (configurable via WeatherDisplay config).
struct Thresholds {
    int   heavyRainPercent = 80;   // tomorrow's chanceOfRain >= this -> alert
    float bigTempDeltaF    = 15.0; // |tomorrow.high - today.high| in F -> alert
};

// Inspect the report and return everything that's currently worth alerting on.
// Order in the returned set is by priority (highest first).
AlertSet detect(const WeatherReport& report, const Thresholds& t);

// Stable, short key for an alert (used by Alerter to remember what was
// already sent so we don't email the same thing twice in a row).
std::string keyFor(const Alert& a);

} // namespace weather_alerts

#endif