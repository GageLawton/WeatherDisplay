#include "weather.h"
#include "log.h"
#include <curl/curl.h>
#include <ctime>
#include <iomanip>
#include <sstream>
#include "json.hpp"

using json = nlohmann::json;

namespace {

std::string urlEncode(const std::string& value) {
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

size_t writeCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    static_cast<std::string*>(userp)->append(static_cast<char*>(contents), size * nmemb);
    return size * nmemb;
}

// Helper: safely extract a string from a JSON value, returning fallback on missing/null.
std::string jsonStr(const json& j, const char* key, const std::string& fallback = "") {
    if (!j.contains(key) || j[key].is_null()) return fallback;
    try { return j[key].get<std::string>(); } catch (...) { return fallback; }
}

float jsonFloat(const json& j, const char* key, float fallback = 0.0f) {
    if (!j.contains(key) || j[key].is_null()) return fallback;
    try { return j[key].get<float>(); } catch (...) { return fallback; }
}

int jsonInt(const json& j, const char* key, int fallback = 0) {
    if (!j.contains(key) || j[key].is_null()) return fallback;
    try { return j[key].get<int>(); } catch (...) { return fallback; }
}

// Parse "YYYY-MM-DD" into Unix epoch seconds at UTC midnight.
std::time_t parseIsoDate(const std::string& iso) {
    std::tm tm{};
    if (iso.size() < 10) return 0;
    tm.tm_year = std::atoi(iso.substr(0, 4).c_str()) - 1900;
    tm.tm_mon  = std::atoi(iso.substr(5, 2).c_str()) - 1;
    tm.tm_mday = std::atoi(iso.substr(8, 2).c_str());
    return timegm(&tm);
}

} // anonymous namespace

WeatherReport getWeatherReport(const std::string& apiKey,
                                const std::string& city,
                                std::string* rawResponse) {
    WeatherReport report;

    if (apiKey.empty()) {
        LOG_ERROR("getWeatherReport called with empty API key");
        return report;
    }

    // Request 3 days of forecast + alerts in a single call.
    std::string url = "https://api.weatherapi.com/v1/forecast.json"
                      "?key=" + apiKey +
                      "&q=" + urlEncode(city) +
                      "&days=3"
                      "&alerts=yes"
                      "&aqi=no";

    LOG_INFO("Fetching weather report for: " << city);

    std::string buffer;
    CURL* curl = curl_easy_init();
    if (!curl) {
        LOG_ERROR("CURL init failed");
        return report;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);
    curl_easy_setopt(curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);  // forecast is bigger than current

    CURLcode res = curl_easy_perform(curl);
    long httpCode = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        LOG_ERROR("CURL request failed: " << curl_easy_strerror(res));
        return report;
    }
    if (httpCode != 200) {
        LOG_ERROR("Weather API returned HTTP " << httpCode);
        return report;
    }

    if (rawResponse) *rawResponse = buffer;

    try {
        auto j = json::parse(buffer);

        // Current conditions
        if (j.contains("current")) {
            const auto& c = j["current"];
            report.current.tempC = jsonFloat(c, "temp_c");
            if (c.contains("condition")) {
                report.current.description = jsonStr(c["condition"], "text", "N/A");
            }
        }

        // Forecast
        if (j.contains("forecast") && j["forecast"].contains("forecastday")) {
            for (const auto& d : j["forecast"]["forecastday"]) {
                DailyForecast df;
                df.date = parseIsoDate(jsonStr(d, "date"));
                if (d.contains("day")) {
                    const auto& day = d["day"];
                    df.highC        = jsonFloat(day, "maxtemp_c");
                    df.lowC         = jsonFloat(day, "mintemp_c");
                    df.chanceOfRain = jsonInt(day, "daily_chance_of_rain");
                    df.willItRain   = jsonInt(day, "daily_will_it_rain") == 1;
                    df.willItSnow   = jsonInt(day, "daily_will_it_snow") == 1;
                    if (day.contains("condition")) {
                        df.description = jsonStr(day["condition"], "text");
                    }
                }
                report.forecast.push_back(df);
            }
        }

        // Alerts
        if (j.contains("alerts") && j["alerts"].contains("alert")) {
            for (const auto& a : j["alerts"]["alert"]) {
                WeatherAlert wa;
                wa.headline    = jsonStr(a, "headline");
                wa.event       = jsonStr(a, "event");
                wa.severity    = jsonStr(a, "severity");
                wa.description = jsonStr(a, "desc");
                report.alerts.push_back(wa);
            }
        }

        report.fetchedAt = std::time(nullptr);
        report.valid     = true;
        LOG_INFO("Weather report parsed: "
                 << report.current.description
                 << ", " << report.forecast.size() << " forecast day(s), "
                 << report.alerts.size() << " alert(s)");

    } catch (const std::exception& e) {
        LOG_ERROR("JSON parse error: " << e.what());
    }

    return report;
}