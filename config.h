#pragma once
#include <string>

struct Config {
    std::string apiKey;
    std::string location;
    std::string units;
    int updateInterval;

    void load(const std::string& filePath);
};
