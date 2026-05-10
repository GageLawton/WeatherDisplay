#ifndef TEMPERATURE_H
#define TEMPERATURE_H

#include <string>

namespace temperature {

inline float cToF(float celsius) {
    return celsius * 9.0f / 5.0f + 32.0f;
}

inline float fToC(float fahrenheit) {
    return (fahrenheit - 32.0f) * 5.0f / 9.0f;
}

// Convert Celsius to the desired unit string ("F" or "C").
// Anything other than "F" returns the Celsius value unchanged.
inline float toUnits(float celsius, const std::string& units) {
    return (units == "F") ? cToF(celsius) : celsius;
}

} // namespace temperature

#endif