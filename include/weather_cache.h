#ifndef WEATHER_CACHE_H
#define WEATHER_CACHE_H

#include "weather.h"
#include <ctime>
#include <string>

// Wraps the weather API with retry/backoff and staleness tracking.
//
// Typical usage in the main loop:
//   if (cache.shouldFetchNow(now)) {
//       cache.tryFetch(apiKey, location, now);
//   }
//   const WeatherReport& r = cache.lastReport();
//   bool stale = cache.isStale(now);
//
// On success: lastReport() updates, failure counter resets, next fetch
// happens after the normal refresh interval.
//
// On failure: lastReport() keeps the previous good data (so the LCD
// keeps showing something useful), failure counter increments, and
// the next fetch is scheduled after an exponentially growing backoff.
class WeatherCache {
public:
    // refreshInterval     - seconds between fetches when everything is fine
    // staleThreshold      - if last successful fetch was longer ago than this,
    //                       isStale() returns true (default: 3x refresh, capped 1hr)
    // alertAfterFailures  - how many consecutive failures before alerting
    //                       (the cache itself doesn't alert; this is exposed for callers)
    void configure(int refreshInterval,
                   int staleThreshold,
                   int alertAfterFailures = 6);

    // True when it's time to attempt a fetch (or retry).
    bool shouldFetchNow(std::time_t now) const;

    // Try to fetch a new report. Returns true if the fetch succeeded.
    // On failure, the previous lastReport() is preserved.
    bool tryFetch(const std::string& apiKey,
                  const std::string& location,
                  std::time_t now);

    // The most recent successful report (or an invalid report if we've
    // never had one).
    const WeatherReport& lastReport() const { return lastReport_; }

    // True if the last successful fetch is older than the stale threshold,
    // OR we've never had a successful fetch at all.
    bool isStale(std::time_t now) const;

    // True when we should consider the API "broken" for alerting purposes.
    bool shouldAlert(std::time_t now) const;

    // Stats / introspection
    int  consecutiveFailures() const { return failureCount_; }
    std::time_t lastSuccessTime() const { return lastReport_.fetchedAt; }
    std::time_t nextFetchTime()   const { return nextFetchTime_; }

private:
    int           refreshInterval_   = 600;
    int           staleThreshold_    = 1800;
    int           alertAfterFailures_= 6;

    WeatherReport lastReport_;             // .valid=false until first success
    int           failureCount_      = 0;
    std::time_t   nextFetchTime_     = 0;  // 0 = "fetch immediately"

    // Compute the next-fetch time after a failure (exponential backoff).
    std::time_t computeBackoffUntil(std::time_t now) const;
};

#endif