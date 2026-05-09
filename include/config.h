// config.h
#pragma once
#include <string>

struct Config {
    // Weather
    std::string apiKey;
    std::string location;
    std::string units;
    int updateInterval;     // seconds between weather refreshes

    // OLED display
    std::string oledFormat;  // e.g. "HH:MM:SS", "HH:MM"
    std::string oledScale;   // "auto" or a number ("1".."4")
    int         oledI2CAddr; // typically 0x3C

    // Celestial observer (used by the LED ring)
    double      latitude;
    double      longitude;

    // LED ring
    int         ledCount;        // number of pixels on the ring
    double      ledBrightness;   // 0.0 - 1.0
    int         ledOffset;       // logical->physical pixel offset
    bool        ledClockwise;    // true if logical indices go clockwise
    std::string ledSpiDevice;    // typically "/dev/spidev0.0"

    void load(const std::string& filePath);
};