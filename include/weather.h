#ifndef WEATHER_H
#define WEATHER_H

#include <ctime>
#include <string>
#include <vector>

// Current conditions parsed out of the API's `current` block.
struct CurrentConditions {
    std::string description;   // e.g. "Partly cloudy"
    float       tempC = 0.0f;
};

// One day in the forecast (from the API's `forecast.forecastday[]`).
struct DailyForecast {
    std::time_t date = 0;         // UTC midnight of the day in question
    float       highC = 0.0f;
    float       lowC  = 0.0f;
    std::string description;
    int         chanceOfRain = 0; // percent, 0-100
    bool        willItRain   = false;
    bool        willItSnow   = false;
};

// Weather alert as reported by weatherapi.com (typically NOAA-sourced in the US).
struct WeatherAlert {
    std::string headline;   // e.g. "Tornado Warning issued ..."
    std::string event;      // e.g. "Tornado Warning"
    std::string severity;   // e.g. "Severe", "Moderate", "Minor"
    std::string description;
};

// Full report from one API call. Stores everything we need on the device.
struct WeatherReport {
    bool                       valid = false;
    std::time_t                fetchedAt = 0;     // when we received this report
    CurrentConditions          current;
    std::vector<DailyForecast> forecast;          // [0] = today, [1] = tomorrow, ...
    std::vector<WeatherAlert>  alerts;
};

// Fetch a full weather report (current + 3-day forecast + alerts) from
// weatherapi.com. Returns an invalid report on failure (check .valid).
//
// `rawResponse` is optional: if provided, the raw JSON body is stored
// there for debugging.
WeatherReport getWeatherReport(const std::string& apiKey,
                                const std::string& city,
                                std::string* rawResponse = nullptr);

#endif