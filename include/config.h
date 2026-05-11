// config.h
#pragma once
#include <string>

struct Config {
    // ---- Weather ----
    std::string apiKey;
    std::string location;
    std::string units;
    int updateInterval;        // seconds between weather refreshes

    // ---- Weather cache / alerting ----
    int staleThresholdSec;     // mark data stale after this many seconds
    int alertAfterFailures;    // consecutive fetch failures before emailing

    // ---- Alert thresholds ----
    int   heavyRainPercent;    // tomorrow's chance-of-rain >= this -> alert
    float bigTempDeltaF;       // |tomorrow.high - today.high| in F -> alert

    // ---- Display rotation (seconds per screen) ----
    int rotationCurrentSec;
    int rotationTodaySec;
    int rotationTomorrowSec;
    int rotationAlertSec;

    // ---- OLED display ----
    std::string oledFormat;
    std::string oledScale;
    int         oledI2CAddr;

    // ---- Celestial observer ----
    double      latitude;
    double      longitude;

    // ---- LED ring ----
    int         ledCount;
    double      ledBrightness;
    int         ledOffset;
    bool        ledClockwise;
    std::string ledSpiDevice;

    void load(const std::string& filePath);
};