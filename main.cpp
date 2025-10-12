/*
 * Author: Gage Lawton
 * Date Written: 2025-10-11
 * Last Updated: 2025-10-11
 * Description: This file contains the main application logic for the WeatherDisplay system.
 */

#include <wiringPiI2C.h>
#include <wiringPi.h>
#include <unistd.h>
#include <iostream>
#include "lcd.h"
#include "weather.h"

int main() {
    const std::string apiKey = "234a6c0a573a4526bbb53739251110";  // Your WeatherAPI.com key
    const std::string city = "Westmont, IL"; // Your city or ZIP code

    std::cout << "\n[INFO] ==============================" << std::endl;
    std::cout << "[INFO] Starting WeatherDisplay" << std::endl;
    std::cout << "[INFO] Location: " << city << std::endl;
    std::cout << "[INFO] Units: Fahrenheit (°F)" << std::endl;
    std::cout << "[INFO] ==============================\n" << std::endl;

    int fd = wiringPiI2CSetup(0x27); // Use your actual LCD I2C address
    if (fd < 0) {
        std::cerr << "[ERROR] Failed to open I2C bus!" << std::endl;
        return 1;
    }

    lcd_init(fd);

    while (true) {
        std::cout << "\n[DEBUG] ---- Fetching weather data ----" << std::endl;

        std::string rawResponse;
        Weather w = getWeather(apiKey, city, &rawResponse);

        float tempF = (w.tempC * 9.0f / 5.0f) + 32.0f;

        std::cout << "[DEBUG] Description : " << w.description << std::endl;
        std::cout << "[DEBUG] Temperature  : " << w.tempC << " °C / " << tempF << " °F" << std::endl;

        // Optional: show raw JSON (comment out if noisy)
        std::cout << "[DEBUG] Raw API JSON : " << rawResponse << std::endl;

        // Prepare LCD display strings
        std::string line1 = "Temp: " + std::to_string((int)tempF) + "F";
        std::string line2 = w.description.substr(0, 16);

        lcd_display(fd, line1, line2);

        std::cout << "[INFO] Display updated: [" << line1 << "] / [" << line2 << "]" << std::endl;
        std::cout << "[INFO] Sleeping for 10 minutes...\n" << std::endl;

        sleep(600); // 10 minutes
    }

    return 0;
}
