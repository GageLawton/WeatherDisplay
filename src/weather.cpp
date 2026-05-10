#include "weather.h"
#include "log.h"
#include <curl/curl.h>
#include <string>
#include <iomanip>
#include <sstream>
#include "json.hpp"

using json = nlohmann::json;

std::string urlEncode(const std::string &value) {
    std::ostringstream escaped;
    escaped.fill('0');
    escaped << std::hex;
    for (char c : value) {
        if (isalnum(static_cast<unsigned char>(c)) || c == '-' || c == '_' || c == '.' || c == '~') {
            escaped << c;
        } else if (c == ' ') {
            escaped << "%20";
        } else {
            escaped << '%' << std::setw(2) << int((unsigned char)c);
        }
    }
    return escaped.str();
}

static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

Weather getWeather(const std::string& apiKey, const std::string& city, std::string* rawResponse) {
    CURL* curl;
    CURLcode res;
    std::string readBuffer;

    std::string url = "https://api.weatherapi.com/v1/current.json?key=" + apiKey + "&q=" + urlEncode(city);

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    if (!curl) {
        LOG_ERROR("CURL init failed");
        curl_global_cleanup();
        return Weather{"N/A", 0.0f};
    }

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
    curl_easy_setopt(curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

    res = curl_easy_perform(curl);

    if (res != CURLE_OK) {
        LOG_ERROR("CURL request failed: " << curl_easy_strerror(res));
        curl_easy_cleanup(curl);
        curl_global_cleanup();
        return Weather{"N/A", 0.0f};
    }

    if (rawResponse) {
        *rawResponse = readBuffer;
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
            LOG_ERROR("'current' field missing in API response");
        }

        return Weather{description, temp};
    } catch (const json::exception& e) {
        LOG_ERROR("JSON parsing error: " << e.what());
        return Weather{"N/A", 0.0f};
    }
}