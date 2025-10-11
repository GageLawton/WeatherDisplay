#include "weather.h"
#include <curl/curl.h>
#include <string>
#include <iostream>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

// Updated getWeather with optional rawResponse output parameter
Weather getWeather(const std::string& apiKey, const std::string& city, std::string* rawResponse) {
    CURL* curl;
    CURLcode res;
    std::string readBuffer;

    std::string url = "https://api.openweathermap.org/data/2.5/weather?q=" + city + "&appid=" + apiKey + "&units=metric";

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    if (!curl) {
        std::cerr << "[ERROR] CURL init failed\n";
        return Weather{"N/A", 0.0f};
    }

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

    res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        std::cerr << "[ERROR] CURL request failed: " << curl_easy_strerror(res) << "\n";
        curl_easy_cleanup(curl);
        curl_global_cleanup();
        return Weather{"N/A", 0.0f};
    }

    // Save raw response if requested
    if (rawResponse) {
        *rawResponse = readBuffer;
    }

    curl_easy_cleanup(curl);
    curl_global_cleanup();

    try {
        auto j = json::parse(readBuffer);

        // Safe access with checks to avoid crashes
        std::string description = "N/A";
        float temp = 0.0f;

        if (j.contains("weather") && j["weather"].is_array() && !j["weather"].empty()) {
            if (j["weather"][0].contains("description") && !j["weather"][0]["description"].is_null()) {
                description = j["weather"][0]["description"].get<std::string>();
            }
        }

        if (j.contains("main") && j["main"].contains("temp") && !j["main"]["temp"].is_null()) {
            temp = j["main"]["temp"].get<float>();
        }

        return Weather{description, temp};

    } catch (const json::exception& e) {
        std::cerr << "[ERROR] JSON parsing error: " << e.what() << "\n";
        return Weather{"N/A", 0.0f};
    }
}
