#include "config.h"
#include "lcd.h"
#include "weather.h"
#include <wiringPiI2C.h>
#include <wiringPi.h>
#include <unistd.h>
#include <iostream>
#include <cstdlib> // for system()

// 🆕 Function to call the Python script that displays time on the OLED
void displayCurrentTimeOnOLED() {
    int result = system("python3 scripts/oled_display.py");
    if (result != 0) {
        std::cerr << "[ERROR] Failed to update OLED with current time." << std::endl;
    } else {
        std::cout << "[INFO] OLED display updated with current time." << std::endl;
    }
}

int main() {
    Config cfg;
    cfg.load("config.json");

    std::cout << "[INFO] ==============================" << std::endl;
    std::cout << "[INFO] Starting WeatherDisplay" << std::endl;
    std::cout << "[INFO] Location: " << cfg.location << std::endl;
    std::cout << "[INFO] Units: " << cfg.units << std::endl;
    std::cout << "[INFO] Update interval: " << cfg.updateInterval << " seconds" << std::endl;
    std::cout << "[INFO] ==============================\n" << std::endl;

    // 🆕 Display current time on OLED
    displayCurrentTimeOnOLED();

    // Setup LCD
    int fd = wiringPiI2CSetup(0x27); // Adjust I2C address
    if (fd < 0) {
        std::cerr << "[ERROR] Failed to open I2C bus!" << std::endl;
        return 1;
    }
    lcd_init(fd);

    while (true) {
        std::cout << "[DEBUG] ---- Fetching weather data ----" << std::endl;

        std::string rawResponse;
        Weather w = getWeather(cfg.apiKey, cfg.location, &rawResponse);

        float temp = (cfg.units == "F") ? (w.tempC * 9.0f / 5.0f) + 32.0f : w.tempC;

        std::cout << "[DEBUG] Description : " << w.description << std::endl;
        std::cout << "[DEBUG] Temperature  : " << w.tempC << " °C / " 
                  << temp << " " << cfg.units << std::endl;

        std::string line1 = "Temp: " + std::to_string((int)temp) + cfg.units;
        std::string line2 = w.description.substr(0, 16);

        lcd_display(fd, line1, line2);

        std::cout << "[INFO] Display updated: [" << line1 << "] / [" << line2 << "]" << std::endl;
        std::cout << "[INFO] Sleeping for " << cfg.updateInterval << " seconds...\n" << std::endl;

        sleep(cfg.updateInterval);
    }

    return 0;
}
