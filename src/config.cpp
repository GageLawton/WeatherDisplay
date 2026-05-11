#include "config.h"
#include "log.h"
#include <fstream>
#include <cstdlib>
#include "json.hpp"

namespace {
    std::string envOrEmpty(const char* name) {
        const char* v = std::getenv(name);
        return v ? std::string(v) : std::string();
    }
}

void Config::load(const std::string& filePath) {
    // 1. Hardcoded defaults.
    apiKey               = "";
    location             = "Westmont, IL";
    units                = "F";
    updateInterval       = 600;

    staleThresholdSec    = 1800;   // 30 min by default (3x default refresh, capped 1hr)
    alertAfterFailures   = 6;

    heavyRainPercent     = 80;
    bigTempDeltaF        = 15.0f;

    rotationCurrentSec   = 12;
    rotationTodaySec     = 12;
    rotationTomorrowSec  = 12;
    rotationAlertSec     = 12;

    oledFormat           = "HH:MM:SS";
    oledScale            = "auto";
    oledI2CAddr          = 0x3C;

    latitude             = 41.7958;
    longitude            = -87.9756;

    ledCount             = 16;
    ledBrightness        = 0.1;
    ledOffset            = 0;
    ledClockwise         = true;
    ledSpiDevice         = "/dev/spidev0.0";

    // 2. Read config.json if present.
    std::ifstream inFile(filePath);
    if (inFile.is_open()) {
        try {
            nlohmann::json j;
            inFile >> j;

            // Top-level legacy keys.
            if (j.contains("WEATHER_API_KEY"))   apiKey         = j["WEATHER_API_KEY"].get<std::string>();
            if (j.contains("WEATHER_LOCATION"))  location       = j["WEATHER_LOCATION"].get<std::string>();
            if (j.contains("UNITS"))             units          = j["UNITS"].get<std::string>();
            if (j.contains("UPDATE_INTERVAL"))   updateInterval = j["UPDATE_INTERVAL"].get<int>();

            // location block (celestial)
            if (j.contains("location")) {
                const auto& loc = j["location"];
                if (loc.contains("latitude"))  latitude  = loc["latitude"].get<double>();
                if (loc.contains("longitude")) longitude = loc["longitude"].get<double>();
            }

            // oled block
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

            // led block
            if (j.contains("led")) {
                const auto& l = j["led"];
                if (l.contains("count"))      ledCount      = l["count"].get<int>();
                if (l.contains("brightness")) ledBrightness = l["brightness"].get<double>();
                if (l.contains("offset"))     ledOffset     = l["offset"].get<int>();
                if (l.contains("clockwise"))  ledClockwise  = l["clockwise"].get<bool>();
                if (l.contains("spi_device")) ledSpiDevice  = l["spi_device"].get<std::string>();
            }

            // weather block (cache/alerting + thresholds + rotation)
            if (j.contains("weather")) {
                const auto& w = j["weather"];
                if (w.contains("stale_threshold_sec"))    staleThresholdSec  = w["stale_threshold_sec"].get<int>();
                if (w.contains("alert_after_failures"))   alertAfterFailures = w["alert_after_failures"].get<int>();
                if (w.contains("heavy_rain_percent"))     heavyRainPercent   = w["heavy_rain_percent"].get<int>();
                if (w.contains("big_temp_delta_f"))       bigTempDeltaF      = w["big_temp_delta_f"].get<float>();

                if (w.contains("rotation")) {
                    const auto& r = w["rotation"];
                    if (r.contains("current_sec"))  rotationCurrentSec  = r["current_sec"].get<int>();
                    if (r.contains("today_sec"))    rotationTodaySec    = r["today_sec"].get<int>();
                    if (r.contains("tomorrow_sec")) rotationTomorrowSec = r["tomorrow_sec"].get<int>();
                    if (r.contains("alert_sec"))    rotationAlertSec    = r["alert_sec"].get<int>();
                }
            }
        } catch (const std::exception& e) {
            LOG_WARN("Failed to parse " << filePath << ": " << e.what());
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
    if (apiKey.empty())              LOG_WARN("No WEATHER_API_KEY set; weather will not work.");
    if (oledFormat.empty())          oledFormat = "HH:MM:SS";
    if (oledScale.empty())           oledScale  = "auto";
    if (updateInterval <= 0)         updateInterval = 600;
    if (ledCount <= 0)               ledCount = 16;
    if (ledBrightness < 0)           ledBrightness = 0;
    if (ledBrightness > 1)           ledBrightness = 1;
    if (ledSpiDevice.empty())        ledSpiDevice = "/dev/spidev0.0";
    if (staleThresholdSec <= 0)      staleThresholdSec = 1800;
    if (alertAfterFailures <= 0)     alertAfterFailures = 6;
    if (heavyRainPercent < 0)        heavyRainPercent = 0;
    if (heavyRainPercent > 100)      heavyRainPercent = 100;
    if (bigTempDeltaF <= 0)          bigTempDeltaF = 15.0f;
    if (rotationCurrentSec  <= 0)    rotationCurrentSec  = 12;
    if (rotationTodaySec    <= 0)    rotationTodaySec    = 12;
    if (rotationTomorrowSec <= 0)    rotationTomorrowSec = 12;
    if (rotationAlertSec    <= 0)    rotationAlertSec    = 12;
}