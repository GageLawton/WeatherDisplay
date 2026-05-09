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

    void load(const std::string& filePath);
};