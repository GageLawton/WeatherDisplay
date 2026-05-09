// Simple test: prints today's sunrise/sunset and moon phase for Westmont.
// Compare against timeanddate.com or similar to validate.

#include "celestial.h"
#include <cstdio>
#include <ctime>

static void printTime(const char* label, std::time_t t) {
    std::tm tm = *std::localtime(&t);
    char buf[64];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S %Z", &tm);
    std::printf("  %-12s %s\n", label, buf);
}

int main() {
    // Westmont, IL
    celestial::Observer westmont{41.7958, -87.9756};

    std::time_t now = std::time(nullptr);
    std::printf("Now (local): ");
    {
        std::tm tm = *std::localtime(&now);
        char buf[64];
        std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S %Z", &tm);
        std::printf("%s\n\n", buf);
    }

    auto suns = celestial::sunTimesForDate(westmont, now);
    std::printf("Sun (Westmont):\n");
    if (!suns.valid) {
        std::printf("  No sunrise/sunset (polar?).\n");
    } else {
        printTime("Sunrise:", suns.sunrise);
        printTime("Sunset:",  suns.sunset);
        bool day = celestial::isDaytime(westmont, now);
        std::printf("  Currently:   %s\n", day ? "DAY" : "NIGHT");
        if (day) {
            double f = celestial::sunFractionOfDay(westmont, now);
            std::printf("  Sun progress: %.1f%% through the day\n", f * 100.0);
        }
    }

    std::printf("\nMoon:\n");
    double phase = celestial::moonPhaseFraction(now);
    double illum = celestial::moonIllumination(now);
    bool   wax   = celestial::isMoonWaxing(now);
    std::printf("  Phase fraction: %.3f (0=new, 0.5=full)\n", phase);
    std::printf("  Illumination:   %.1f%%\n", illum * 100.0);
    std::printf("  Direction:      %s\n", wax ? "waxing" : "waning");

    return 0;
}