#ifndef WEATHER_H
#define WEATHER_H

#include <string>

struct Weather {
    std::string description;
    double tempC;
};

Weather getWeather(const std::string &apiKey, const std::string &city);

#endif // WEATHER_H
