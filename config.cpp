#include "config.h"
#include <fstream>
#include <cstdlib>
#include <iostream>
#include <nlohmann/json.hpp>

void Config::load(const std::string& filePath) {
    // Start with hardcoded defaults (used only if neither secret nor config exists)
    apiKey = "default_api_key";
    location = "Westmont, IL";
    units = "F";
    updateInterval = 600;

    // 1️⃣ Attempt to use environment variables (GitHub secrets)
    if (const char* env_api = std::getenv("WEATHER_API_KEY")) apiKey = env_api;
    if (const char* env_loc = std::getenv("WEATHER_LOCATION")) location = env_loc;
    if (const char* env_units = std::getenv("UNITS")) units = env_units;
    if (const char* env_interval = std::getenv("UPDATE_INTERVAL")) updateInterval = std::atoi(env_interval);

    // 2️⃣ If env vars not set, try config.json
    std::ifstream inFile(filePath);
    if (inFile.is_open()) {
        try {
            nlohmann::json j;
            inFile >> j;
            if (apiKey.empty() && j.contains("WEATHER_API_KEY")) apiKey = j["WEATHER_API_KEY"];
            if (location.empty() && j.contains("WEATHER_LOCATION")) location = j["WEATHER_LOCATION"];
            if (units.empty() && j.contains("UNITS")) units = j["UNITS"];
            if (updateInterval == 600 && j.contains("UPDATE_INTERVAL")) updateInterval = j["UPDATE_INTERVAL"];
        } catch (const std::exception &e) {
            std::cerr << "[WARNING] Failed to parse config.json: " << e.what() << std::endl;
        }
    }

    // 3️⃣ Final check: ensure no empty values remain
    if (apiKey.empty()) apiKey = "default_api_key";
    if (location.empty()) location = "Westmont, IL";
}
