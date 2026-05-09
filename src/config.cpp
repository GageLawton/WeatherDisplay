#include "config.h"
#include <fstream>
#include <cstdlib>
#include <iostream>
#include "json.hpp"

namespace {
    std::string envOrEmpty(const char* name) {
        const char* v = std::getenv(name);
        return v ? std::string(v) : std::string();
    }
}

void Config::load(const std::string& filePath) {
    // 1. Hardcoded defaults.
    apiKey         = "";
    location       = "Westmont, IL";
    units          = "F";
    updateInterval = 600;

    oledFormat     = "HH:MM:SS";
    oledScale      = "auto";
    oledI2CAddr    = 0x3C;

    // Westmont, IL defaults for celestial calc.
    latitude       = 41.7958;
    longitude      = -87.9756;

    ledCount       = 16;
    ledBrightness  = 0.1;
    ledOffset      = 0;
    ledClockwise   = true;
    ledSpiDevice   = "/dev/spidev0.0";

    // 2. Read config.json if present.
    std::ifstream inFile(filePath);
    if (inFile.is_open()) {
        try {
            nlohmann::json j;
            inFile >> j;

            // Top-level flat keys (legacy / GitHub Actions style).
            if (j.contains("WEATHER_API_KEY"))   apiKey         = j["WEATHER_API_KEY"].get<std::string>();
            if (j.contains("WEATHER_LOCATION"))  location       = j["WEATHER_LOCATION"].get<std::string>();
            if (j.contains("UNITS"))             units          = j["UNITS"].get<std::string>();
            if (j.contains("UPDATE_INTERVAL"))   updateInterval = j["UPDATE_INTERVAL"].get<int>();

            // Location block.
            if (j.contains("location")) {
                const auto& loc = j["location"];
                if (loc.contains("latitude"))  latitude  = loc["latitude"].get<double>();
                if (loc.contains("longitude")) longitude = loc["longitude"].get<double>();
            }

            // OLED block.
            if (j.contains("oled")) {
                const auto& o = j["oled"];
                if (o.contains("format")) oledFormat = o["format"].get<std::string>();
                if (o.contains("scale")) {
                    if (o["scale"].is_string())      oledScale = o["scale"].get<std::string>();
                    else if (o["scale"].is_number()) oledScale = std::to_string(o["scale"].get<int>());
                }
                if (o.contains("i2c_address")) {
                    if (o["i2c_address"].is_string())
                        oledI2CAddr = std::stoi(o["i2c_address"].get<std::string>(), nullptr, 0);
                    else if (o["i2c_address"].is_number())
                        oledI2CAddr = o["i2c_address"].get<int>();
                }
            }

            // LED ring block.
            if (j.contains("led")) {
                const auto& l = j["led"];
                if (l.contains("count"))      ledCount      = l["count"].get<int>();
                if (l.contains("brightness")) ledBrightness = l["brightness"].get<double>();
                if (l.contains("offset"))     ledOffset     = l["offset"].get<int>();
                if (l.contains("clockwise"))  ledClockwise  = l["clockwise"].get<bool>();
                if (l.contains("spi_device")) ledSpiDevice  = l["spi_device"].get<std::string>();
            }
        } catch (const std::exception& e) {
            std::cerr << "[WARNING] Failed to parse " << filePath << ": " << e.what() << std::endl;
        }
    }

    // 3. Env vars override.
    {
        std::string s;
        s = envOrEmpty("WEATHER_API_KEY");      if (!s.empty()) apiKey         = s;
        s = envOrEmpty("WEATHER_LOCATION");     if (!s.empty()) location       = s;
        s = envOrEmpty("UNITS");                if (!s.empty()) units          = s;
        s = envOrEmpty("UPDATE_INTERVAL");      if (!s.empty()) updateInterval = std::atoi(s.c_str());
        s = envOrEmpty("OLED_FORMAT");          if (!s.empty()) oledFormat     = s;
        s = envOrEmpty("OLED_SCALE");           if (!s.empty()) oledScale      = s;
    }

    // 4. Sanity guards.
    if (apiKey.empty())         std::cerr << "[WARNING] No WEATHER_API_KEY set; weather will not work." << std::endl;
    if (oledFormat.empty())     oledFormat = "HH:MM:SS";
    if (oledScale.empty())      oledScale  = "auto";
    if (updateInterval <= 0)    updateInterval = 600;
    if (ledCount <= 0)          ledCount = 16;
    if (ledBrightness < 0)      ledBrightness = 0;
    if (ledBrightness > 1)      ledBrightness = 1;
    if (ledSpiDevice.empty())   ledSpiDevice = "/dev/spidev0.0";
}