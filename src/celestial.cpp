#include "celestial.h"
#include <cmath>
#include <ctime>

namespace celestial {

namespace {

constexpr double PI       = 3.14159265358979323846;
constexpr double DEG2RAD  = PI / 180.0;
constexpr double RAD2DEG  = 180.0 / PI;

// Helper: get UTC midnight (Unix epoch seconds) of the same calendar date as `t`.
std::time_t utcMidnight(std::time_t t) {
    std::tm gm = *std::gmtime(&t);
    gm.tm_hour = 0;
    gm.tm_min  = 0;
    gm.tm_sec  = 0;
    return timegm(&gm);
}

// Helper: Julian day for midnight of the UTC date containing `t`.
double julianDayUtcMidnight(std::time_t t) {
    return 2440587.5 + utcMidnight(t) / 86400.0;
}

// Sunrise/sunset using NOAA's Solar Calculator algorithm.
// Returns Unix epoch seconds of the event, or -1 if no rise/set occurs.
//
// Convention used here:
//   lat: degrees, north positive
//   lon: degrees, EAST positive (Westmont longitude is negative)
double sunEvent(double lat, double lon, std::time_t t, bool rising) {
    // Julian day at 00:00 UT of the requested date.
    double jdMidnight = julianDayUtcMidnight(t);

    // We'll iterate twice for accuracy: first pass uses noon UTC as the
    // estimate; second pass refines using the result of the first.
    // For a single pass, use 0.5 (= noon UT) as the time fraction.
    double timeFraction = 0.5;

    double sunriseSec = -1, sunsetSec = -1;
    for (int iter = 0; iter < 2; ++iter) {
        double jd = jdMidnight + timeFraction;

        // Time in centuries since J2000.0.
        double T = (jd - 2451545.0) / 36525.0;

        // Mean longitude of the sun (degrees), normalized to [0, 360).
        double L0 = std::fmod(280.46646 + T * (36000.76983 + T * 0.0003032), 360.0);
        if (L0 < 0) L0 += 360.0;

        // Mean anomaly of the sun (degrees).
        double M = 357.52911 + T * (35999.05029 - 0.0001537 * T);

        // Equation of center.
        double Mrad = M * DEG2RAD;
        double C = std::sin(Mrad)        * (1.914602 - T * (0.004817 + 0.000014 * T))
                 + std::sin(2 * Mrad)    * (0.019993 - 0.000101 * T)
                 + std::sin(3 * Mrad)    * 0.000289;

        // True longitude.
        double trueLong = L0 + C;

        // Apparent longitude.
        double omega = 125.04 - 1934.136 * T;
        double appLong = trueLong - 0.00569 - 0.00478 * std::sin(omega * DEG2RAD);

        // Obliquity of the ecliptic (corrected).
        double eps0 = 23.0 + (26.0 + (21.448 - T*(46.8150 + T*(0.00059 - T*0.001813))) / 60.0) / 60.0;
        double eps  = eps0 + 0.00256 * std::cos(omega * DEG2RAD);

        // Declination.
        double sinDec = std::sin(eps * DEG2RAD) * std::sin(appLong * DEG2RAD);
        double dec    = std::asin(sinDec);

        // Equation of time (in minutes).
        double y = std::tan(eps * DEG2RAD / 2.0);
        y = y * y;
        double L0rad = L0 * DEG2RAD;
        double eccentEarth = 0.016708634 - T * (0.000042037 + 0.0000001267 * T);
        double eqTime = 4.0 * RAD2DEG * (
              y * std::sin(2 * L0rad)
            - 2 * eccentEarth * std::sin(Mrad)
            + 4 * eccentEarth * y * std::sin(Mrad) * std::cos(2 * L0rad)
            - 0.5 * y * y * std::sin(4 * L0rad)
            - 1.25 * eccentEarth * eccentEarth * std::sin(2 * Mrad)
        );

        // Hour angle for sunrise/sunset (degrees).
        double cosHA = (std::cos(90.833 * DEG2RAD)
                      - std::sin(lat * DEG2RAD) * sinDec)
                     / (std::cos(lat * DEG2RAD) * std::cos(dec));
        if (cosHA > 1.0)  return -1.0; // sun doesn't rise
        if (cosHA < -1.0) return -1.0; // sun doesn't set
        double HA = std::acos(cosHA) * RAD2DEG; // morning HA, positive

        // Solar noon in minutes from UTC midnight.
        // EAST positive longitude, so add 4*lon.
        double solarNoonMin = 720.0 - 4.0 * lon - eqTime;

        // Event time in minutes from UTC midnight.
        double eventMin = rising
            ? solarNoonMin - 4.0 * HA
            : solarNoonMin + 4.0 * HA;

        // Update time fraction for next iteration.
        timeFraction = eventMin / (24.0 * 60.0);

        if (rising) sunriseSec = eventMin * 60.0;
        else        sunsetSec  = eventMin * 60.0;
    }

    double minSec = rising ? sunriseSec : sunsetSec;

    // Result is seconds since UTC midnight of the requested date.
    // Convert to absolute Unix epoch.
    std::time_t midnight = utcMidnight(t);
    return midnight + minSec;
}

} // anonymous namespace

SunTimes sunTimesForDate(const Observer& obs, std::time_t dayUtc) {
    SunTimes out;
    double rise = sunEvent(obs.latitude, obs.longitude, dayUtc, true);
    double set  = sunEvent(obs.latitude, obs.longitude, dayUtc, false);
    if (rise < 0 || set < 0) {
        out.valid = false;
        out.sunrise = 0;
        out.sunset  = 0;
    } else {
        out.valid = true;
        out.sunrise = static_cast<std::time_t>(rise);
        out.sunset  = static_cast<std::time_t>(set);
    }
    return out;
}

bool isDaytime(const Observer& obs, std::time_t now) {
    SunTimes t = sunTimesForDate(obs, now);
    if (!t.valid) return false;
    return now >= t.sunrise && now <= t.sunset;
}

double sunFractionOfDay(const Observer& obs, std::time_t now) {
    SunTimes t = sunTimesForDate(obs, now);
    if (!t.valid || now < t.sunrise || now > t.sunset) return -1.0;
    double total   = static_cast<double>(t.sunset - t.sunrise);
    double elapsed = static_cast<double>(now - t.sunrise);
    if (total <= 0) return -1.0;
    double f = elapsed / total;
    if (f < 0.0) f = 0.0;
    if (f > 1.0) f = 1.0;
    return f;
}

double moonPhaseFraction(std::time_t now) {
    // Synodic month: 29.530588853 days.
    // Reference new moon: 2000-01-06 18:14 UTC = Unix 947182440.
    constexpr double SYNODIC = 29.530588853 * 86400.0;
    constexpr double REF_NEW_MOON = 947182440.0;

    double diff = static_cast<double>(now) - REF_NEW_MOON;
    double phase = std::fmod(diff, SYNODIC) / SYNODIC;
    if (phase < 0) phase += 1.0;
    return phase;
}

double moonIllumination(std::time_t now) {
    double phase = moonPhaseFraction(now);
    return (1.0 - std::cos(2.0 * PI * phase)) / 2.0;
}

bool isMoonWaxing(std::time_t now) {
    return moonPhaseFraction(now) < 0.5;
}

} // namespace celestial