#ifndef DISPLAY_ROTATION_H
#define DISPLAY_ROTATION_H

#include "weather.h"
#include "weather_alerts.h"
#include <ctime>
#include <string>

// Manages what gets shown on the 16x2 LCD. Cycles through up to four
// screens:
//   1. CURRENT  - "Temp: 71F" / "Partly cloudy"     (always shown)
//   2. TODAY    - "Today H75 L52" / description     (always shown)
//   3. TOMORROW - "Tmrw H72 L48" / description      (always shown)
//   4. ALERT    - "! ALERT HEADLINE !" / detail     (shown only when active)
//
// Each screen has its own configurable display duration (seconds).
//
// The rotation indicates "stale data" by appending an asterisk to the
// values on the CURRENT / TODAY / TOMORROW screens when isStale is true.
// When there's a full outage (no valid report at all), the rotation
// shows an "OFFLINE" message in place of weather data.
//
// Each `tick()` call returns the LCD lines to display right now.
class DisplayRotation {
public:
    struct Durations {
        int currentSec  = 12;
        int todaySec    = 12;
        int tomorrowSec = 12;
        int alertSec    = 12;
    };

    struct LcdLines {
        std::string line1;
        std::string line2;
    };

    void configure(const Durations& d, const std::string& units);

    // Update the data the rotation will display. Called when the
    // weather cache has new content (or when staleness/alerts change).
    void setData(const WeatherReport& report,
                 const weather_alerts::AlertSet& alerts,
                 bool isStale);

    // Compute the lines to show right now. Advances the rotation timer
    // internally based on `now`.
    LcdLines tick(std::time_t now);

private:
    enum class Screen { Current, Today, Tomorrow, Alert };

    Durations               durations_;
    std::string             units_     = "F";
    WeatherReport           report_;
    weather_alerts::AlertSet alerts_;
    bool                    stale_     = false;

    Screen                  currentScreen_     = Screen::Current;
    std::time_t             screenStartedAt_   = 0;

    // Build the lines for a given screen using the current data.
    LcdLines render(Screen s) const;

    // Advance to the next screen in the cycle (skipping Alert when none).
    Screen nextScreen(Screen s) const;

    // How long this particular screen should be visible.
    int durationFor(Screen s) const;
};

#endif