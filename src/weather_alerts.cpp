#include "weather_alerts.h"
#include "temperature.h"
#include <cmath>
#include <sstream>

namespace weather_alerts {

namespace {

// Lowercase substring search (cheap, ASCII-only).
bool containsCI(const std::string& haystack, const std::string& needle) {
    if (needle.empty()) return true;
    if (haystack.size() < needle.size()) return false;
    for (size_t i = 0; i + needle.size() <= haystack.size(); ++i) {
        bool ok = true;
        for (size_t k = 0; k < needle.size(); ++k) {
            char a = haystack[i + k];
            char b = needle[k];
            if (a >= 'A' && a <= 'Z') a = a - 'A' + 'a';
            if (b >= 'A' && b <= 'Z') b = b - 'A' + 'a';
            if (a != b) { ok = false; break; }
        }
        if (ok) return true;
    }
    return false;
}

const DailyForecast* dayN(const WeatherReport& r, size_t idx) {
    if (idx >= r.forecast.size()) return nullptr;
    return &r.forecast[idx];
}

} // anonymous namespace

AlertSet detect(const WeatherReport& report, const Thresholds& t) {
    AlertSet out;
    if (!report.valid) return out;

    // ---- 1. Severe weather alerts straight from the API ----
    for (const auto& wa : report.alerts) {
        Alert a;
        a.kind = Kind::SevereWeather;
        a.headline = wa.event.empty() ? "Weather Alert" : wa.event;
        a.detail   = wa.headline.empty() ? wa.description : wa.headline;
        out.active.push_back(a);
    }

    const auto* today    = dayN(report, 0);
    const auto* tomorrow = dayN(report, 1);

    // ---- 2. Heavy rain coming (tomorrow) ----
    if (tomorrow && tomorrow->chanceOfRain >= t.heavyRainPercent) {
        Alert a;
        a.kind = Kind::HeavyRainComing;
        std::ostringstream h, d;
        h << "Rain tomorrow " << tomorrow->chanceOfRain << "%";
        d << "Tomorrow: " << tomorrow->chanceOfRain
          << "% chance of rain (" << tomorrow->description << ")";
        a.headline = h.str();
        a.detail   = d.str();
        out.active.push_back(a);
    }

    // ---- 3. Heavy snow coming ----
    if (tomorrow && (tomorrow->willItSnow
                     || containsCI(tomorrow->description, "snow"))) {
        Alert a;
        a.kind = Kind::HeavySnowComing;
        a.headline = "Snow tomorrow";
        a.detail   = "Tomorrow: " + tomorrow->description;
        out.active.push_back(a);
    }

    // ---- 4 & 5. Large temperature swing between today and tomorrow ----
    if (today && tomorrow) {
        float todayHiF    = temperature::cToF(today->highC);
        float tomorrowHiF = temperature::cToF(tomorrow->highC);
        float deltaF      = tomorrowHiF - todayHiF;
        if (std::fabs(deltaF) >= t.bigTempDeltaF) {
            Alert a;
            std::ostringstream h, d;
            if (deltaF < 0) {
                a.kind = Kind::BigTempDrop;
                h << "Cold drop " << static_cast<int>(std::round(-deltaF)) << "F";
                d << "Tomorrow's high is " << static_cast<int>(std::round(-deltaF))
                  << "F colder than today ("
                  << static_cast<int>(std::round(tomorrowHiF)) << "F vs "
                  << static_cast<int>(std::round(todayHiF))    << "F).";
            } else {
                a.kind = Kind::BigTempJump;
                h << "Warm jump +" << static_cast<int>(std::round(deltaF)) << "F";
                d << "Tomorrow's high is " << static_cast<int>(std::round(deltaF))
                  << "F warmer than today ("
                  << static_cast<int>(std::round(tomorrowHiF)) << "F vs "
                  << static_cast<int>(std::round(todayHiF))    << "F).";
            }
            a.headline = h.str();
            a.detail   = d.str();
            out.active.push_back(a);
        }
    }

    // Already in priority order via the enum sequence we appended.
    return out;
}

std::string keyFor(const Alert& a) {
    // Stable identifier for "have we already alerted on this?" tracking.
    // Combine kind + headline so a "Tornado Warning" today and a "Snow"
    // tomorrow are distinguished, but the same kind of alert repeating
    // every refresh doesn't generate new emails.
    std::ostringstream k;
    switch (a.kind) {
        case Kind::SevereWeather:    k << "severe:"; break;
        case Kind::HeavyRainComing:  k << "rain:";   break;
        case Kind::HeavySnowComing:  k << "snow:";   break;
        case Kind::BigTempDrop:      k << "tempdn:"; break;
        case Kind::BigTempJump:      k << "tempup:"; break;
    }
    k << a.headline;
    return k.str();
}

} // namespace weather_alerts