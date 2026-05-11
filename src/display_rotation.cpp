#include "display_rotation.h"
#include "temperature.h"
#include <algorithm>
#include <cmath>
#include <sstream>

namespace {

constexpr int LCD_WIDTH = 16;

std::string clip(const std::string& s) {
    if (static_cast<int>(s.size()) <= LCD_WIDTH) return s;
    return s.substr(0, LCD_WIDTH);
}

// Round + cast a Celsius temperature to the chosen unit as an int.
int tempInt(float c, const std::string& units) {
    float t = temperature::toUnits(c, units);
    return static_cast<int>(std::lround(t));
}

} // anonymous namespace

void DisplayRotation::configure(const Durations& d, const std::string& units) {
    durations_ = d;
    units_     = units.empty() ? "F" : units;
    if (durations_.currentSec  <= 0) durations_.currentSec  = 12;
    if (durations_.todaySec    <= 0) durations_.todaySec    = 12;
    if (durations_.tomorrowSec <= 0) durations_.tomorrowSec = 12;
    if (durations_.alertSec    <= 0) durations_.alertSec    = 12;
}

void DisplayRotation::setData(const WeatherReport& report,
                               const weather_alerts::AlertSet& alerts,
                               bool isStale) {
    report_ = report;
    alerts_ = alerts;
    stale_  = isStale;
}

DisplayRotation::LcdLines DisplayRotation::tick(std::time_t now) {
    // First call: snap the start time to now.
    if (screenStartedAt_ == 0) {
        screenStartedAt_ = now;
    }

    int elapsed = static_cast<int>(now - screenStartedAt_);
    if (elapsed >= durationFor(currentScreen_)) {
        currentScreen_   = nextScreen(currentScreen_);
        screenStartedAt_ = now;
    }

    return render(currentScreen_);
}

DisplayRotation::Screen DisplayRotation::nextScreen(Screen s) const {
    // Cycle order: Current -> Today -> Tomorrow -> (Alert if any) -> Current
    switch (s) {
        case Screen::Current:  return Screen::Today;
        case Screen::Today:    return Screen::Tomorrow;
        case Screen::Tomorrow: return alerts_.hasAny() ? Screen::Alert : Screen::Current;
        case Screen::Alert:    return Screen::Current;
    }
    return Screen::Current;
}

int DisplayRotation::durationFor(Screen s) const {
    switch (s) {
        case Screen::Current:  return durations_.currentSec;
        case Screen::Today:    return durations_.todaySec;
        case Screen::Tomorrow: return durations_.tomorrowSec;
        case Screen::Alert:    return durations_.alertSec;
    }
    return 12;
}

DisplayRotation::LcdLines DisplayRotation::render(Screen s) const {
    LcdLines out;

    // No valid data at all? Show offline message.
    if (!report_.valid) {
        out.line1 = "Weather offline";
        out.line2 = "  ...retrying";
        return out;
    }

    // Asterisk suffix to denote stale data on weather screens.
    const char* staleMark = stale_ ? "*" : "";

    switch (s) {
        case Screen::Current: {
            std::ostringstream l1;
            l1 << "Temp: " << tempInt(report_.current.tempC, units_)
               << units_ << staleMark;
            out.line1 = clip(l1.str());
            out.line2 = clip(report_.current.description);
            break;
        }

        case Screen::Today: {
            if (report_.forecast.empty()) {
                out.line1 = "Today: no data";
                out.line2 = "";
                break;
            }
            const auto& day = report_.forecast[0];
            std::ostringstream l1, l2;
            l1 << "Today H" << tempInt(day.highC, units_)
               << " L" << tempInt(day.lowC, units_) << staleMark;
            l2 << day.description;
            if (day.chanceOfRain >= 30) {
                l2 << " " << day.chanceOfRain << "%";
            }
            out.line1 = clip(l1.str());
            out.line2 = clip(l2.str());
            break;
        }

        case Screen::Tomorrow: {
            if (report_.forecast.size() < 2) {
                out.line1 = "Tmrw: no data";
                out.line2 = "";
                break;
            }
            const auto& day = report_.forecast[1];
            std::ostringstream l1, l2;
            l1 << "Tmrw H" << tempInt(day.highC, units_)
               << " L" << tempInt(day.lowC, units_) << staleMark;
            l2 << day.description;
            if (day.chanceOfRain >= 30) {
                l2 << " " << day.chanceOfRain << "%";
            }
            out.line1 = clip(l1.str());
            out.line2 = clip(l2.str());
            break;
        }

        case Screen::Alert: {
            const auto* a = alerts_.primary();
            if (!a) {
                // Shouldn't happen since nextScreen() skips Alert when empty,
                // but be defensive.
                out.line1 = "";
                out.line2 = "";
                break;
            }
            out.line1 = "! " + clip(a->headline);
            out.line2 = clip(a->detail);
            break;
        }
    }

    return out;
}