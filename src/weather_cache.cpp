#include "weather_cache.h"
#include "log.h"
#include <algorithm>

void WeatherCache::configure(int refreshInterval, int staleThreshold, int alertAfterFailures) {
    refreshInterval_    = std::max(1, refreshInterval);
    staleThreshold_     = std::max(1, staleThreshold);
    alertAfterFailures_ = std::max(1, alertAfterFailures);
}

bool WeatherCache::shouldFetchNow(std::time_t now) const {
    // First-ever call (nextFetchTime_ unset) => fetch right away.
    if (nextFetchTime_ == 0) return true;
    return now >= nextFetchTime_;
}

bool WeatherCache::tryFetch(const std::string& apiKey,
                             const std::string& location,
                             std::time_t now) {
    WeatherReport r = getWeatherReport(apiKey, location);
    if (r.valid) {
        lastReport_     = r;
        failureCount_   = 0;
        nextFetchTime_  = now + refreshInterval_;
        return true;
    } else {
        failureCount_++;
        nextFetchTime_ = computeBackoffUntil(now);
        LOG_WARN("Weather fetch failed (consecutive failures: " << failureCount_
                 << "). Next retry in " << (nextFetchTime_ - now) << "s.");
        return false;
    }
}

bool WeatherCache::isStale(std::time_t now) const {
    if (!lastReport_.valid) return true;
    return (now - lastReport_.fetchedAt) > staleThreshold_;
}

bool WeatherCache::shouldAlert(std::time_t now) const {
    // We alert when ALL of these are true:
    //   * we've never succeeded in the recent past (stale data)
    //   * we've actually tried and failed many times in a row
    //
    // The double condition prevents false alarms — e.g. on a brand-new
    // boot where we haven't had time to succeed yet, or on a single
    // transient blip.
    if (failureCount_ < alertAfterFailures_) return false;
    if (!isStale(now)) return false;
    return true;
}

std::time_t WeatherCache::computeBackoffUntil(std::time_t now) const {
    // 30s, 60s, 120s, 240s, ... capped at refreshInterval_.
    // After N failures, delay = min(30 * 2^(N-1), refreshInterval).
    int n = std::max(1, failureCount_);
    int delay = 30;
    for (int i = 1; i < n && delay < refreshInterval_; ++i) {
        delay *= 2;
    }
    if (delay > refreshInterval_) delay = refreshInterval_;
    return now + delay;
}