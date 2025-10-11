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

Weather getWeather(const std::string& apiKey, const std::string& city, std::string* rawResponse) {
    CURL* curl;
    CURLcode res;
    std::string readBuffer;

    std::string url = "https://api.weatherapi.com/v1/current.json?key=" + apiKey + "&q=" + city;

    std::cout << "[INFO] Requesting URL: " << url << std::endl;

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

    if (rawResponse) {
        *rawResponse = readBuffer;
        std::cout << "[DEBUG] API Response: " << readBuffer << std::endl;
    }

    curl_easy_cleanup(curl);
    curl_global_cleanup();

    try {
        auto j = json::parse(readBuffer);

        std::string description = "N/A";
        float temp = 0.0f;

        if (j.contains("current")) {
            if (j["current"].contains("condition") && j["current"]["condition"].contains("text") && !j["current"]["condition"]["text"].is_null()) {
                description = j["current"]["condition"]["text"].get<std::string>();
            }
            if (j["current"].contains("temp_c") && !j["current"]["temp_c"].is_null()) {
                temp = j["current"]["temp_c"].get<float>();
            }
        } else {
            std::cerr << "[ERROR] 'current' field missing in API response\n";
        }

        return Weather{description, temp};

    } catch (const json::exception& e) {
        std::cerr << "[ERROR] JSON parsing error: " << e.what() << "\n";
        return Weather{"N/A", 0.0f};
    }
}
