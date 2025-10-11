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

struct Weather {
    std::string description;
    double tempC;
};

Weather getWeather(const std::string& apiKey, const std::string& city) {
    CURL* curl;
    CURLcode res;
    std::string readBuffer;

    std::string url = "https://api.openweathermap.org/data/2.5/weather?q=" + city + "&appid=" + apiKey + "&units=metric";

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    if(curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
    }
    curl_global_cleanup();

    auto j = json::parse(readBuffer);
    return { j["weather"][0]["description"], j["main"]["temp"] };
}
