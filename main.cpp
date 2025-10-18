#include "config.h"
#include "lcd.h"
#include "weather.h"
#include <wiringPiI2C.h>
#include <wiringPi.h>
#include <unistd.h>
#include <iostream>
#include "oled.h"
#include <thread> // For threading support

int main() {
    Config cfg;
    cfg.load("config.json");

    std::cout << "[INFO] ==============================" << std::endl;
    std::cout << "[INFO] Starting WeatherDisplay" << std::endl;
    std::cout << "[INFO] Location: " << cfg.location << std::endl;
    std::cout << "[INFO] Units: " << cfg.units << std::endl;
    std::cout << "[INFO] Update interval: " << cfg.updateInterval << " seconds" << std::endl;
    std::cout << "[INFO] ==============================\n" << std::endl;

    // Setup LCD
    int fd = wiringPiI2CSetup(0x27); // Adjust I2C address if needed
    if (fd < 0) {
        std::cerr << "[ERROR] Failed to open I2C bus!" << std::endl;
        return 1;
    }
    lcd_init(fd);

    // Start the OLED clock display in a separate thread
    std::thread oledThread(startOLEDClock);

    while (true) {
        std::cout << "[DEBUG] ---- Fetching weather data ----" << std::endl;

        std::string rawResponse;
        Weather w = getWeather(cfg.apiKey, cfg.location, &rawResponse);

        if (w.tempC == -9999.0) {  // In case of error fetching weather data
            std::cerr << "[ERROR] Failed to fetch weather data!" << std::endl;
            continue;  // Skip this iteration and try again after the update interval
        }

        // Convert Celsius to requested units (Fahrenheit or Celsius)
        float temp = (cfg.units == "F") ? (w.tempC * 9.0f / 5.0f) + 32.0f : w.tempC;

        std::cout << "[DEBUG] Description : " << w.description << std::endl;
        std::cout << "[DEBUG] Temperature  : " << w.tempC << " °C / " 
                  << temp << " " << cfg.units << std::endl;

        // Prepare display content for LCD and OLED
        std::string line1 = "Temp: " + std::to_string((int)temp) + cfg.units;
        std::string line2 = w.description.substr(0, 16); // Shorten description if necessary

        lcd_display(fd, line1, line2);

        std::cout << "[INFO] Display updated: [" << line1 << "] / [" << line2 << "]" << std::endl;
        std::cout << "[INFO] Sleeping for " << cfg.updateInterval << " seconds...\n" << std::endl;

        sleep(cfg.updateInterval);
    }

    // Join the OLED thread before exiting
    oledThread.join();

    return 0;
}
